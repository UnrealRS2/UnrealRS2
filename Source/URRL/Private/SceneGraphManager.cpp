#define NOMINMAX
#include "SceneGraphManager.h"
#include "SceneGraphBridge.h"
#include "TextureBridge.h"
#include "Engine/Texture.h"
#include "HAL/IConsoleManager.h"
#include "Async/Async.h"

static TAutoConsoleVariable<int32> CVarStaticLighting(
    TEXT("urrl.StaticLighting"),
    1,
    TEXT("0 = UE dynamic lighting, 1 = static/unlit (vanilla OSRS look)"),
    ECVF_Default);

static TAutoConsoleVariable<int32> CVarPassthrough(
    TEXT("urrl.Passthrough"),
    0,
    TEXT("1 = tell RuneLite to skip its GL render pass (UE is the renderer)"),
    ECVF_Default);

// Windows.h defines UpdateResource as UpdateResourceA/W which conflicts with
// UTexture::UpdateResource(). Undefine it after our platform includes.
#ifdef UpdateResource
#undef UpdateResource
#endif

ASceneGraphManager::ASceneGraphManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASceneGraphManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Wait for any in-flight decode before the actor is destroyed
    if (EntityDecodeTask.IsValid())
    {
        EntityDecodeTask.Wait();
    }
    Super::EndPlay(EndPlayReason);
}

