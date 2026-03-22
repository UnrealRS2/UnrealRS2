#pragma once

#define NOMINMAX
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2DArray.h"
#include "ProceduralMeshComponent.h"
#include "SceneGraphManager.generated.h"

/**
 * ASceneGraphManager – polls the "URRL_Scene" shared memory each tick and
 * materialises RuneLite zone geometry as UProceduralMeshComponents.
 *
 * Each zone gets one ProceduralMeshComponent with two sections:
 *   Section 0 – opaque geometry  (Masked material,      M_RSZone)
 *   Section 1 – alpha geometry   (Translucent material, M_RSZoneAlpha)
 *
 * Vertex UV channels:
 *   UV0 – texture coords (tu/256, tv/256)
 *   UV1 – (tt: 1-based texture ID,  tf: texture flags)
 *   UV2 – (bias: depth-offset 0-255, vertAlpha: 0-1 opacity)
 */
UCLASS()
class URRL_API ASceneGraphManager : public AActor
{
    GENERATED_BODY()

public:
    ASceneGraphManager();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    /** One ProceduralMeshComponent per zone, keyed by ZoneKey(). */
    UPROPERTY()
    TMap<int64, UProceduralMeshComponent*> ZoneMeshes;

    /** Opaque/masked material — auto-loaded from /Game/Materials/M_RSZone. */
    UPROPERTY(EditAnywhere, Category="Rendering")
    TObjectPtr<UMaterialInterface> ZoneMaterial;

    /** Translucent material for alpha geometry — auto-loaded from /Game/Materials/M_RSZoneAlpha. */
    UPROPERTY(EditAnywhere, Category="Rendering")
    TObjectPtr<UMaterialInterface> ZoneAlphaMaterial;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> ZoneMaterialDynamic;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> ZoneAlphaMaterialDynamic;

    UPROPERTY()
    TObjectPtr<UTexture2DArray> ZoneTextureArray;

    bool  bTexturesUploaded    = false;
    float CachedStaticLighting = -1.f;

    void TryUploadTextures();
    void ApplyMaterialParams(UMaterialInstanceDynamic* MID);

    void ProcessZoneData(const struct FZonePacket& Packet);
    void ProcessSceneClear();
    void ProcessZoneClear(int32 ZoneX, int32 ZoneZ);

    static FProcMeshSection BuildSection(const int32_t* Data, int32 IntCount);
    static int64 ZoneKey(int32 X, int32 Z) { return (static_cast<int64>(X) << 32) | static_cast<uint32>(Z); }
};
