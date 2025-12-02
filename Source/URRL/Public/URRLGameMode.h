// MyGameMode.h
#pragma once

//Win32 BULLSHIT
#define NOMINMAX

#include "CoreMinimal.h"
#include "URRLHud.h"
#include "GameFramework/GameModeBase.h"
#include "URRLGameMode.generated.h"

UCLASS()
class URRL_API AURRLGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AURRLGameMode();
	static AURRLHud* URRLHud;
	
protected:
	virtual void BeginPlay() override;
};
