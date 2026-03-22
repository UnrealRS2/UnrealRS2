#define NOMINMAX
#include "TextureBridge.h"
#include <atomic>

FTextureBridge FTextureBridge::Instance;

bool FTextureBridge::Init(const char* Name)
{
    hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, Name);
    if (!hMapFile)
    {
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
            0, TEX_SHM_SIZE, Name);
        if (!hMapFile)
        {
            UE_LOG(LogTemp, Error, TEXT("TextureBridge: failed to open/create %hs"), Name);
            return false;
        }
    }

    Base = reinterpret_cast<uint8*>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, TEX_SHM_SIZE));
    if (!Base)
    {
        CloseHandle(hMapFile);
        hMapFile = nullptr;
        UE_LOG(LogTemp, Error, TEXT("TextureBridge: MapViewOfFile failed for %hs"), Name);
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("TextureBridge: mapped %hs (%u bytes, %u slots)"), Name, TEX_SHM_SIZE, TEX_COUNT);
    return true;
}

void FTextureBridge::Shutdown()
{
    if (Base)     { UnmapViewOfFile(Base);  Base = nullptr; }
    if (hMapFile) { CloseHandle(hMapFile);  hMapFile = nullptr; }
}

bool FTextureBridge::IsReady() const
{
    if (!Base) return false;
    return std::atomic_ref<uint32_t>(
        *reinterpret_cast<uint32_t*>(Base + TEX_OFF_READY)
    ).load(std::memory_order_acquire) == 1;
}
