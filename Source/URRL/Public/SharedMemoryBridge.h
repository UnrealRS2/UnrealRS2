#pragma once

//Win32 BULLSHIT
#define NOMINMAX

#include "CoreMinimal.h"
#include <windows.h>
#include <atomic>
#include <cstdint>

// POD structs with fixed layout for predictable offsets
#pragma pack(push, 1)
struct RLCameraStatus
{
    int x, y, z;
    int pitch, yaw, scale;
};
struct RLFrameBuffer
{
    int width;
    int height;
    bool ready;
    bool consumed = true;

    //dummy padding so pixels is 4-byte aligned
    uint8_t dummy;
    uint8_t dummy2;

    uint8_t pixels[3840 * 2160 * 4];
};
struct SResolution {
    int width;
    int height;
    bool consumed = true;
};
struct SMouseMove {
    int x;
    int y;
    bool consumed = true;
};

struct SMousePress {
    int button;
    bool consumed = true;
};

struct SMouseRelease {
    int button;
    bool consumed = true;
};
#pragma pack(pop)

class FSharedMemoryBridge
{
public:
    FSharedMemoryBridge();
    ~FSharedMemoryBridge();

    bool Init(const char* name);
    void Shutdown();

    static RLCameraStatus* RLCameraStatusPtr;
    static RLFrameBuffer* RLFrameBufferPtr;
    static SResolution* Resolution;
    static SMouseMove* MouseMove;
    static SMousePress* MousePress;
    static SMouseRelease* MouseRelease;

    static void* Raw;
    static FSharedMemoryBridge SharedMemoryBridge;

private:
    HANDLE hMapFile = nullptr;
    size_t TotalSize = 0;
};
