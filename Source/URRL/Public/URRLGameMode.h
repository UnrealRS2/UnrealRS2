// MyGameMode.h
#pragma once

//Win32 BULLSHIT
#define NOMINMAX

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "URRLGameMode.generated.h"

UCLASS()
class URRL_API AURRLGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AURRLGameMode();

protected:
	virtual void BeginPlay() override;
};
