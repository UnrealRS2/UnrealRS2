#include "URRLPawn.h"

#include "SharedMemoryBridge.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AURRLPawn::AURRLPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	// Create camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetWorldLocation(FVector::ZeroVector);
	Camera->SetWorldRotation(FRotator::ZeroRotator);

	// Disable movement entirely
	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AURRLPawn::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Enable mouse and input
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
		FInputModeGameAndUI InputMode = FInputModeGameAndUI();
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(InputMode);
		EnableInput(PC); // Forces the pawn to bind to this controller
	}
}

void AURRLPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (FSharedMemoryBridge::RLCameraStatusPtr)
	{
		const RLCameraStatus& C = *FSharedMemoryBridge::RLCameraStatusPtr;
		
		// Convert RuneLite coordinates to Unreal coordinates if needed
		FVector NewLocation(C.x, C.y, C.z); // adjust axes swap if RL X/Y != Unreal X/Y
     
		float PitchDegrees = C.pitch * (360.f / 2048.f);
		float YawDegrees   = C.yaw * (360.f / 2048.f);

		FRotator NewRotation(YawDegrees, PitchDegrees, 0.f);
		

		Camera->SetWorldLocation(NewLocation);
		Camera->SetWorldRotation(NewRotation);
	}
}

void AURRLPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Action bindings
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AURRLPawn::OnLeftClick);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AURRLPawn::OnLeftRelease);

	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AURRLPawn::OnRightClick);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AURRLPawn::OnRightRelease);
	
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &AURRLPawn::OnMidClick);
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &AURRLPawn::OnMidRelease);
}

void AURRLPawn::OnLeftClick()
{
	FSharedMemoryBridge::MousePress->button = 1;
	FSharedMemoryBridge::MousePress->consumed = false;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, TEXT("Left click"));
}

void AURRLPawn::OnLeftRelease()
{
	FSharedMemoryBridge::MouseRelease->button = 1;
	FSharedMemoryBridge::MouseRelease->consumed = false;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Green, TEXT("Left release"));
}

void AURRLPawn::OnRightClick()
{
	FSharedMemoryBridge::MousePress->button = 3;
	FSharedMemoryBridge::MousePress->consumed = false;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("Right click"));
}

void AURRLPawn::OnRightRelease()
{
	FSharedMemoryBridge::MouseRelease->button = 3;
	FSharedMemoryBridge::MouseRelease->consumed = false;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Blue, TEXT("Right release"));
}

void AURRLPawn::OnMidClick()
{
	FSharedMemoryBridge::MousePress->button = 2;
	FSharedMemoryBridge::MousePress->consumed = false;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Orange, TEXT("Mid click"));
}

void AURRLPawn::OnMidRelease()
{
	FSharedMemoryBridge::MouseRelease->button = 2;
	FSharedMemoryBridge::MouseRelease->consumed = false;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Orange, TEXT("Mid release"));
}
