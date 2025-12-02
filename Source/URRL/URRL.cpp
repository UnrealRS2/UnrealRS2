// Copyright Epic Games, Inc. All Rights Reserved.

#include "URRL.h"

#include "FShimTick.h"
#include "URRLGameMode.h"
#include "SharedMemoryBridge.h"
#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "URRLHud.h"

bool started = false;
FSharedMemoryBridge FSharedMemoryBridge::SharedMemoryBridge{};
RLCameraStatus* FSharedMemoryBridge::RLCameraStatusPtr;
RLFrameBuffer* FSharedMemoryBridge::RLFrameBufferPtr;
SResolution* FSharedMemoryBridge::Resolution;
SMouseMove* FSharedMemoryBridge::MouseMove;
SMousePress* FSharedMemoryBridge::MousePress;
SMouseRelease* FSharedMemoryBridge::MouseRelease;
URRL_API AURRLHud* AURRLGameMode::URRLHud;
static FShimTick GCameraTick;

void FURRLModule::StartupModule()
{
	// Run once when the first world is ready
	FWorldDelegates::OnPostWorldInitialization.AddLambda([](UWorld* World, const UWorld::InitializationValues)
	{
		// Wait one tick so the viewport and player controller exist
		World->GetTimerManager().SetTimerForNextTick([World]()
		{
			if (!started)
			{
				AsyncTask(ENamedThreads::GameThread, [msg = FString("[INIT]: "), World]()
									{
										if (GEngine && GEngine->GameViewport)
										{
											//log
											UE_LOG(LogTemp, Verbose, TEXT("%s"), *msg);

											APlayerController* PC = World->GetFirstPlayerController();
											AURRLGameMode::URRLHud = Cast<AURRLHud>(PC->GetHUD());
										}
									});
				started = true;
				FSharedMemoryBridge::SharedMemoryBridge.Init("URRL");

			}
		});
	});
}

void FURRLModule::ShutdownModule()
{
	// Optional: cleanup if needed
}



IMPLEMENT_PRIMARY_GAME_MODULE( FURRLModule, URRL, "URRL" );
