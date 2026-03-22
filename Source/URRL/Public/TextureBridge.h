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
static constexpr DWORD    TEX_SHM_SIZE   = TEX_DATA_BASE + TEX_COUNT * TEX_BYTES_EACH; // ~16 MB

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

    static FTextureBridge Instance;

private:
    HANDLE  hMapFile = nullptr;
    uint8*  Base     = nullptr;
};
