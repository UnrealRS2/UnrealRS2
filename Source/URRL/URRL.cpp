// Copyright Epic Games, Inc. All Rights Reserved.

#include "URRL.h"

#include "FShimTick.h"
#include "URRLGameMode.h"
#include "SharedMemoryBridge.h"
#include "SceneGraphBridge.h"
#include "SceneGraphManager.h"
#include "TextureBridge.h"
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "URRLHud.h"
#include "Engine/Engine.h"

FSharedMemoryBridge FSharedMemoryBridge::SharedMemoryBridge{};
FSceneGraphBridge   FSceneGraphBridge::Instance{};
FTextureBridge      FTextureBridge::Instance{};
void*           FSharedMemoryBridge::Raw             = nullptr;
RLCameraStatus* FSharedMemoryBridge::RLCameraStatusPtr = nullptr;
RLFrameBuffer*  FSharedMemoryBridge::RLFrameBufferPtr  = nullptr;
SResolution*    FSharedMemoryBridge::Resolution        = nullptr;
SMouseMove*     FSharedMemoryBridge::MouseMove         = nullptr;
SMousePress*    FSharedMemoryBridge::MousePress        = nullptr;
SMouseRelease*  FSharedMemoryBridge::MouseRelease      = nullptr;
SMouseWheel*    FSharedMemoryBridge::MouseWheel        = nullptr;
SKeyQueue*      FSharedMemoryBridge::KeyQueue          = nullptr;
URRL_API AURRLHud* AURRLGameMode::URRLHud;
static FShimTick GCameraTick;

// ── Helpers ───────────────────────────────────────────────────────────────────

static void InitOnWorld(UWorld* World)
{
	if (FSceneGraphBridge::Instance.IsInitialized())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("URRL: initialising on game world '%s'"), *World->GetName());

	FSharedMemoryBridge::SharedMemoryBridge.Init("URRL");
	FSceneGraphBridge::Instance.Init("URRL_Scene");
	FEntityBridge::Instance.Init("URRL_Entities");
	FTextureBridge::Instance.Init("URRL_Textures");

	// Spawn the actor that polls zone packets and builds meshes
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<ASceneGraphManager>(
		ASceneGraphManager::StaticClass(),
		FVector::ZeroVector, FRotator::ZeroRotator, Params);

	UE_LOG(LogTemp, Log, TEXT("URRL: init complete"));

	// Defer the HUD lookup one tick because the PlayerController may not exist yet
	World->GetTimerManager().SetTimerForNextTick([World]()
	{
		if (GEngine && GEngine->GameViewport)
		{
			if (APlayerController* PC = World->GetFirstPlayerController())
			{
				AURRLGameMode::URRLHud = Cast<AURRLHud>(PC->GetHUD());
			}
		}
	});
}

// ── Module ────────────────────────────────────────────────────────────────────

void FURRLModule::StartupModule()
{
	FWorldDelegates::OnPostWorldInitialization.AddLambda([](UWorld* World, const UWorld::InitializationValues)
	{
		if (World->IsGameWorld())
		{
			InitOnWorld(World);
		}
	});

	// When the game world ends (PIE stop or game quit), tear down the bridge so
	// the next Play session re-initialises cleanly.
	FWorldDelegates::OnWorldCleanup.AddLambda([](UWorld* World, bool /*bSessionEnded*/, bool /*bCleanupResources*/)
	{
		if (World->IsGameWorld() && FSceneGraphBridge::Instance.IsInitialized())
		{
			FSceneGraphBridge::Instance.Shutdown();
			FEntityBridge::Instance.Shutdown();
			FTextureBridge::Instance.Shutdown();
			UE_LOG(LogTemp, Log, TEXT("URRL: bridges shut down on world cleanup"));
		}
	});

	// Handle Live Coding reloads: the module re-runs StartupModule after OnPostWorldInitialization
	// has already fired for the current game world, so check for an existing game world here.
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* W = Context.World();
			if (W && W->IsGameWorld())
			{
				InitOnWorld(W);
				break;
			}
		}
	}
}

void FURRLModule::ShutdownModule()
{
	FSceneGraphBridge::Instance.Shutdown();
	FEntityBridge::Instance.Shutdown();
	FTextureBridge::Instance.Shutdown();
}



IMPLEMENT_PRIMARY_GAME_MODULE( FURRLModule, URRL, "URRL" );
