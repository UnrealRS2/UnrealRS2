#include "URRLPawn.h"

#include "SharedMemoryBridge.h"
#include "SceneGraphBridge.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"

// ─── Key forwarder ────────────────────────────────────────────────────────────
// Intercepts all keyboard events via Slate and pushes them into the SHM key
// queue so Java/RuneLite can consume them.

static TCHAR GetTypedChar(uint32 Win32VK)
{
	BYTE KeyState[256] = {};
	if (!GetKeyboardState(KeyState)) return 0;
	WCHAR Buf[4] = {};
	const int N = ToUnicodeEx(Win32VK,
	                          MapVirtualKeyW(Win32VK, MAPVK_VK_TO_VSC),
	                          KeyState, Buf, 4, 0,
	                          GetKeyboardLayout(0));
	return (N == 1) ? (TCHAR)Buf[0] : 0;
}

static int32 Win32ToJavaVK(uint32 Win32VK)
{
	switch (Win32VK)
	{
		case 0x0D: return 10;   // VK_RETURN  → Java VK_ENTER
		case 0x2E: return 127;  // VK_DELETE  → Java VK_DELETE
		default:   return (int32)Win32VK;
	}
}

static void EnqueueKey(int32 Id, int32 JavaVK, int32 KeyChar, int32 Mods)
{
	SKeyQueue* Q = FSharedMemoryBridge::KeyQueue;
	if (!Q) return;

	const int32 WH = std::atomic_ref<int32>(Q->writeHead).load(std::memory_order_relaxed);
	const int32 RH = std::atomic_ref<int32>(Q->readHead ).load(std::memory_order_acquire);
	if ((WH - RH) >= KEY_QUEUE_CAPACITY) return; // ring full – drop

	SKeyEvent& E = Q->events[WH & (KEY_QUEUE_CAPACITY - 1)];
	E.id        = Id;
	E.keyCode   = JavaVK;
	E.keyChar   = KeyChar;
	E.modifiers = Mods;

	std::atomic_ref<int32>(Q->writeHead).store(WH + 1, std::memory_order_release);
}

class FKeyForwarder : public IInputProcessor
{
public:
	virtual void Tick(const float /*DeltaTime*/, FSlateApplication& /*SlateApp*/,
	                  TSharedRef<ICursor> /*Cursor*/) override {}

	virtual bool HandleKeyDownEvent(FSlateApplication& /*SlateApp*/,
	                                const FKeyEvent& Evt) override
	{
		if (!FSharedMemoryBridge::KeyQueue) return false;

		const int32  Mods   = BuildMods(Evt.GetModifierKeys());
		const uint32 Win32  = Evt.GetKeyCode();
		const int32  JavaVK = Win32ToJavaVK(Win32);
		const TCHAR  Ch     = GetTypedChar(Win32);

		// KEY_PRESSED (401)
		EnqueueKey(401, JavaVK, 0xFFFF, Mods);

		// KEY_TYPED (400) for printable characters
		if (Ch >= 0x20 && Ch != 0x7F) // exclude control chars
		{
			EnqueueKey(400, 0, (int32)Ch, Mods);
		}

		return false; // don't consume – UE still needs to process bindings
	}

	virtual bool HandleKeyUpEvent(FSlateApplication& /*SlateApp*/,
	                              const FKeyEvent& Evt) override
	{
		if (!FSharedMemoryBridge::KeyQueue) return false;

		const int32  Mods   = BuildMods(Evt.GetModifierKeys());
		const uint32 Win32  = Evt.GetKeyCode();
		const int32  JavaVK = Win32ToJavaVK(Win32);

		// KEY_RELEASED (402)
		EnqueueKey(402, JavaVK, 0xFFFF, Mods);
		return false;
	}

private:
	static int32 BuildMods(const FModifierKeysState& M)
	{
		int32 Out = 0;
		if (M.IsShiftDown())   Out |= 64;   // Java InputEvent.SHIFT_MASK
		if (M.IsControlDown()) Out |= 128;  // Java InputEvent.CTRL_MASK
		if (M.IsAltDown())     Out |= 512;  // Java InputEvent.ALT_MASK
		return Out;
	}
};

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

	// Register keyboard forwarder with Slate so all key events reach RuneLite.
	KeyForwarder = MakeShared<FKeyForwarder>();
	FSlateApplication::Get().RegisterInputPreProcessor(KeyForwarder);
}

void AURRLPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (KeyForwarder && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(KeyForwarder);
	}
	KeyForwarder.Reset();
	Super::EndPlay(EndPlayReason);
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
	PlayerInputComponent->BindKey(EKeys::MouseScrollUp,   IE_Pressed,  this, &AURRLPawn::OnScrollUp);
	PlayerInputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed,  this, &AURRLPawn::OnScrollDown);
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

void AURRLPawn::OnScrollUp()
{
	if (FSharedMemoryBridge::MouseWheel)
		std::atomic_ref<int32>(FSharedMemoryBridge::MouseWheel->accum).fetch_add(-1, std::memory_order_relaxed);
}

void AURRLPawn::OnScrollDown()
{
	if (FSharedMemoryBridge::MouseWheel)
		std::atomic_ref<int32>(FSharedMemoryBridge::MouseWheel->accum).fetch_add(1, std::memory_order_relaxed);
}
