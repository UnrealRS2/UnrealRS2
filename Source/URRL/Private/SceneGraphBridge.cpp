#define NOMINMAX
#include "SceneGraphBridge.h"
#include <atomic>

bool FSceneGraphBridge::Init(const char* Name)
{
    hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, Name);
    if (!hMapFile)
    {
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            0,
            SCENE_SHM_SIZE,
            Name
        );

        if (!hMapFile)
        {
            UE_LOG(LogTemp, Error, TEXT("SceneGraphBridge: CreateFileMapping failed for %hs"), Name);
            return false;
        }
    }

    Base = reinterpret_cast<uint8*>(
        MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SCENE_SHM_SIZE));
    if (!Base)
    {
        CloseHandle(hMapFile);
        hMapFile = nullptr;
        UE_LOG(LogTemp, Error, TEXT("SceneGraphBridge: MapViewOfFile failed for %hs"), Name);
        return false;
    }

    // Skip any zones already pending in the ring (Java may have started first).
    // Read write_head and park read_head at the same position.
    const uint32_t CurrentWriteHead = std::atomic_ref<uint32_t>(
        *reinterpret_cast<uint32_t*>(Base + SCENE_OFF_WRITE_HEAD)
    ).load(std::memory_order_acquire);

    std::atomic_ref<uint32_t>(
        *reinterpret_cast<uint32_t*>(Base + SCENE_OFF_READ_HEAD)
    ).store(CurrentWriteHead, std::memory_order_release);

    UE_LOG(LogTemp, Log, TEXT("SceneGraphBridge: opened %hs (%u bytes, %u slots)"),
        Name, SCENE_SHM_SIZE, SCENE_SLOT_COUNT);
    return true;
}

void FSceneGraphBridge::Shutdown()
{
    if (Base)
    {
        UnmapViewOfFile(Base);
        Base = nullptr;
    }
    if (hMapFile)
    {
        CloseHandle(hMapFile);
        hMapFile = nullptr;
    }
}

void FSceneGraphBridge::SetFlags(uint32_t Flags)
{
    if (!Base) return;
    std::atomic_ref<uint32_t>(
        *reinterpret_cast<uint32_t*>(Base + SCENE_OFF_UE_FLAGS)
    ).store(Flags, std::memory_order_relaxed);
}

bool FSceneGraphBridge::PollZone(FZonePacket& Out)
{
    if (!Base)
    {
        return false;
    }

    // Acquire-load write_head: if > read_head, a slot is ready.
    const uint32_t WriteHead = std::atomic_ref<uint32_t>(
        *reinterpret_cast<uint32_t*>(Base + SCENE_OFF_WRITE_HEAD)
    ).load(std::memory_order_acquire);

    const uint32_t ReadHead = *reinterpret_cast<const uint32_t*>(Base + SCENE_OFF_READ_HEAD);

    if (WriteHead == ReadHead)
    {
        return false;  // ring empty
    }

    const uint32_t SlotIdx  = ReadHead % SCENE_SLOT_COUNT;
    const uint8*   Slot     = Base + SCENE_SLOTS_BASE + SlotIdx * SCENE_SLOT_BYTES;

    Out.Command        = Slot[SLOT_OFF_COMMAND];
    Out.ZoneX          = *reinterpret_cast<const int32*>(Slot + SLOT_OFF_ZONE_X);
    Out.ZoneZ          = *reinterpret_cast<const int32*>(Slot + SLOT_OFF_ZONE_Z);
    Out.OpaqueIntCount = *reinterpret_cast<const int32*>(Slot + SLOT_OFF_OPAQUE_COUNT);
    Out.AlphaIntCount  = *reinterpret_cast<const int32*>(Slot + SLOT_OFF_ALPHA_COUNT);
    Out.LevelOffsets[0] = *reinterpret_cast<const int32*>(Slot + SLOT_OFF_LO0);
    Out.LevelOffsets[1] = *reinterpret_cast<const int32*>(Slot + SLOT_OFF_LO1);
    Out.LevelOffsets[2] = *reinterpret_cast<const int32*>(Slot + SLOT_OFF_LO2);
    Out.LevelOffsets[3] = *reinterpret_cast<const int32*>(Slot + SLOT_OFF_LO3);

    UE_LOG(LogTemp, Log, TEXT("SceneGraphBridge: slot[%u] cmd=%d zone(%d,%d) opaque=%d alpha=%d"),
        SlotIdx, Out.Command, Out.ZoneX, Out.ZoneZ, Out.OpaqueIntCount, Out.AlphaIntCount);

    // Clamp to the slot's actual data capacity to guard against corrupt headers.
    static constexpr int32 MaxSlotDataInts = (SCENE_SLOT_BYTES - SLOT_OFF_DATA) / sizeof(int32);
    Out.OpaqueIntCount = FMath::Min(Out.OpaqueIntCount, MaxSlotDataInts);
    Out.AlphaIntCount  = FMath::Min(Out.AlphaIntCount,  MaxSlotDataInts - Out.OpaqueIntCount);
    for (int i = 0; i < 4; ++i)
    {
        Out.LevelOffsets[i] = FMath::Clamp(Out.LevelOffsets[i], 0, Out.OpaqueIntCount);
    }

    const int32 TotalInts = Out.OpaqueIntCount + Out.AlphaIntCount;
    Out.Data.SetNumUninitialized(TotalInts);
    if (TotalInts > 0)
    {
        FMemory::Memcpy(
            Out.Data.GetData(),
            Slot + SLOT_OFF_DATA,
            TotalInts * sizeof(int32));
    }

    // Advance read_head: releases this slot back to Java.
    std::atomic_ref<uint32_t>(
        *reinterpret_cast<uint32_t*>(Base + SCENE_OFF_READ_HEAD)
    ).store(ReadHead + 1, std::memory_order_release);

    return true;
}

