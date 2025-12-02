#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "URRLPawn.generated.h"

UCLASS()
class URRL_API AURRLPawn : public APawn
{
	GENERATED_BODY()

public:
	AURRLPawn();

protected:
	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// Mouse buttons
	void OnLeftClick();
	void OnLeftRelease();

	void OnRightClick();
	void OnRightRelease();
	
	void OnMidClick();
	void OnMidRelease();

public:
	virtual void Tick(float DeltaTime) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	UCameraComponent* Camera;
};
