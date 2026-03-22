// MyHUD.cpp

//Win32 BULLSHIT
#define NOMINMAX

#include "URRLHud.h"
#include "CanvasItem.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Engine/Canvas.h"

//Win32 BULLSHIT
#ifdef _WIN32
#undef UpdateResource
#endif

UTexture2D* GDrawTexture = nullptr;
UImage* GFrameBufferImage = nullptr;
USizeBox* GFrameBufferSizeBox = nullptr;

void AURRLHud::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	if (!FSharedMemoryBridge::RLFrameBufferPtr) return;

	if (RLFrameBuffer& F = *FSharedMemoryBridge::RLFrameBufferPtr;
		std::atomic_ref<bool>(F.ready).load(std::memory_order_acquire))
		UpdateFromSharedMemory(FSharedMemoryBridge::RLFrameBufferPtr);
	
	FVector2D ScreenPosition(50, 50);
	FVector2D ScreenPosition2(50, 75);
	FVector2D ScreenPosition3(50, 100);
	FVector2D ScreenPosition4(50, 125);
	FVector2D ScreenPosition5(50, 150);
	FCanvasTextItem TextItem(ScreenPosition, FText::FromString(CameraDebugText), GEngine->GetMediumFont(), FLinearColor::Green);
	FCanvasTextItem TextItem2(ScreenPosition2, FText::FromString(FrontFrameInfoDebugText), GEngine->GetMediumFont(), FLinearColor::Green);
	FCanvasTextItem TextItem3(ScreenPosition3, FText::FromString(BackFrameInfoDebugText), GEngine->GetMediumFont(), FLinearColor::Green);
	FCanvasTextItem TextItem4(ScreenPosition4, FText::FromString(ConsumesPerSecondText), GEngine->GetMediumFont(), FLinearColor::Green);
	FCanvasTextItem TextItem5(ScreenPosition5, FText::FromString(DebugText), GEngine->GetMediumFont(), FLinearColor::Green);
	TextItem.EnableShadow(FLinearColor::Black);
	TextItem2.EnableShadow(FLinearColor::Black);
	TextItem3.EnableShadow(FLinearColor::Black);
	TextItem4.EnableShadow(FLinearColor::Black);
	TextItem5.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(TextItem);
	Canvas->DrawItem(TextItem2);
	Canvas->DrawItem(TextItem3);
	Canvas->DrawItem(TextItem4);
	Canvas->DrawItem(TextItem5);
}

void AURRLHud::UpdateCameraDebug(const FString& NewText)
{
	CameraDebugText = NewText;
}

void AURRLHud::UpdateFrontFrameInfoDebug(const FString& NewText)
{
	FrontFrameInfoDebugText = NewText;
}

void AURRLHud::UpdateBackFrameInfoDebug(const FString& NewText)
{
	BackFrameInfoDebugText = NewText;
}

void AURRLHud::UpdateConsumesPerSecondDebug(const FString& NewText)
{
	ConsumesPerSecondText = NewText;
}

void AURRLHud::UpdateDebugText(const FString& NewText)
{
	DebugText = NewText;
}

int lastX = -1;
int lastY = -1;
bool moveUpdate = false;

void AURRLHud::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (APlayerController* PC = GetOwningPlayerController())
	{
		float X, Y;
		PC->GetMousePosition(X, Y);
		if (lastX != static_cast<int>(X) || lastY != static_cast<int>(Y))
			moveUpdate = true;
		
		if (moveUpdate)
		{
			lastX = X;
			lastY = Y;
			
			if (std::atomic_ref<bool>(FSharedMemoryBridge::MouseMove->consumed).load(std::memory_order_acquire))
			{
				FSharedMemoryBridge::MouseMove->x = lastX;
				FSharedMemoryBridge::MouseMove->y = lastY;
				std::atomic_ref<bool>(FSharedMemoryBridge::MouseMove->consumed).store(false, std::memory_order_release);
			}
		}
		
		// Optional: debug print
		// GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::Green, FString::Printf(TEXT("MouseX: %f, MouseY: %f"), X, Y));
	}
	
	CPSAccumTime += DeltaSeconds;
	if (CPSAccumTime >= 1.0f) // 1 second has passed
	{
		LastCPS = ConsumeCounter;
		ConsumeCounter = 0;
		CPSAccumTime -= 1.0f;
        
		// Optional: update a debug string for HUD
		UpdateConsumesPerSecondDebug(FString::Printf(TEXT("CPS: %d"), LastCPS));
	}
}


