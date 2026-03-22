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
}

// ── Vertex decode helpers ─────────────────────────────────────────────────────

static FORCEINLINE int16_t UnpackLo(int32_t V) { return static_cast<int16_t>(V & 0xffff); }

/**
 * Convert a packed RS HSL value to sRGB.
 * Only reads the low 15 bits — bits 16-23 are bias, bits 24-31 are alpha.
 */
static FColor HslToRgb(int32 Hsl)
{
    // Mask to low 15 bits (hsl only)
    Hsl &= 0x7fff;

    const float H = float((Hsl >> 10) & 0x3f) / 64.f;
    const float S = float((Hsl >>  7) & 0x07) / 8.f;
    const float L = float( Hsl        & 0x7f) / 128.f;

    auto Hue2Rgb = [](float p, float q, float t) -> float
    {
        if (t < 0.f) t += 1.f;
        if (t > 1.f) t -= 1.f;
        if (t < 1.f / 6.f) return p + (q - p) * 6.f * t;
        if (t < 1.f / 2.f) return q;
        if (t < 2.f / 3.f) return p + (q - p) * (2.f / 3.f - t) * 6.f;
        return p;
    };

    float R, G, B;
    if (S < 1e-6f)
    {
        R = G = B = L;
    }
    else
    {
        const float Q = L < 0.5f ? L * (1.f + S) : L + S - L * S;
        const float P = 2.f * L - Q;
        R = Hue2Rgb(P, Q, H + 1.f / 3.f);
        G = Hue2Rgb(P, Q, H);
        B = Hue2Rgb(P, Q, H - 1.f / 3.f);
    }

    return FColor(
        uint8(FMath::Clamp(R * 255.f, 0.f, 255.f)),
        uint8(FMath::Clamp(G * 255.f, 0.f, 255.f)),
        uint8(FMath::Clamp(B * 255.f, 0.f, 255.f)),
        255);
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

        // UV0 – texture coords
        // VD[3] = (tu << 16) | tt,  VD[4] = (tf << 16) | tv
        const float tu = static_cast<float>((static_cast<uint32_t>(VD[3]) >> 16) & 0xffff);
        const float tv = static_cast<float>(VD[4] & 0xffff);
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
