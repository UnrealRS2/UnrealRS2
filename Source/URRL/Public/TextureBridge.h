#pragma once

#define NOMINMAX
#include "CoreMinimal.h"
#include <windows.h>
#include <atomic>
#include <cstdint>

static constexpr uint32_t TEX_COUNT      = 256;
static constexpr uint32_t TEX_SIZE       = 128;
static constexpr uint32_t TEX_BYTES_EACH = TEX_SIZE * TEX_SIZE * 4; // RGBA8
static constexpr uint32_t TEX_OFF_READY  = 0;
static constexpr uint32_t TEX_DATA_BASE  = 8;
// Animation speeds: 2 floats (animU, animV) per texture, pre-scaled to UV-units/sec.
// Starts immediately after all pixel data.
static constexpr uint32_t TEX_ANIM_BASE  = TEX_DATA_BASE + TEX_COUNT * TEX_BYTES_EACH;
static constexpr DWORD    TEX_SHM_SIZE   = TEX_ANIM_BASE + TEX_COUNT * 2 * sizeof(float); // ~16 MB + 2 KB

class FTextureBridge
{
public:
    bool Init(const char* Name);
    void Shutdown();

    bool IsInitialized() const { return Base != nullptr; }

    /** True once Java has written all 256 textures and set the ready flag. */
    bool IsReady() const;

    /** Pointer to the raw RGBA pixel data, or nullptr if not mapped.
     *  Layout: texture[id] starts at offset id * TEX_BYTES_EACH. */
    const uint8* GetTextureData() const { return Base ? (Base + TEX_DATA_BASE) : nullptr; }

    /** Pointer to animation speed pairs: float[TEX_COUNT * 2], pre-scaled to UV-units/sec.
     *  animSpeeds[id*2+0] = U speed, animSpeeds[id*2+1] = V speed. */
    const float* GetAnimSpeedData() const { return Base ? reinterpret_cast<const float*>(Base + TEX_ANIM_BASE) : nullptr; }

    static FTextureBridge Instance;

private:
    HANDLE  hMapFile = nullptr;
    uint8*  Base     = nullptr;
};
