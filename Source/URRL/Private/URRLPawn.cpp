#include "URRLPawn.h"

#include "SharedMemoryBridge.h"
#include "SceneGraphBridge.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AURRLPawn::AURRLPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessPlayer = EAutoReceiveInput::Player0;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetWorldLocation(FVector::ZeroVector);
	Camera->SetWorldRotation(FRotator::ZeroRotator);
}

void AURRLPawn::BeginPlay()
{
	Super::BeginPlay();

	// Seed free roam position from wherever the pawn was placed in the level.
	FreeRoamLocation = GetActorLocation();
	FreeRoamRotation = GetActorRotation();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (bFreeRoam)
		{
			// Capture mouse for look; cursor hidden while in free roam.
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}
		else
		{
			PC->bShowMouseCursor = true;
			PC->bEnableClickEvents = true;
			PC->bEnableMouseOverEvents = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);
		}
		EnableInput(PC);
	}
}

void AURRLPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bFreeRoam)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (!PC) return;

		// ── Mouse look ────────────────────────────────────────────────────────
		float MouseDX = 0.f, MouseDY = 0.f;
		PC->GetInputMouseDelta(MouseDX, MouseDY);

		FreeRoamRotation.Yaw   += MouseDX * FreeRoamMouseSensitivity;
		FreeRoamRotation.Pitch  = FMath::Clamp(
			FreeRoamRotation.Pitch - MouseDY * FreeRoamMouseSensitivity, -89.f, 89.f);
		FreeRoamRotation.Roll = 0.f;

		// ── WASD + Q/E movement ───────────────────────────────────────────────
		const float Fwd = (PC->IsInputKeyDown(EKeys::W) ? 1.f : 0.f)
		                - (PC->IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
		const float Rt  = (PC->IsInputKeyDown(EKeys::D) ? 1.f : 0.f)
		                - (PC->IsInputKeyDown(EKeys::A) ? 1.f : 0.f);
		const float Up  = (PC->IsInputKeyDown(EKeys::E) ? 1.f : 0.f)
		                - (PC->IsInputKeyDown(EKeys::Q) ? 1.f : 0.f);

		// Shift to sprint
		const float SpeedMult = PC->IsInputKeyDown(EKeys::LeftShift) ? 4.f : 1.f;

		const FRotationMatrix RotMat(FreeRoamRotation);
		const FVector FwdVec = RotMat.GetScaledAxis(EAxis::X);
		const FVector RtVec  = RotMat.GetScaledAxis(EAxis::Y);

		FreeRoamLocation += (FwdVec * Fwd + RtVec * Rt + FVector::UpVector * Up)
		                  * FreeRoamSpeed * SpeedMult * DeltaTime;

		Camera->SetWorldLocation(FreeRoamLocation);
		Camera->SetWorldRotation(FreeRoamRotation);
	}
	else if (FSharedMemoryBridge::RLCameraStatusPtr)
	{
		const RLCameraStatus& C = *FSharedMemoryBridge::RLCameraStatusPtr;

		// Shim sends (getCameraX, getCameraZ, getCameraY) → struct (x, y, z).
		// So C.y = RS south (Z), C.z = RS height (Y).
		// Height is negated to match tile/entity convention (-lh, -wy → positive UE Z).
		FVector NewLocation(-C.x * RS_TO_UE_SCALE, C.y * RS_TO_UE_SCALE, -C.z * RS_TO_UE_SCALE);

		// Note: shim struct field names are swapped — C.pitch stores RS yaw, C.yaw stores RS pitch.
		const float RsYawDeg   = C.pitch * (360.f / 2048.f);
		const float RsPitchDeg = C.yaw   * (360.f / 2048.f);

		// RS pitch increases looking downward → negate for UE (positive = up).
		// RS yaw 0 = south (+Y in UE), UE yaw 0 = +X → add 90° offset.
		// RS zoom (scale) → horizontal FOV: fov_x = 2 * atan(viewportW / (2 * scale))
		// Uses client.getViewportWidth/Height() (the actual 3D projection dimensions),
		// not the framebuffer buffer dimensions which can differ in stretched/HiDPI modes.
		const float Zoom = FMath::Max(1.f, static_cast<float>(C.scale)) * 0.57f; // tweak this

		const float fovX = 2.f * FMath::Atan(static_cast<float>(C.viewportW) / (2.f * Zoom));
		const float aspect = static_cast<float>(C.viewportW) / static_cast<float>(C.viewportH);
		const float fovY = 2.f * FMath::Atan(FMath::Tan(fovX * 0.5f) / aspect);

		Camera->FieldOfView = FMath::RadiansToDegrees(fovY);

		Camera->SetWorldLocation(NewLocation);
		Camera->SetWorldRotation(FRotator(-RsPitchDeg, 90.f - RsYawDeg, 0.f));
	}
}

void AURRLPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindKey(EKeys::LeftMouseButton,   IE_Pressed,  this, &AURRLPawn::OnLeftClick);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton,   IE_Released, this, &AURRLPawn::OnLeftRelease);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton,  IE_Pressed,  this, &AURRLPawn::OnRightClick);
	PlayerInputComponent->BindKey(EKeys::RightMouseButton,  IE_Released, this, &AURRLPawn::OnRightRelease);
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed,  this, &AURRLPawn::OnMidClick);
	PlayerInputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &AURRLPawn::OnMidRelease);
}

void AURRLPawn::OnLeftClick()
{
	FSharedMemoryBridge::MousePress->button = 1;
	std::atomic_ref<bool>(FSharedMemoryBridge::MousePress->consumed).store(false, std::memory_order_release);
}

void AURRLPawn::OnLeftRelease()
{
	FSharedMemoryBridge::MouseRelease->button = 1;
	std::atomic_ref<bool>(FSharedMemoryBridge::MouseRelease->consumed).store(false, std::memory_order_release);
}

void AURRLPawn::OnRightClick()
{
	FSharedMemoryBridge::MousePress->button = 3;
	std::atomic_ref<bool>(FSharedMemoryBridge::MousePress->consumed).store(false, std::memory_order_release);
}

void AURRLPawn::OnRightRelease()
{
	FSharedMemoryBridge::MouseRelease->button = 3;
	std::atomic_ref<bool>(FSharedMemoryBridge::MouseRelease->consumed).store(false, std::memory_order_release);
}

void AURRLPawn::OnMidClick()
{
	FSharedMemoryBridge::MousePress->button = 2;
	std::atomic_ref<bool>(FSharedMemoryBridge::MousePress->consumed).store(false, std::memory_order_release);
}

void AURRLPawn::OnMidRelease()
{
	FSharedMemoryBridge::MouseRelease->button = 2;
	std::atomic_ref<bool>(FSharedMemoryBridge::MouseRelease->consumed).store(false, std::memory_order_release);
}
