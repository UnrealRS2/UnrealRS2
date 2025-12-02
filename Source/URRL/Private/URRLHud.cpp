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

	if (const RLFrameBuffer& F = *FSharedMemoryBridge::RLFrameBufferPtr; F.ready)
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
			
			if (FSharedMemoryBridge::MouseMove->consumed)
			{
				FSharedMemoryBridge::MouseMove->x = lastX;
				FSharedMemoryBridge::MouseMove->y = lastY;
				FSharedMemoryBridge::MouseMove->consumed = false;
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

void UpdateFrameBufferTexture(UTexture2D* Texture, uint8* Pixels, int32 Width, int32 Height)
{
	if (!Texture || !Pixels)
		return;

	FUpdateTextureRegion2D Region(0, 0, 0, 0, Width, Height);

	// Unreal’s safe, async-friendly helper
	Texture->UpdateTextureRegions(
		0,        // mip index
		1,        // number of regions
		&Region,  // region(s)
		Width * 4, // source pitch (bytes per row)
		4,        // bytes per pixel
		Pixels    // source data
	);
}

void UpdateTexture(UTexture2D* Texture, const uint8* SrcData)
{
	if (!Texture || !SrcData) return;

	const int32 Width  = Texture->GetSizeX();
	const int32 Height = Texture->GetSizeY();

	FUpdateTextureRegion2D Region(0, 0, 0, 0, Width, Height);

	auto Data = MakeShared<TArray<uint8>, ESPMode::ThreadSafe>();
	Data->SetNumUninitialized(Width * Height * 4);
	FMemory::Memcpy(Data->GetData(), SrcData, Width * Height * 4);

	ENQUEUE_RENDER_COMMAND(UpdateTextureRegion)(
		[Texture, Region, Data](FRHICommandListImmediate& RHICmdList)
		{
			RHIUpdateTexture2D(
				Texture->GetResource()->GetTexture2DRHI(),
				0,
				Region,
				Texture->GetSizeX() * 4,
				Data->GetData()
			);
		}
	);
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

bool setMat = false;

UTexture2D* CreateFrameBufferTexture(int32 Width, int32 Height)
{
	UTexture2D* Tex = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
	Tex->SRGB = true;
	Tex->NeverStream = true;

	// Force the RHI resource to be created immediately
	Tex->AddToRoot(); // optional, prevent GC
	Tex->UpdateResource();

	return Tex;
}

void UpdateFrameBufferTexture(UTexture2D* Texture, const TArray<FColor>& Pixels)
{
	if (!Texture) return;

	const int32 Width  = Texture->GetSizeX();
	const int32 Height = Texture->GetSizeY();

	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);

	// Copy pixels directly
	FMemory::Memcpy(Data, Pixels.GetData(), Width * Height * sizeof(FColor));

	Mip.BulkData.Unlock();
	Texture->UpdateResource(); // <-- REQUIRED
}

float targetHeight, targetWidth;

bool AURRLHud::UpdateFromSharedMemory(const RLFrameBuffer* Info)
{
    if (!Info || !Info->pixels)
    {
        UE_LOG(LogTemp, Warning, TEXT("UpdateFromSharedMemory failed: Info or pixels null"));
        return false;
    }

    const int32 W = Info->width;
    const int32 H = Info->height;

    // --- Step 1: Create or resize the framebuffer texture ---
    if (!GDrawTexture || GDrawTexture->GetSizeX() != W || GDrawTexture->GetSizeY() != H)
    {
        GDrawTexture = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
        if (!GDrawTexture)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create GDrawTexture"));
            return false;
        }

        GDrawTexture->SRGB = true;
        GDrawTexture->NeverStream = true;
        GDrawTexture->AddToRoot(); // prevent GC
        GDrawTexture->UpdateResource();

        // Compute target size to fit viewport while keeping aspect ratio
        FVector2D ScreenSize;
        if (GEngine && GEngine->GameViewport)
        {
            GEngine->GameViewport->GetViewportSize(ScreenSize);

            float widthRatio  = ScreenSize.X / W;
            float heightRatio = ScreenSize.Y / H;

            if (widthRatio > heightRatio)
            {
                targetWidth  = W * widthRatio;
                targetHeight = H * widthRatio;
            }
            else
            {
                targetWidth  = W * heightRatio;
                targetHeight = H * heightRatio;
            }
        }
    }

    // --- Step 2: Copy pixels safely ---
    if (GDrawTexture->GetPlatformData() && GDrawTexture->GetPlatformData()->Mips.Num() > 0)
    {
        FTexture2DMipMap& Mip = GDrawTexture->GetPlatformData()->Mips[0];
        void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
        if (Data)
        {
            FMemory::Memcpy(Data, Info->pixels, W * H * 4);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GDrawTexture Mip BulkData lock failed"));
            Mip.BulkData.Unlock();
            return false;
        }
        Mip.BulkData.Unlock();
        GDrawTexture->UpdateResource();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("GDrawTexture PlatformData invalid or no Mips"));
        return false;
    }

    // --- Step 3: Update UMG Image only if valid ---
    if (GFrameBufferImage && GDrawTexture)
    {
        GFrameBufferImage->SetBrushFromTexture(GDrawTexture, true);
        GFrameBufferImage->SetDesiredSizeOverride(FVector2D(targetWidth, targetHeight));
        GFrameBufferImage->SetColorAndOpacity(FLinearColor::White);
    }

    // --- Step 4: Mark shared memory as consumed ---
    const_cast<RLFrameBuffer*>(Info)->ready = false;
    const_cast<RLFrameBuffer*>(Info)->consumed = true;

    ConsumeCounter++;

    return true;
}





