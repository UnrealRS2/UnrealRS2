// MyHUD.h
#pragma once

//Win32 BULLSHIT
#define NOMINMAX

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SharedMemoryBridge.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "URRLHud.generated.h"


UCLASS()
class URRL_API AURRLHud : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	void UpdateCameraDebug(const FString& NewText);
	void UpdateFrontFrameInfoDebug(const FString& NewText);
	void UpdateBackFrameInfoDebug(const FString& NewText);
	void UpdateConsumesPerSecondDebug(const FString& NewText);
	void UpdateDebugText(const FString& NewText);
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	bool UpdateFromSharedMemory(const RLFrameBuffer* Info);

private:
	FString CameraDebugText = TEXT("Initializing...");
	FString FrontFrameInfoDebugText = TEXT("Initializing...");
	FString BackFrameInfoDebugText = TEXT("Initializing...");
	FString ConsumesPerSecondText = TEXT("Initializing...");
	FString DebugText = TEXT("Initializing...");
	int32 ConsumeCounter = 0;        // counts frames consumed
	int32 LastCPS = 0;               // last calculated CPS
	float CPSAccumTime = 0.0f;       // accumulator for timing
};
