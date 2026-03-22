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

    UE_LOG(LogTemp, Log, TEXT("SceneGraphBridge: slot[%u] cmd=%d zone(%d,%d) opaque=%d alpha=%d"),
        SlotIdx, Out.Command, Out.ZoneX, Out.ZoneZ, Out.OpaqueIntCount, Out.AlphaIntCount);

    // Clamp to the slot's actual data capacity to guard against corrupt headers.
    static constexpr int32 MaxSlotDataInts = (SCENE_SLOT_BYTES - SLOT_OFF_DATA) / sizeof(int32);
    Out.OpaqueIntCount = FMath::Min(Out.OpaqueIntCount, MaxSlotDataInts);
    Out.AlphaIntCount  = FMath::Min(Out.AlphaIntCount,  MaxSlotDataInts - Out.OpaqueIntCount);

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
