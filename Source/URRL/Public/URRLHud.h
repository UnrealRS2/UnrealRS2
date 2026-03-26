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
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	bool UpdateFromSharedMemory(RLFrameBuffer* Info);
};
