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

	// Mouse buttons (forwarded to RuneLite)
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

	/** When true, WASD + mouse controls the camera instead of syncing from RuneLite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Free Roam")
	bool bFreeRoam = false;

	/** Movement speed in UE cm/s while in free roam. */
	UPROPERTY(EditAnywhere, Category="Free Roam")
	float FreeRoamSpeed = 2000.f;

	/** Mouse look sensitivity (degrees per pixel). */
	UPROPERTY(EditAnywhere, Category="Free Roam")
	float FreeRoamMouseSensitivity = 0.2f;

private:
	FVector  FreeRoamLocation;
	FRotator FreeRoamRotation;
};
