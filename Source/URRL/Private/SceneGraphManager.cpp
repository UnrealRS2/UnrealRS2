#define NOMINMAX
#include "SceneGraphManager.h"
#include "SceneGraphBridge.h"
#include "TextureBridge.h"
#include "Engine/Texture.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<int32> CVarStaticLighting(
    TEXT("urrl.StaticLighting"),
    1,
    TEXT("0 = UE dynamic lighting, 1 = static/unlit (vanilla OSRS look)"),
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

    if (!bTexturesUploaded)
    {
        TryUploadTextures();
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

    // Poll dedicated entity batch SHM (separate from zone ring — no size limit issues)
    {
        const int32* EntityData  = nullptr;
        int32        EntityCount = 0;
        if (FEntityBridge::Instance.Poll(EntityData, EntityCount))
        {
            FZonePacket EntityPacket;
            EntityPacket.Command        = SCENE_CMD_ENTITY_BATCH;
            EntityPacket.OpaqueIntCount = EntityCount;
            EntityPacket.AlphaIntCount  = 0;
            if (EntityCount > 0)
            {
                EntityPacket.Data.SetNumUninitialized(EntityCount);
                FMemory::Memcpy(EntityPacket.Data.GetData(), EntityData, EntityCount * sizeof(int32));
            }
            ProcessEntityBatch(EntityPacket);
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
FProcMeshSection ASceneGraphManager::BuildSection(const int32_t* Data, int32 IntCount)
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

        // UV1 – texture ID (1-based) + flags
        Vert.UV1 = FVector2D(
            float(VD[3] & 0xffff),
            float((static_cast<uint32_t>(VD[4]) >> 16) & 0xffff));

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

    // Section 0 — opaque geometry
    if (Packet.OpaqueIntCount >= 5)
    {
        Mesh->SetProcMeshSection(0, BuildSection(Data, Packet.OpaqueIntCount));

        UMaterialInterface* Mat = ZoneMaterialDynamic ? ZoneMaterialDynamic.Get() : ZoneMaterial.Get();
        if (Mat) Mesh->SetMaterial(0, Mat);
    }

    // Section 1 — alpha (translucent) geometry
    if (Packet.AlphaIntCount >= 5)
    {
        Mesh->SetProcMeshSection(1, BuildSection(Data + Packet.OpaqueIntCount, Packet.AlphaIntCount));

        UMaterialInterface* AlphaMat = ZoneAlphaMaterialDynamic ? ZoneAlphaMaterialDynamic.Get() : ZoneAlphaMaterial.Get();
        if (AlphaMat) Mesh->SetMaterial(1, AlphaMat);
    }
}

void ASceneGraphManager::ProcessZoneClear(int32 ZoneX, int32 ZoneZ)
{
    const int64 Key = ZoneKey(ZoneX, ZoneZ);
    if (UProceduralMeshComponent** Found = ZoneMeshes.Find(Key))
    {
        (*Found)->ClearAllMeshSections();
        (*Found)->DestroyComponent();
        ZoneMeshes.Remove(Key);
    }
}

void ASceneGraphManager::ProcessSceneClear()
{
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
 * Decode a full-frame entity geometry batch.
 *
 * Vertex format (6 ints per vertex, putfff4 + put2222 from SceneUploader):
 *   [0] floatBits(worldX)    [1] floatBits(worldY/height)  [2] floatBits(worldZ)
 *   [3] abhsl                [4] (su<<16)|texture           [5] (tf<<16)|sv
 *
 * Coordinate conversion (scene-local RS units → UE cm):
 *   UE X = -worldX * RS_TO_UE_SCALE
 *   UE Y =  worldZ * RS_TO_UE_SCALE
 *   UE Z = -worldY * RS_TO_UE_SCALE   (RS Y is height, negative = above ground)
 */
FProcMeshSection ASceneGraphManager::BuildEntitySection(const int32_t* Data, int32 IntCount)
{
    FProcMeshSection Section;

    const int32 NVerts = IntCount / 6;
    const int32 NTris  = NVerts / 3;

    if (NTris <= 0) return Section;

    Section.ProcVertexBuffer.SetNumUninitialized(NVerts);
    Section.bEnableCollision = false;
    Section.bSectionVisible  = true;
    Section.SectionLocalBox  = FBox(EForceInit::ForceInitToZero);

    for (int32 v = 0; v < NVerts; ++v)
    {
        const int32_t* VD = Data + v * 6;
        FProcMeshVertex& Vert = Section.ProcVertexBuffer[v];

        // Float world-space positions
        float wx, wy, wz;
        FMemory::Memcpy(&wx, &VD[0], sizeof(float));
        FMemory::Memcpy(&wy, &VD[1], sizeof(float));
        FMemory::Memcpy(&wz, &VD[2], sizeof(float));

        Vert.Position = FVector(-wx * RS_TO_UE_SCALE, wz * RS_TO_UE_SCALE, -wy * RS_TO_UE_SCALE);

        // UV0 – texture coords (signed 16-bit — projected UVs can be negative)
        // VD[4]: Lo=texture, Hi=su  |  VD[5]: Lo=sv, Hi=tf
        const float su = static_cast<float>(static_cast<int16_t>((static_cast<uint32_t>(VD[4]) >> 16) & 0xffff));
        const float sv = static_cast<float>(static_cast<int16_t>(VD[5] & 0xffff));
        Vert.UV0 = FVector2D(su / 256.f, sv / 256.f);

        // UV1 – texture ID (1-based) + flags
        Vert.UV1 = FVector2D(float(VD[4] & 0xffff), float((static_cast<uint32_t>(VD[5]) >> 16) & 0xffff));

        // UV2 – depth bias + vertex opacity  (same bit layout as zone VD[2]/abhsl)
        const float bias      = float((static_cast<uint32_t>(VD[3]) >> 16) & 0xff);
        const float vertAlpha = float((static_cast<uint32_t>(VD[3]) >> 24) & 0xff) / 255.f;
        Vert.UV2 = FVector2D(bias, 1.f - vertAlpha);

        Vert.Color   = HslToRgb(VD[3]);
        Vert.Normal  = FVector::UpVector;
        Vert.Tangent = FProcMeshTangent(FVector(1.f, 0.f, 0.f), false);

        Section.SectionLocalBox += Vert.Position;
    }

    Section.ProcIndexBuffer.Reserve(NTris * 3);
    for (int32 t = 0; t < NTris; ++t)
    {
        Section.ProcIndexBuffer.Add(t * 3 + 0);
        Section.ProcIndexBuffer.Add(t * 3 + 1);
        Section.ProcIndexBuffer.Add(t * 3 + 2);
    }

    // Flat normals (no downward flip — entity faces can legitimately face any direction)
    for (int32 t = 0; t < NTris; ++t)
    {
        const int32 i = t * 3;
        const FVector& P0 = Section.ProcVertexBuffer[i + 0].Position;
        const FVector& P1 = Section.ProcVertexBuffer[i + 1].Position;
        const FVector& P2 = Section.ProcVertexBuffer[i + 2].Position;
        const FVector N = FVector::CrossProduct(P1 - P0, P2 - P0).GetSafeNormal();
        Section.ProcVertexBuffer[i + 0].Normal = N;
        Section.ProcVertexBuffer[i + 1].Normal = N;
        Section.ProcVertexBuffer[i + 2].Normal = N;
    }

    return Section;
}

void ASceneGraphManager::ProcessEntityBatch(const FZonePacket& Packet)
{
    // Create the entity mesh once
    if (!EntityMesh)
    {
        EntityMesh = NewObject<UProceduralMeshComponent>(this);
        EntityMesh->SetupAttachment(GetRootComponent());
        EntityMesh->RegisterComponent();
        EntityMesh->SetWorldLocation(FVector::ZeroVector);
    }

    if (Packet.OpaqueIntCount >= 6)
    {
        EntityMesh->SetProcMeshSection(0, BuildEntitySection(Packet.Data.GetData(), Packet.OpaqueIntCount));

        UMaterialInterface* Mat = ZoneMaterialDynamic ? ZoneMaterialDynamic.Get() : ZoneMaterial.Get();
        if (Mat) EntityMesh->SetMaterial(0, Mat);
    }
    else
    {
        EntityMesh->ClearAllMeshSections();
    }
}
