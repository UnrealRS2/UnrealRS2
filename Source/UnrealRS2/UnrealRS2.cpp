// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealRS2.h"
#include "Modules/ModuleManager.h"
#include "client_377.h"
#include "Microsoft/AllowMicrosoftPlatformTypes.h"

const class FUnrealRS2Module : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// Run once when the first world is ready
		FWorldDelegates::OnPostWorldInitialization.AddLambda([](UWorld* World, const UWorld::InitializationValues)
		{
			// Trigger only on standalone builds (not in editor)
			if (!World || World->WorldType != EWorldType::Game)
				return;

			// Wait one tick so the viewport and player controller exist
			World->GetTimerManager().SetTimerForNextTick([World]()
			{
				APlayerController* PC = World->GetFirstPlayerController();
				if (PC && PC->GetHUD())
				{
					// Set debug print callback for client
					SetPrintCallback([](const char* message)
					{
						if (GEngine && GEngine->GameViewport)
						{
							GEngine->AddOnScreenDebugMessage(-1, 240.f, FColor::Green, FString(message));
						}
					});

					// test print callback / dll functionality
					Init();
				}
			});
		});
	}

	virtual void ShutdownModule() override
	{
		// Optional: cleanup if needed
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE( FUnrealRS2Module, UnrealRS2, "UnrealRS2" );

