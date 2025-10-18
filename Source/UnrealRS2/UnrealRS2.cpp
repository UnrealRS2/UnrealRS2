// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealRS2.h"

#include "api.h"
#include "bridge.h"
#include "Modules/ModuleManager.h"
#include "Microsoft/AllowMicrosoftPlatformTypes.h"

// Define the static members
UTexture2D* FUnrealRS2Module::GClientTexture = nullptr;
int32 FUnrealRS2Module::CurrentDrawColor = 0;

bool started = false;

void RegisterUnrealPlatformCallbacks()
{
}
	
void Unreal_InitTexture()
{
	if (FUnrealRS2Module::GClientTexture)
		return; // Already initialized

	FUnrealRS2Module::GClientTexture = UTexture2D::CreateTransient(765, 503, PF_R8G8B8A8); // No pixel conversion needed
	FUnrealRS2Module::GClientTexture->AddToRoot(); // Prevent GC from deleting it
	FUnrealRS2Module::GClientTexture->Filter = TF_Nearest; // TODO: offer options
	FUnrealRS2Module::GClientTexture->SRGB = false;

	// Fill with black initially
	FTexture2DMipMap& Mip = FUnrealRS2Module::GClientTexture->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memset(Data, 0, 765 * 503 * 4);
	Mip.BulkData.Unlock();
	FUnrealRS2Module::GClientTexture->UpdateResource();
}

void InitDebugScreenCallback()
{
	// Set debug print callback for client
	SetDebugScreenCallback([](const char* Message)
	{
		if (!Message) return;
		// Always forward to game thread
		AsyncTask(ENamedThreads::GameThread, [msg = FString(Message)]()
		{
			if (GEngine && GEngine->GameViewport)
			{
				//log
				UE_LOG(LogTemp, Verbose, TEXT("%s"), *msg);
				//screen
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, msg);
			}
		});
	});
}

void InitDebugConsoleCallback()
{
	// Set debug print callback for client
	SetDebugConsoleCallback([](const char* Message)
	{
		if (!Message) return;
		// Always forward to game thread
		AsyncTask(ENamedThreads::GameThread, [msg = FString(Message)]()
		{
			if (GEngine && GEngine->GameViewport)
			{
				//log
				UE_LOG(LogTemp, Verbose, TEXT("%s"), *msg);
				//console message
				if (APlayerController* PC = GEngine->GetFirstLocalPlayerController(GWorld))
				{
					PC->ClientMessage(msg);
				}
			}
		});
	});
}

void InitCallbacks()
{
	InitDebugScreenCallback();
	InitDebugConsoleCallback();
}

	void FUnrealRS2Module::StartupModule()
	{
		if (!started)
		{
			InitCallbacks();
			started = true;
		}
		
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
					rs2_start_client();
				}
			});
		});
	}



	void FUnrealRS2Module::ShutdownModule()
	{
		// Optional: cleanup if needed
	}



IMPLEMENT_PRIMARY_GAME_MODULE( FUnrealRS2Module, UnrealRS2, "UnrealRS2" );

