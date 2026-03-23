#pragma once

#define NOMINMAX
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2DArray.h"
#include "ProceduralMeshComponent.h"
#include "Async/Future.h"
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
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
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
    /** Zone geometry: 5 ints/vertex, packed 16-bit integer local positions. */
    static FProcMeshSection BuildSection(const int32_t* Data, int32 IntCount);

    static int64 ZoneKey(int32 X, int32 Z) { return (static_cast<int64>(X) << 32) | static_cast<uint32>(Z); }

    // ── Entity batch — async decode pipeline ─────────────────────────────────
    //
    // Flow each tick:
    //   1. If decode task is done: SubmitEntityMesh() on game thread
    //   2. If idle: Peek SHM → memcpy to staging → Commit (unblocks Java) → dispatch decode task
    //
    // The async task only writes EntityVertCache. The game thread only reads it
    // inside SubmitEntityMesh, which runs before the next task is dispatched.

    /** Single mesh rebuilt each time a decode completes. */
    UPROPERTY()
    TObjectPtr<UProceduralMeshComponent> EntityMesh;

    /** Raw int copy of SHM data — written by game thread, read by async decode task. */
    TArray<int32>           EntityStagingBuffer;

    /** Decoded opaque/alpha vertex buffers — written by async task, read by game thread. */
    TArray<FProcMeshVertex> EntityOpaqueVertCache;
    TArray<FProcMeshVertex> EntityAlphaVertCache;

    /** Sequential index buffers (0,1,2,...) — grow-only. */
    TArray<int32>           EntityOpaqueIdxCache;
    TArray<int32>           EntityAlphaIdxCache;

    /** In-flight async decode. Valid when EntityPendingVertCount > 0. */
    TFuture<void>           EntityDecodeTask;

    /** Total verts in pending result (> 0 means task result is waiting to submit). */
    int32                   EntityPendingVertCount    = 0;
    /** Opaque/alpha split counts set by the async task, read after IsReady(). */
    int32                   EntityPendingOpaqueVerts  = 0;
    int32                   EntityPendingAlphaVerts   = 0;

    /** Decode raw ints from EntityStagingBuffer into Opaque/Alpha vert caches. Runs on task thread. */
    void DecodeEntityBatchAsync(int32 IntCount);

    /** Upload decoded caches to EntityMesh sections 0 (opaque) and 1 (alpha). Runs on game thread. */
    void SubmitEntityMesh();
};
