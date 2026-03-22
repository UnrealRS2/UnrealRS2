#pragma once

#define NOMINMAX
#include "CoreMinimal.h"
#include <windows.h>
#include <atomic>
#include <cstdint>

// ── Ring-buffer shared memory protocol ───────────────────────────────────────
//
// "URRL_Scene" layout (~32 MB):
//
// Header (16 bytes):
//   [0]  write_head (uint32, atomic) — Java increments after writing a slot
//   [4]  read_head  (uint32, atomic) — UE increments after reading a slot
//   [8]  pad[8]
//
// Slots (SCENE_SLOT_COUNT × SCENE_SLOT_BYTES, starting at byte 16):
//   [0]  command          (uint8)  — 1=ZONE_DATA, 2=ZONE_CLEAR, 3=SCENE_CLEAR
//   [1]  pad[3]
//   [4]  zone_x           (int32)
//   [8]  zone_z           (int32)
//   [12] opaque_int_count (int32) — number of int32s of opaque vertex data
//   [16] alpha_int_count  (int32) — number of int32s of alpha vertex data
//   [20] data[]           — opaque ints then alpha ints (5 ints per vertex)
//
// Vertex encoding (put22224 + put2222 from GpuIntBuffer):
//   data[i+0] = (lh << 16) | (lx & 0xffff)  → local height + local x
//   data[i+1] = lz & 0xffff                  → local z (signed 16-bit)
//   data[i+2] = hsl                           → packed HSL color
//   data[i+3] = (tu << 16) | (tt & 0xffff)   → Lo=tt (1-based tex ID), Hi=tu (U coord)
//   data[i+4] = (tf << 16) | (tv & 0xffff)   → Lo=tv (V coord), Hi=tf (tex flags)
//
// World position = zone base + local:
//   worldX = (zone_x - SCENE_OFFSET_ZONES) * 1024 + sign_extend16(lx)
//   worldZ = (zone_z - SCENE_OFFSET_ZONES) * 1024 + sign_extend16(lz)
//   worldY = sign_extend16(lh)   (height, typically negative in RS)
//
// Ordering:
//   Java: write slot fields + data, releaseFence(), store write_head (release)
//   UE:   load write_head (acquire), read slot, Memcpy data, store read_head (release)

// Ring dimensions
static constexpr uint32_t SCENE_SLOT_COUNT  = 256;
static constexpr uint32_t SCENE_SLOT_BYTES  = 2 * 1024 * 1024;  // 2 MB per slot
static constexpr DWORD    SCENE_SHM_SIZE    = 16 + SCENE_SLOT_COUNT * SCENE_SLOT_BYTES; // ~512 MB

// Header offsets
static constexpr int SCENE_OFF_WRITE_HEAD   = 0;   // uint32 (Java writes)
static constexpr int SCENE_OFF_READ_HEAD    = 4;   // uint32 (UE writes)
static constexpr int SCENE_SLOTS_BASE       = 16;  // first slot starts here

// Per-slot offsets (relative to each slot's base)
static constexpr int SLOT_OFF_COMMAND       = 0;
static constexpr int SLOT_OFF_ZONE_X        = 4;
static constexpr int SLOT_OFF_ZONE_Z        = 8;
static constexpr int SLOT_OFF_OPAQUE_COUNT  = 12;
static constexpr int SLOT_OFF_ALPHA_COUNT   = 16;
static constexpr int SLOT_OFF_DATA          = 20;

static constexpr uint8_t SCENE_CMD_ZONE_DATA   = 1;
static constexpr uint8_t SCENE_CMD_ZONE_CLEAR  = 2;
static constexpr uint8_t SCENE_CMD_SCENE_CLEAR = 3;

// RuneLite OSRS coordinate scale: 1 tile = 128 RS units in X/Z.
// Unreal units: 1 UE unit = 1 cm. OSRS tile = ~0.9m → ~90 UE units/tile.
// RS units per tile = 128, so scale ≈ 90 / 128 ≈ 0.7f.
// Adjust to taste.
static constexpr float RS_TO_UE_SCALE = 0.7f;

// (EXTENDED_SCENE_SIZE - SCENE_SIZE) / 2 / 8  =  (184 - 104) / 2 / 8  = 5
// Subtract this from zone_x/zone_z to convert extended-scene zone index → scene-local zone index.
static constexpr int32 SCENE_OFFSET_ZONES = 5;

struct FZonePacket
{
    uint8  Command;
    int32  ZoneX;
    int32  ZoneZ;
    int32  OpaqueIntCount;
    int32  AlphaIntCount;
    TArray<int32> Data;  // copied out of shared memory before advancing read_head
};

class FSceneGraphBridge
{
public:
    bool Init(const char* Name);
    void Shutdown();

    /** Call from game thread each tick. Returns true + fills OutPacket when a
     *  zone arrives. Copies vertex data out and advances read_head internally
     *  so Java can immediately start writing the next slot. */
    bool PollZone(FZonePacket& OutPacket);
    bool IsInitialized() const { return Base != nullptr; }

    static FSceneGraphBridge Instance;

private:
    HANDLE   hMapFile = nullptr;
    uint8*   Base     = nullptr;
};
