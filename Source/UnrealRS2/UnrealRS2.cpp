// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealRS2.h"

#include "bridge.h"
#include "Modules/ModuleManager.h"
#include "Microsoft/AllowMicrosoftPlatformTypes.h"

// Define the static members
UTexture2D* FUnrealRS2Module::GClientTexture = nullptr;
int32 FUnrealRS2Module::CurrentDrawColor = 0;



bool started = false;

static void Unreal_SetColor(int color)
{
	FUnrealRS2Module::CurrentDrawColor = color;
}

static void Unreal_DrawRect(int x, int y, int w, int h)
{
	if (!FUnrealRS2Module::GClientTexture)
	{
		UE_LOG(LogTemp, Warning, TEXT("DrawRect called before texture initialized!"));
		return;
	}

	FTexture2DMipMap& Mip = FUnrealRS2Module::GClientTexture->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	uint32* Pixels = static_cast<uint32*>(Data);

	const int TexWidth  = 765;
	const int TexHeight = 503;

	// Bounds check
	if (x < 0 || y < 0 || x + w > TexWidth || y + h > TexHeight)
	{
		UE_LOG(LogTemp, Warning, TEXT("DrawRect out of bounds: (%d,%d,%d,%d)"), x, y, w, h);
		Mip.BulkData.Unlock();
		return;
	}

	// Draw borders using the client's 0x00RRGGBB format directly
	const uint32 Color = static_cast<uint32>(FUnrealRS2Module::CurrentDrawColor) | 0xFF000000; // ensure alpha=255

	for (int i = 0; i < w; i++)
	{
		Pixels[(y * TexWidth) + (x + i)] = Color;                  // Top
		Pixels[((y + h - 1) * TexWidth) + (x + i)] = Color;        // Bottom
	}

	for (int i = 0; i < h; i++)
	{
		Pixels[((y + i) * TexWidth) + x] = Color;                  // Left
		Pixels[((y + i) * TexWidth) + (x + w - 1)] = Color;        // Right
	}

	Mip.BulkData.Unlock();
	FUnrealRS2Module::GClientTexture->UpdateResource();
}

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

	void FUnrealRS2Module::StartupModule()
	{
		if (!started)
		{
			Unreal_InitTexture();
			//RegisterUnrealPlatformCallbacks();
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
					GEngine->AddOnScreenDebugMessage(-1,  120.f, FColor::Green, FString("Starting RS2"));
					rs2_start_client();
					GEngine->AddOnScreenDebugMessage(-1,  120.f, FColor::Green, FString("RS2 Loaded"));
				}
			});
		});
	}

	void FUnrealRS2Module::ShutdownModule()
	{
		// Optional: cleanup if needed
	}



IMPLEMENT_PRIMARY_GAME_MODULE( FUnrealRS2Module, UnrealRS2, "UnrealRS2" );