void ASceneGraphManager::BeginPlay()
{
    Super::BeginPlay();

    ZoneMaterial = Cast<UMaterialInterface>(
        StaticLoadObject(UMaterialInterface::StaticClass(), nullptr,
            TEXT("/Game/Materials/M_RSZone.M_RSZone")));

    ZoneAlphaMaterial = Cast<UMaterialInterface>(
        StaticLoadObject(UMaterialInterface::StaticClass(), nullptr,
            TEXT("/Game/Materials/M_RSZoneAlpha.M_RSZoneAlpha")));

    if (ZoneMaterial)
    {
        UE_LOG(LogTemp, Log, TEXT("SceneGraphManager: loaded M_RSZone"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("SceneGraphManager: failed to load /Game/Materials/M_RSZone"));
    }

    if (ZoneAlphaMaterial)
    {
        UE_LOG(LogTemp, Log, TEXT("SceneGraphManager: loaded M_RSZoneAlpha"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SceneGraphManager: M_RSZoneAlpha not found — alpha geometry will be invisible"));
    }
}

void ASceneGraphManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    const bool bBridgeReady = FTextureBridge::Instance.IsReady();
    if (bBridgeReady && !bLastBridgeReady)
    {
        // Rising edge: Java just finished writing — always re-upload
        UE_LOG(LogTemp, Log, TEXT("SceneGraphManager: texture bridge ready signal detected, re-uploading"));
        bTexturesUploaded = false;
    }
    bLastBridgeReady = bBridgeReady;
    if (!bTexturesUploaded)
    {
        TryUploadTextures();
    }

    // Sync UE→Java flags (passthrough, etc.) into SHM header
    {
        uint32_t Flags = 0;
        if (CVarPassthrough.GetValueOnGameThread() != 0) Flags |= SCENE_FLAG_PASSTHROUGH;
        FSceneGraphBridge::Instance.SetFlags(Flags);
    }

    // Sync StaticLighting CVar to both materials
    const float NewVal = (float)CVarStaticLighting.GetValueOnGameThread();
    if (NewVal != CachedStaticLighting)
    {
        CachedStaticLighting = NewVal;
        if (ZoneMaterialDynamic)
            ZoneMaterialDynamic->SetScalarParameterValue(TEXT("StaticLighting"), NewVal);
        if (ZoneAlphaMaterialDynamic)
            ZoneAlphaMaterialDynamic->SetScalarParameterValue(TEXT("StaticLighting"), NewVal);
    }

    // Read Java→UE hide-roofs flag and toggle roof sections if it changed
    {
        const bool bNewHideRoofs = (FSceneGraphBridge::Instance.ReadJavaFlags() & JAVA_FLAG_HIDE_ROOFS) != 0;
        if (bNewHideRoofs != bHideRoofs)
        {
            bHideRoofs = bNewHideRoofs;
            for (int64 Key : ZoneRoofKeys)
            {
                if (UProceduralMeshComponent** Found = ZoneMeshes.Find(Key))
                {
                    (*Found)->SetMeshSectionVisible(1, !bHideRoofs);
                }
            }
        }
    }

    // Drain all queued zone packets this tick
    FZonePacket Packet;
    while (FSceneGraphBridge::Instance.PollZone(Packet))
    {
        switch (Packet.Command)
        {
        case SCENE_CMD_ZONE_DATA:
            ProcessZoneData(Packet);
            break;
        case SCENE_CMD_ZONE_CLEAR:
            ProcessZoneClear(Packet.ZoneX, Packet.ZoneZ);
            break;
        case SCENE_CMD_SCENE_CLEAR:
            ProcessSceneClear();
            break;
        default:
            break;
        }
    }

    // ── Entity batch async pipeline ───────────────────────────────────────────
    // Step 1: if a decode just finished, submit the result to the mesh (game thread only)
    if (EntityPendingVertCount > 0 && EntityDecodeTask.IsValid() && EntityDecodeTask.IsReady())
    {
        SubmitEntityMesh();
        EntityPendingVertCount = 0;
    }

    // Step 2: if idle, grab the next batch — copy fast, Commit immediately so Java
    // can write its next frame while we decode this one on a task thread
    if (EntityPendingVertCount == 0)
    {
        const int32* EntityData  = nullptr;
        int32        EntityCount = 0;
        if (FEntityBridge::Instance.Peek(EntityData, EntityCount) && EntityCount > 0)
        {
            // Raw copy to staging — game thread owns staging until task is launched
            EntityStagingBuffer.SetNumUninitialized(EntityCount, EAllowShrinking::No);
            FMemory::Memcpy(EntityStagingBuffer.GetData(), EntityData, EntityCount * sizeof(int32));
            FEntityBridge::Instance.Commit(); // Java unblocked here, not after decode

            EntityPendingVertCount = EntityCount / 6;
            EntityDecodeTask = Async(EAsyncExecution::ThreadPool, [this, EntityCount]()
            {
                DecodeEntityBatchAsync(EntityCount);
            });
        }
    }
}

// ── Vertex decode helpers ─────────────────────────────────────────────────────

static FORCEINLINE int16_t UnpackLo(int32_t V) { return static_cast<int16_t>(V & 0xffff); }

/**
 * Convert a packed RS HSL value to sRGB.
 * Only reads the low 15 bits — bits 16-23 are bias, bits 24-31 are alpha.
 */
static FColor HslToRgb(int32 Hsl)
{
    // Mask to low 16 bits: hue occupies bits 15-10 (6-bit), sat 9-7, lum 6-0.
    // Must keep bit 15 — stripping it corrupts hues >= 32 (purple, blue, etc.)
    // Bits 16+ are bias/alpha from (bias<<16)|hsl — safe to drop.
    Hsl &= 0xffff;

    // Match hsl_to_rgb.glsl exactly:
    //   hue = hsl.x / 64.0 + 0.0078125   (hsl.x = bits 14-10, 5-bit, 0-31)
    //   sat = hsl.y / 8.0  + 0.0625       (hsl.y = bits  9- 7, 3-bit, 0-7)
    //   lum = hsl.z                        (hsl.z = bits  6- 0, 7-bit, 0-127)
    const float Hue = float((Hsl >> 10) & 0x3f) / 64.f + 0.0078125f;
    const float Sat = float((Hsl >>  7) & 0x07) / 8.f  + 0.0625f;
    const float Lum = float( Hsl        & 0x7f) / 128.f;

    const float Q = Lum < 0.5f ? Lum * (1.f + Sat) : Lum + Sat - Lum * Sat;
    const float P = 2.f * Lum - Q;

    // Channels: R uses (hue + 1/3), G uses hue, B uses (hue - 1/3)
    auto HueChannel = [P, Q](float T) -> float
    {
        if (T > 1.f) T -= 1.f;
        if (T < 0.f) T += 1.f;
        if (6.f * T < 1.f) return P + (Q - P) * 6.f * T;
        if (2.f * T < 1.f) return Q;
        if (3.f * T < 2.f) return P + (Q - P) * (2.f / 3.f - T) * 6.f;
        return P;
    };

    const float R = HueChannel(Hue + 1.f / 3.f);
    const float G = HueChannel(Hue);
    const float B = HueChannel(Hue - 1.f / 3.f);

    // Store raw HSL lightness (0-127 → 0-255) in alpha so the material can use
    // it as a greyscale multiplier for textured faces, matching RuneLite's GLSL:
    //   "float light = float(hsl & 127) / 127.f"
    const uint8 LightAlpha = uint8(FMath::Clamp((Hsl & 0x7f) * (255.f / 127.f), 0.f, 255.f));

    return FColor(
        uint8(FMath::Clamp(R * 255.f, 0.f, 255.f)),
        uint8(FMath::Clamp(G * 255.f, 0.f, 255.f)),
        uint8(FMath::Clamp(B * 255.f, 0.f, 255.f)),
        LightAlpha);
}

static FVector DecodePosition(const int32_t* Data)
{
    const float lx = static_cast<float>(UnpackLo(Data[0]));
    const float lh = static_cast<float>(static_cast<int16_t>((static_cast<uint32_t>(Data[0]) >> 16) & 0xffff));
    const float lz = static_cast<float>(UnpackLo(Data[1]));

    return FVector(-lx * RS_TO_UE_SCALE, lz * RS_TO_UE_SCALE, -lh * RS_TO_UE_SCALE);
}

// ── Section builder ───────────────────────────────────────────────────────────

/**
 * Build a FProcMeshSection from a flat array of RS vertex ints.
 * IntCount must be a multiple of 5 (5 ints per vertex).
 *
 * UV channels:
 *   UV0 – (tu/256, tv/256)          texture coords
 *   UV1 – (tt, tf)                  1-based texture ID + flags
 *   UV2 – (bias, vertAlpha)         depth bias (0-255) + vertex opacity (0-1)
 */
FProcMeshSection ASceneGraphManager::BuildSection(const int32_t* Data, int32 IntCount, bool bSuppressAnim)
{
    FProcMeshSection Section;

    const int32 NVerts = IntCount / 5;
    const int32 NTris  = NVerts / 3;

    if (NTris <= 0) return Section;

    Section.ProcVertexBuffer.SetNumUninitialized(NVerts);
    Section.bEnableCollision = false;
    Section.bSectionVisible  = true;
    Section.SectionLocalBox  = FBox(EForceInit::ForceInitToZero);

    for (int32 v = 0; v < NVerts; ++v)
    {
        const int32_t* VD = Data + v * 5;
        FProcMeshVertex& Vert = Section.ProcVertexBuffer[v];

        Vert.Position = DecodePosition(VD);

        // UV0 – texture coords (signed 16-bit — projected UVs can be negative)
        // VD[3] = (tu << 16) | tt,  VD[4] = (tf << 16) | tv
        const float tu = static_cast<float>(static_cast<int16_t>((static_cast<uint32_t>(VD[3]) >> 16) & 0xffff));
        const float tv = static_cast<float>(static_cast<int16_t>(VD[4] & 0xffff));
        Vert.UV0 = FVector2D(tu / 256.f, tv / 256.f);

        // UV1 – texture ID (1-based) + flags (UV1.y=1 suppresses animation)
        const float tf = bSuppressAnim ? 1.f : float((static_cast<uint32_t>(VD[4]) >> 16) & 0xffff);
        Vert.UV1 = FVector2D(float(VD[3] & 0xffff), tf);

        // UV2 – depth bias (bits 23-16) + vertex alpha (bits 31-24)
        // bias:      0-255, used for Pixel Depth Offset in material
        // vertAlpha: 0=opaque, 255=fully transparent → invert to opacity
        const float bias      = float((static_cast<uint32_t>(VD[2]) >> 16) & 0xff);
        const float vertAlpha = float((static_cast<uint32_t>(VD[2]) >> 24) & 0xff) / 255.f;
        Vert.UV2 = FVector2D(bias, 1.f - vertAlpha); // UV2.Y = opacity (1=opaque, 0=transparent)

        // Vertex colour from RS HSL (low 15 bits only)
        Vert.Color   = HslToRgb(VD[2]);
        Vert.Normal  = FVector::UpVector; // filled in normal pass below
        Vert.Tangent = FProcMeshTangent(FVector(1.f, 0.f, 0.f), false);

        Section.SectionLocalBox += Vert.Position;
    }

    // Sequential indices
    Section.ProcIndexBuffer.Reserve(NTris * 3);
    for (int32 t = 0; t < NTris; ++t)
    {
        Section.ProcIndexBuffer.Add(t * 3 + 0);
        Section.ProcIndexBuffer.Add(t * 3 + 1);
        Section.ProcIndexBuffer.Add(t * 3 + 2);
    }

    // Flat normals — flip any that point downward so lighting is consistent
    for (int32 t = 0; t < NTris; ++t)
    {
        const int32 i = t * 3;
        const FVector& P0 = Section.ProcVertexBuffer[i + 0].Position;
        const FVector& P1 = Section.ProcVertexBuffer[i + 1].Position;
        const FVector& P2 = Section.ProcVertexBuffer[i + 2].Position;
        FVector N = FVector::CrossProduct(P1 - P0, P2 - P0).GetSafeNormal();
        if (N.Z < 0.f) N = -N;
        Section.ProcVertexBuffer[i + 0].Normal = N;
        Section.ProcVertexBuffer[i + 1].Normal = N;
        Section.ProcVertexBuffer[i + 2].Normal = N;
    }

    return Section;
}

// ── Texture upload ────────────────────────────────────────────────────────────

void ASceneGraphManager::ApplyMaterialParams(UMaterialInstanceDynamic* MID)
{
    if (!MID) return;
    MID->SetTextureParameterValue(TEXT("TextureArray"), ZoneTextureArray);
    MID->SetTextureParameterValue(TEXT("AnimSpeeds"),   AnimSpeedsTexture);
    MID->SetScalarParameterValue(TEXT("StaticLighting"), CachedStaticLighting);
}

void ASceneGraphManager::TryUploadTextures()
{
    if (!FTextureBridge::Instance.IsReady()) return;

    const uint8* Src = FTextureBridge::Instance.GetTextureData();
    if (!Src)
    {
        UE_LOG(LogTemp, Warning, TEXT("SceneGraphManager: texture bridge ready but data null"));
        return;
    }

    const int32 TotalBytes = (int32)(TEX_COUNT * TEX_SIZE * TEX_SIZE * 4);

    FTexturePlatformData* PlatData = new FTexturePlatformData();
    PlatData->SizeX = (int32)TEX_SIZE;
    PlatData->SizeY = (int32)TEX_SIZE;
    PlatData->SetNumSlices((int32)TEX_COUNT);
    PlatData->PixelFormat = PF_R8G8B8A8;

    FTexture2DMipMap* Mip = new FTexture2DMipMap((int32)TEX_SIZE, (int32)TEX_SIZE, (int32)TEX_COUNT);
    PlatData->Mips.Add(Mip);
    Mip->BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(Mip->BulkData.Realloc(TotalBytes), Src, TotalBytes);
    Mip->BulkData.Unlock();

    ZoneTextureArray = NewObject<UTexture2DArray>(this, NAME_None, RF_Transient);
    ZoneTextureArray->Filter      = TF_Bilinear;
    ZoneTextureArray->AddressX    = TA_Wrap;
    ZoneTextureArray->AddressY    = TA_Wrap;
    ZoneTextureArray->SRGB        = true;
    ZoneTextureArray->NeverStream = true;
    ZoneTextureArray->SetPlatformData(PlatData);
    ZoneTextureArray->UpdateResource();

    UE_LOG(LogTemp, Log, TEXT("SceneGraphManager: uploaded %u×%u×%u texture array"),
        TEX_SIZE, TEX_SIZE, TEX_COUNT);

    // Build 256×1 RGBA16F animation speeds lookup texture.
    // texel[id].rg = (animU, animV) pre-scaled to UV-units/sec.
    // Material formula:  UV += Time * Texture2DSample(AnimSpeeds, texID).rg
    {
        const float* AnimSpeeds = FTextureBridge::Instance.GetAnimSpeedData();

        FTexturePlatformData* AnimPD = new FTexturePlatformData();
        AnimPD->SizeX        = (int32)TEX_COUNT;
        AnimPD->SizeY        = 1;
        AnimPD->SetNumSlices(1);
        AnimPD->PixelFormat  = PF_FloatRGBA; // RGBA16F

        const int32 AnimMipBytes = (int32)(TEX_COUNT * 4 * sizeof(uint16)); // 4 channels × uint16
        FTexture2DMipMap* AnimMip = new FTexture2DMipMap((int32)TEX_COUNT, 1);
        AnimPD->Mips.Add(AnimMip);
        AnimMip->BulkData.Lock(LOCK_READ_WRITE);
        uint16* AnimDst = reinterpret_cast<uint16*>(AnimMip->BulkData.Realloc(AnimMipBytes));

        for (int32 i = 0; i < (int32)TEX_COUNT; ++i)
        {
            const float u = AnimSpeeds ? AnimSpeeds[i * 2 + 0] : 0.f;
            const float v = AnimSpeeds ? AnimSpeeds[i * 2 + 1] : 0.f;
            // Pack as float16 (FFloat16)
            AnimDst[i * 4 + 0] = FFloat16(u).Encoded;
            AnimDst[i * 4 + 1] = FFloat16(v).Encoded;
            AnimDst[i * 4 + 2] = 0;
            AnimDst[i * 4 + 3] = 0;
        }
        AnimMip->BulkData.Unlock();

        AnimSpeedsTexture = NewObject<UTexture2D>(this, NAME_None, RF_Transient);
        AnimSpeedsTexture->Filter      = TF_Nearest;
        AnimSpeedsTexture->AddressX    = TA_Wrap;
        AnimSpeedsTexture->AddressY    = TA_Wrap;
        AnimSpeedsTexture->SRGB        = false;
        AnimSpeedsTexture->NeverStream = true;
        AnimSpeedsTexture->SetPlatformData(AnimPD);
        AnimSpeedsTexture->UpdateResource();

        UE_LOG(LogTemp, Log, TEXT("SceneGraphManager: built AnimSpeeds lookup texture (%u texels)"), TEX_COUNT);
    }


    CachedStaticLighting = (float)CVarStaticLighting.GetValueOnGameThread();

    if (ZoneMaterial)
    {
        ZoneMaterialDynamic = UMaterialInstanceDynamic::Create(ZoneMaterial, this);
        ApplyMaterialParams(ZoneMaterialDynamic);

        for (auto& Pair : ZoneMeshes)
            if (Pair.Value) Pair.Value->SetMaterial(0, ZoneMaterialDynamic);
    }

    if (ZoneAlphaMaterial)
    {
        ZoneAlphaMaterialDynamic = UMaterialInstanceDynamic::Create(ZoneAlphaMaterial, this);
        ApplyMaterialParams(ZoneAlphaMaterialDynamic);

        for (auto& Pair : ZoneMeshes)
            if (Pair.Value) Pair.Value->SetMaterial(1, ZoneAlphaMaterialDynamic);
    }

    UE_LOG(LogTemp, Log, TEXT("SceneGraphManager: materials applied to %d zones"), ZoneMeshes.Num());
    bTexturesUploaded = true;
}

// ── Zone mesh building ────────────────────────────────────────────────────────

void ASceneGraphManager::ProcessZoneData(const FZonePacket& Packet)
{
    UE_LOG(LogTemp, Log, TEXT("SceneGraphManager: zone(%d,%d) opaque=%d alpha=%d"),
        Packet.ZoneX, Packet.ZoneZ, Packet.OpaqueIntCount / 5, Packet.AlphaIntCount / 5);

    const int32_t* Data = Packet.Data.GetData();

    // Get or create the ProceduralMeshComponent for this zone
    const int64 Key = ZoneKey(Packet.ZoneX, Packet.ZoneZ);
    UProceduralMeshComponent* Mesh = nullptr;
    if (UProceduralMeshComponent** Found = ZoneMeshes.Find(Key))
    {
        Mesh = *Found;
    }
    else
    {
        Mesh = NewObject<UProceduralMeshComponent>(this);
        Mesh->SetupAttachment(GetRootComponent());
        Mesh->RegisterComponent();

        const float ZoneWorldX = -(Packet.ZoneX - SCENE_OFFSET_ZONES) * 1024.f * RS_TO_UE_SCALE;
        const float ZoneWorldY = (Packet.ZoneZ - SCENE_OFFSET_ZONES) * 1024.f * RS_TO_UE_SCALE;
        Mesh->SetWorldLocation(FVector(ZoneWorldX, ZoneWorldY, 0.f));

        ZoneMeshes.Add(Key, Mesh);
    }

    UMaterialInterface* Mat      = ZoneMaterialDynamic      ? ZoneMaterialDynamic.Get()      : ZoneMaterial.Get();
    UMaterialInterface* AlphaMat = ZoneAlphaMaterialDynamic ? ZoneAlphaMaterialDynamic.Get() : ZoneAlphaMaterial.Get();

    // Section 0 — ground-level opaque geometry (0..RoofOffset)
    const int32 GroundInts = Packet.RoofOffset;
    if (GroundInts >= 5)
    {
        Mesh->SetProcMeshSection(0, BuildSection(Data, GroundInts));
        if (Mat) Mesh->SetMaterial(0, Mat);
    }

    // Section 1 — roof / upper-floor opaque geometry (RoofOffset..OpaqueIntCount)
    const int32 RoofInts = Packet.OpaqueIntCount - Packet.RoofOffset;
    if (RoofInts >= 5)
    {
        Mesh->SetProcMeshSection(1, BuildSection(Data + GroundInts, RoofInts, /*bSuppressAnim=*/true));
        if (Mat) Mesh->SetMaterial(1, Mat);
        Mesh->SetMeshSectionVisible(1, !bHideRoofs);
        ZoneRoofKeys.Add(Key);
    }
    else
    {
        // No roof data — clear any stale section from a previous upload and remove from set
        Mesh->ClearMeshSection(1);
        ZoneRoofKeys.Remove(Key);
    }

    // Section 2 — alpha (translucent) geometry
    if (Packet.AlphaIntCount >= 5)
    {
        Mesh->SetProcMeshSection(2, BuildSection(Data + Packet.OpaqueIntCount, Packet.AlphaIntCount));
        if (AlphaMat) Mesh->SetMaterial(2, AlphaMat);
    }
}

void ASceneGraphManager::ProcessZoneClear(int32 ZoneX, int32 ZoneZ)
{
    const int64 Key = ZoneKey(ZoneX, ZoneZ);
    ZoneRoofKeys.Remove(Key);
    if (UProceduralMeshComponent** Found = ZoneMeshes.Find(Key))
    {
        (*Found)->ClearAllMeshSections();
        (*Found)->DestroyComponent();
        ZoneMeshes.Remove(Key);
    }
}

void ASceneGraphManager::ProcessSceneClear()
{
    ZoneRoofKeys.Empty();
    for (auto& Pair : ZoneMeshes)
    {
        if (Pair.Value)
        {
            Pair.Value->ClearAllMeshSections();
            Pair.Value->DestroyComponent();
        }
    }
    ZoneMeshes.Empty();
}

// ── Entity batch ──────────────────────────────────────────────────────────────

/**
 * Decode raw entity ints from EntityStagingBuffer into OpaqueVertCache + AlphaVertCache.
 * Triangles with vertAlpha > 0 (bits 31-24 of VD[3]) go to the alpha section.
 * Runs on a task thread — must NOT touch game-thread-only objects (EntityMesh, etc.).
 */
void ASceneGraphManager::DecodeEntityBatchAsync(int32 IntCount)
{
    const int32    NTris = IntCount / 18; // 6 ints/vert × 3 verts/tri
    const int32_t* Data  = EntityStagingBuffer.GetData();

    // ── Pass 1: count opaque vs alpha triangles ───────────────────────────────
    int32 NOpaqueT = 0, NAlphaT = 0;
    for (int32 t = 0; t < NTris; ++t)
    {
        const uint32_t abhsl = static_cast<uint32_t>(Data[t * 18 + 3]); // VD[3] of first vert
        if ((abhsl >> 24) & 0xff) ++NAlphaT;
        else                      ++NOpaqueT;
    }

    EntityOpaqueVertCache.SetNumUninitialized(NOpaqueT * 3, EAllowShrinking::No);
    EntityAlphaVertCache .SetNumUninitialized(NAlphaT  * 3, EAllowShrinking::No);

    // ── Pass 2: decode and sort into the two caches ───────────────────────────
    int32 oOut = 0, aOut = 0;
    for (int32 t = 0; t < NTris; ++t)
    {
        const int32   srcV0  = t * 3;
        const bool    bAlpha = ((static_cast<uint32_t>(Data[srcV0 * 6 + 3]) >> 24) & 0xff) != 0;
        TArray<FProcMeshVertex>& Buf = bAlpha ? EntityAlphaVertCache : EntityOpaqueVertCache;
        int32&                   Out = bAlpha ? aOut                 : oOut;

        const int32 triStart = Out;
        for (int32 d = 0; d < 3; ++d)
        {
            const int32_t*   VD   = Data + (srcV0 + d) * 6;
            FProcMeshVertex& Vert = Buf[Out++];

            float wx, wy, wz;
            FMemory::Memcpy(&wx, &VD[0], sizeof(float));
            FMemory::Memcpy(&wy, &VD[1], sizeof(float));
            FMemory::Memcpy(&wz, &VD[2], sizeof(float));
            Vert.Position = FVector(-wx * RS_TO_UE_SCALE, wz * RS_TO_UE_SCALE, -wy * RS_TO_UE_SCALE);

            const float su = static_cast<float>(static_cast<int16_t>((static_cast<uint32_t>(VD[4]) >> 16) & 0xffff));
            const float sv = static_cast<float>(static_cast<int16_t>(VD[5] & 0xffff));
            Vert.UV0 = FVector2D(su / 256.f, sv / 256.f);
            Vert.UV1 = FVector2D(float(VD[4] & 0xffff), float((static_cast<uint32_t>(VD[5]) >> 16) & 0xffff));

            const float bias      = float((static_cast<uint32_t>(VD[3]) >> 16) & 0xff);
            const float vertAlpha = float((static_cast<uint32_t>(VD[3]) >> 24) & 0xff) / 255.f;
            Vert.UV2    = FVector2D(bias, 1.f - vertAlpha);
            Vert.Color  = HslToRgb(VD[3]);
            Vert.Tangent = FProcMeshTangent(FVector(1.f, 0.f, 0.f), false);
        }

        // Flat normal for this triangle
        const FVector N = FVector::CrossProduct(
            Buf[triStart+1].Position - Buf[triStart].Position,
            Buf[triStart+2].Position - Buf[triStart].Position
        ).GetSafeNormal();
        Buf[triStart].Normal = Buf[triStart+1].Normal = Buf[triStart+2].Normal = N;
    }

    EntityPendingOpaqueVerts = NOpaqueT * 3;
    EntityPendingAlphaVerts  = NAlphaT  * 3;
}

// Helper: build + upload one ProcMeshSection from a vert cache
static void UploadSection(UProceduralMeshComponent* Mesh, int32 SectionIdx,
                          TArray<FProcMeshVertex>& Verts, TArray<int32>& IdxCache,
                          UMaterialInterface* Mat)
{
    const int32 NVerts = Verts.Num();
    if (NVerts <= 0)
    {
        Mesh->ClearMeshSection(SectionIdx);
        return;
    }

    if (NVerts > IdxCache.Num())
    {
        const int32 Old = IdxCache.Num();
        IdxCache.SetNumUninitialized(NVerts, EAllowShrinking::No);
        for (int32 i = Old; i < NVerts; ++i) IdxCache[i] = i;
    }

    FBox Bounds(EForceInit::ForceInitToZero);
    for (const FProcMeshVertex& V : Verts) Bounds += V.Position;

    FProcMeshSection Section;
    Section.ProcVertexBuffer = Verts;
    Section.ProcIndexBuffer.SetNumUninitialized(NVerts, EAllowShrinking::No);
    FMemory::Memcpy(Section.ProcIndexBuffer.GetData(), IdxCache.GetData(), NVerts * sizeof(int32));
    Section.SectionLocalBox  = Bounds;
    Section.bEnableCollision = false;
    Section.bSectionVisible  = true;

    Mesh->SetProcMeshSection(SectionIdx, MoveTemp(Section));
    if (Mat) Mesh->SetMaterial(SectionIdx, Mat);
}

/**
 * Upload decoded opaque+alpha caches to EntityMesh sections 0 and 1.
 * Must run on the game thread.
 */
void ASceneGraphManager::SubmitEntityMesh()
{
    if (!EntityMesh)
    {
        EntityMesh = NewObject<UProceduralMeshComponent>(this);
        EntityMesh->SetupAttachment(GetRootComponent());
        EntityMesh->RegisterComponent();
        EntityMesh->SetWorldLocation(FVector::ZeroVector);
        EntityMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    UMaterialInterface* OpaqueMat = ZoneMaterialDynamic      ? ZoneMaterialDynamic.Get()      : ZoneMaterial.Get();
    UMaterialInterface* AlphaMat  = ZoneAlphaMaterialDynamic ? ZoneAlphaMaterialDynamic.Get() : ZoneAlphaMaterial.Get();

    UploadSection(EntityMesh, 0, EntityOpaqueVertCache, EntityOpaqueIdxCache, OpaqueMat);
    UploadSection(EntityMesh, 1, EntityAlphaVertCache,  EntityAlphaIdxCache,  AlphaMat);
}
