#include "SharedMemoryBridge.h"

FSharedMemoryBridge::FSharedMemoryBridge() {}
FSharedMemoryBridge::~FSharedMemoryBridge()
{
    Shutdown();
}

bool FSharedMemoryBridge::Init(const char* name)
{
    const size_t shmSize =
        sizeof(RLCameraStatus) +
            sizeof(RLFrameBuffer) +
                sizeof(SResolution) +
                    sizeof(SMouseMove) +
                        sizeof(SMousePress) +
                            sizeof(SMouseRelease) +
                                sizeof(SMouseWheel) +
                                    sizeof(SKeyQueue);

    hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name);

    if (!hMapFile)
    {
        hMapFile = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            nullptr,
            PAGE_READWRITE,
            (DWORD)(shmSize >> 32),
            (DWORD)(shmSize & 0xffffffff),
            name
        );

        if (!hMapFile)
        {
            UE_LOG(LogTemp, Error, TEXT("SharedMemoryBridge: CreateFileMapping failed"));
            return false;
        }
    }

    Raw = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!Raw)
    {
        CloseHandle(hMapFile);
        hMapFile = nullptr;
        UE_LOG(LogTemp, Error, TEXT("SharedMemoryBridge: MapViewOfFile failed"));
        return false;
    }

    uint8_t* ptr = reinterpret_cast<uint8_t*>(Raw);
    
    RLCameraStatusPtr = reinterpret_cast<RLCameraStatus*>(ptr);
    ptr += sizeof(RLCameraStatus);
    
    RLFrameBufferPtr = reinterpret_cast<RLFrameBuffer*>(ptr);
    ptr += sizeof(RLFrameBuffer);
    
    Resolution = reinterpret_cast<SResolution*>(ptr);
    ptr += sizeof(SResolution);
    
    MouseMove = reinterpret_cast<SMouseMove*>(ptr);
    ptr += sizeof(SMouseMove);
    
    MousePress = reinterpret_cast<SMousePress*>(ptr);
    ptr += sizeof(SMousePress);
    
    MouseRelease = reinterpret_cast<SMouseRelease*>(ptr);
    ptr += sizeof(SMouseRelease);

    MouseWheel = reinterpret_cast<SMouseWheel*>(ptr);
    ptr += sizeof(SMouseWheel);

    KeyQueue = reinterpret_cast<SKeyQueue*>(ptr);
    ptr += sizeof(SKeyQueue);

    TotalSize = ptr - reinterpret_cast<uint8_t*>(Raw);

    return true;
}

void FSharedMemoryBridge::Shutdown()
{
    if (Raw)
    {
        UnmapViewOfFile(Raw);
        Raw = nullptr;
    }
    if (hMapFile)
    {
        CloseHandle(hMapFile);
        hMapFile = nullptr;
    }
}