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
    int viewportW, viewportH;
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

// UE → Java: accumulated mouse-wheel delta.
// UE atomically adds ±1; Java reads the value, dispatches N wheel events, and writes 0.
struct SMouseWheel {
    int32_t accum;  // positive = scroll down (zoom in), negative = scroll up (zoom out)
};

// Single key event in the key queue.
struct SKeyEvent {
    int32_t id;        // 400 = KEY_TYPED, 401 = KEY_PRESSED, 402 = KEY_RELEASED
    int32_t keyCode;   // Java VK_ code (0 = VK_UNDEFINED for KEY_TYPED)
    int32_t keyChar;   // Unicode codepoint; 0xFFFF = KeyEvent.CHAR_UNDEFINED
    int32_t modifiers; // Java InputEvent bits: SHIFT=64, CTRL=128, ALT=512
};

// Lock-free SPSC key event ring buffer (UE produces, Java consumes).
static constexpr int KEY_QUEUE_CAPACITY = 32;
struct SKeyQueue {
    int32_t writeHead;  // written by UE (producer)
    int32_t readHead;   // written by Java (consumer)
    SKeyEvent events[KEY_QUEUE_CAPACITY];
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
    static SMouseWheel* MouseWheel;
    static SKeyQueue*   KeyQueue;

    static void* Raw;
    static FSharedMemoryBridge SharedMemoryBridge;

private:
    HANDLE hMapFile = nullptr;
    size_t TotalSize = 0;
};