void AURRLHud::BeginPlay()
{
	Super::BeginPlay();
	
	if (APlayerController* PC = GEngine->GetFirstLocalPlayerController(GWorld))
	{
		UClass* FrameBufferClass = LoadClass<UUserWidget>(
	nullptr,
	TEXT("/Game/Framebuffer.Framebuffer_C")
		);

		if (!FrameBufferClass)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load Framebuffer Blueprint class"));
			return;
		}
		
		
		UUserWidget* Widget = CreateWidget<UUserWidget>(PC, FrameBufferClass);
		if (!Widget)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to create Framebuffer widget"));
			return;
		}
		
		Widget->AddToViewport();
				
		GFrameBufferSizeBox = Cast<USizeBox>(Widget->GetWidgetFromName(TEXT("FrameBufferSizeBox")));
		
		// Grab the Image widget manually
		GFrameBufferImage = Cast<UImage>(Widget->GetWidgetFromName(TEXT("FrameBufferImage")));

		if (GFrameBufferImage)
		{
			UpdateDebugText(FString::Printf(TEXT("Acquired %s"), *Widget->GetName()));
		}
	}
}

float targetHeight, targetWidth;

bool AURRLHud::UpdateFromSharedMemory(RLFrameBuffer* Info)
{
    if (!Info) return false;

    const int32 W = Info->width;
    const int32 H = Info->height;

    if (W <= 0 || H <= 0) return false;

    // --- Step 1: Create or resize texture (only when dimensions change) ---
    if (!GDrawTexture || GDrawTexture->GetSizeX() != W || GDrawTexture->GetSizeY() != H)
    {
        if (GDrawTexture)
            GDrawTexture->RemoveFromRoot();

        GDrawTexture = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
        if (!GDrawTexture)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create GDrawTexture"));
            return false;
        }

        GDrawTexture->SRGB = true;
        GDrawTexture->NeverStream = true;
        GDrawTexture->AddToRoot();
        GDrawTexture->UpdateResource(); // create GPU resource once

        // Letterbox: fit frame inside viewport preserving aspect ratio
        if (GEngine && GEngine->GameViewport)
        {
            FVector2D ScreenSize;
            GEngine->GameViewport->GetViewportSize(ScreenSize);
            const float Ratio = FMath::Min(ScreenSize.X / W, ScreenSize.Y / H);
            targetWidth  = W * Ratio;
            targetHeight = H * Ratio;
        }

        // Bind widget to this texture (only on create/resize, not every frame)
        if (GFrameBufferImage)
        {
            GFrameBufferImage->SetBrushFromTexture(GDrawTexture, true);
            GFrameBufferImage->SetDesiredSizeOverride(FVector2D(targetWidth, targetHeight));
            GFrameBufferImage->SetColorAndOpacity(FLinearColor::White);
        }
    }

    // --- Step 2: Claim this frame so the game thread won't re-process it next tick ---
    std::atomic_ref<bool>(Info->ready).store(false, std::memory_order_relaxed);

    // --- Step 3: Upload pixels on the render thread using the raw shared-memory pointer ---
    // No memcpy needed: Java checks consumed before writing, and consumed stays false
    // until the render command sets it true after RHIUpdateTexture2D completes.
    const uint8* Pixels = Info->pixels;
    UTexture2D* Tex = GDrawTexture;
    const FUpdateTextureRegion2D Region(0, 0, 0, 0, W, H);
    const uint32 Pitch = static_cast<uint32>(W) * 4;

    ENQUEUE_RENDER_COMMAND(UploadFrameBuffer)(
        [Tex, Region, Pitch, Pixels, Info](FRHICommandListImmediate& RHICmdList)
        {
            FTextureResource* Resource = Tex->GetResource();
            if (Resource && Resource->GetTexture2DRHI())
            {
                RHIUpdateTexture2D(Resource->GetTexture2DRHI(), 0, Region, Pitch, Pixels);
            }
            // Release: Java may now write the next frame
            std::atomic_ref<bool>(Info->consumed).store(true, std::memory_order_release);
        }
    );

    ConsumeCounter++;
    return true;
}