uint32_t FSceneGraphBridge::ReadJavaFlags() const
{
    if (!Base) return 0;
    return std::atomic_ref<const uint32_t>(
        *reinterpret_cast<const uint32_t*>(Base + SCENE_OFF_JAVA_FLAGS)
    ).load(std::memory_order_relaxed);
}

// ── FEntityBridge ─────────────────────────────────────────────────────────────

FEntityBridge FEntityBridge::Instance;

bool FEntityBridge::Init(const char* Name)
{
    hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, Name);
    if (!hMapFile)
    {
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
            0,               // high 32 bits — ENTITY_SHM_SIZE fits in 32 bits
            ENTITY_SHM_SIZE,
            Name);
        if (!hMapFile)
        {
            UE_LOG(LogTemp, Error, TEXT("EntityBridge: CreateFileMapping failed for %hs"), Name);
            return false;
        }
    }

    Base = reinterpret_cast<uint8*>(
        MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, ENTITY_SHM_SIZE));
    if (!Base)
    {
        CloseHandle(hMapFile); hMapFile = nullptr;
        UE_LOG(LogTemp, Error, TEXT("EntityBridge: MapViewOfFile failed for %hs"), Name);
        return false;
    }

    // Park read_seq at current write_seq so we don't replay stale data.
    const uint32 WriteSeq = std::atomic_ref<uint32>(
        *reinterpret_cast<uint32*>(Base + ENTITY_OFF_WRITE_SEQ)
    ).load(std::memory_order_acquire);
    LocalReadSeq = WriteSeq;
    std::atomic_ref<uint32>(
        *reinterpret_cast<uint32*>(Base + ENTITY_OFF_READ_SEQ)
    ).store(WriteSeq, std::memory_order_release);

    UE_LOG(LogTemp, Log, TEXT("EntityBridge: opened %hs (%u bytes)"), Name, ENTITY_SHM_SIZE);
    return true;
}

void FEntityBridge::Shutdown()
{
    if (Base)     { UnmapViewOfFile(Base); Base = nullptr; }
    if (hMapFile) { CloseHandle(hMapFile); hMapFile = nullptr; }
}

bool FEntityBridge::Peek(const int32*& OutData, int32& OutIntCount)
{
    if (!Base) return false;

    const uint32 WriteSeq = std::atomic_ref<uint32>(
        *reinterpret_cast<uint32*>(Base + ENTITY_OFF_WRITE_SEQ)
    ).load(std::memory_order_acquire);

    if (WriteSeq == LocalReadSeq) return false;

    OutIntCount = *reinterpret_cast<const int32*>(Base + ENTITY_OFF_INT_COUNT);
    OutIntCount = FMath::Clamp(OutIntCount, 0,
        (int32)((ENTITY_SHM_SIZE - ENTITY_DATA_BASE) / sizeof(int32)));
    OutData = reinterpret_cast<const int32*>(Base + ENTITY_DATA_BASE);

    // Store WriteSeq so Commit() knows what to advance to
    LocalReadSeq = WriteSeq;
    return true;
}

void FEntityBridge::Commit()
{
    if (!Base) return;
    std::atomic_ref<uint32>(
        *reinterpret_cast<uint32*>(Base + ENTITY_OFF_READ_SEQ)
    ).store(LocalReadSeq, std::memory_order_release);
}
