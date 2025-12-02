// MyGameMode.cpp
#include "URRLGameMode.h"

#include "URRLHud.h"
#include "SharedMemoryBridge.h"
#include "GameFramework/GameUserSettings.h"
#include "CoreMinimal.h"
#include "URRLPawn.h"
#include "GameFramework/Pawn.h"

void* FSharedMemoryBridge::Raw = nullptr;

AURRLGameMode::AURRLGameMode()
{
	HUDClass = AURRLHud::StaticClass();
	DefaultPawnClass = AURRLPawn::StaticClass();
}

void AURRLGameMode::BeginPlay()
{
	Super::BeginPlay();
	
	if (UGameUserSettings* Settings = GEngine->GetGameUserSettings())
	{
		Settings->SetOverallScalabilityLevel(1); // 0=Low, 1=Medium, 2=High, 3=Epic
		
		// Windowed mode
		Settings->SetFullscreenMode(EWindowMode::Windowed);
		Settings->SetScreenResolution(FIntPoint(1920, 1080));
		
		// VSync / frame limit
		Settings->SetVSyncEnabled(false);
		Settings->SetFrameRateLimit(60.0f);

		// Optional: disable motion blur, ambient occlusion, etc.
		Settings->SetAntiAliasingQuality(0);     // 0 = Off
		Settings->SetPostProcessingQuality(0);   // Off
		Settings->SetShadowQuality(0);           // Off
		Settings->SetTextureQuality(1);          // Medium
		Settings->SetFoliageQuality(0);          // Off
		
		// Apply immediately (but don't save to disk if you want temporary)
		Settings->ApplySettings(false);
	}
}
