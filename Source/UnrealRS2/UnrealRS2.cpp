// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealRS2.h"

#include "api.h"
#include "bridge.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Modules/ModuleManager.h"
#include "Microsoft/AllowMicrosoftPlatformTypes.h"

bool started = false;

void InitDebugScreenCallback()
{
	// Set debug print callback for client
	SetDebugScreenCallback([](const char* Message)
	{
		if (!Message) return;
		// Always forward to game thread
		AsyncTask(ENamedThreads::GameThread, [msg = FString(Message)]()
		{
			if (GEngine && GEngine->GameViewport)
			{
				//log
				UE_LOG(LogTemp, Verbose, TEXT("%s"), *msg);
				//screen
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, msg);
			}
		});
	});
}

void InitDebugConsoleCallback()
{
	// Set debug print callback for client
	SetDebugConsoleCallback([](const char* Message)
	{
		if (!Message) return;
		// Always forward to game thread
		AsyncTask(ENamedThreads::GameThread, [msg = FString(Message)]()
		{
			if (GEngine && GEngine->GameViewport)
			{
				//log
				UE_LOG(LogTemp, Verbose, TEXT("%s"), *msg);
				//console message
				if (APlayerController* PC = GEngine->GetFirstLocalPlayerController(GWorld))
				{
					PC->ClientMessage(msg);
				}
			}
		});
	});
}

struct DrawFinishedX
{
	uint32_t* pixels;
	int w;
	int h;
	DrawFinishedX();
	DrawFinishedX(uint32_t* p, int width, int height)
	: pixels(p), w(width), h(height)
	{}
};

UTexture2D* GDrawTexture = nullptr;

UImage* GFrameBufferImage = nullptr;
USizeBox* GFrameBufferSizeBox = nullptr;
float targetHeight, targetWidth;

// Call this once at setup
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

void InitDrawFinishedCallback()
{
	SetDrawFinishedCallback([](uint32_t* pixels, int w, int h)
	{
		// Always forward to game thread
		AsyncTask(ENamedThreads::GameThread, [msg = DrawFinishedX(pixels, w, h)]()
		{
			if (!GDrawTexture ||
				GDrawTexture->GetSizeX() != msg.w ||
				GDrawTexture->GetSizeY() != msg.h)
			{
				GDrawTexture = CreateFrameBufferTexture(msg.w, msg.h);

				FVector2D Size;
				GEngine->GameViewport->GetViewportSize(Size);

				float widthRatio = Size.X / msg.w;
				float heightRatio = Size.Y / msg.h;


				if (widthRatio > heightRatio)
				{
					targetWidth = msg.w * widthRatio;
					targetHeight = msg.h * widthRatio;
				}
				else
				{
					targetWidth = msg.w * heightRatio;
					targetHeight = msg.h * heightRatio;
				}
			}
			// TODO: UpdateFrameBufferTexture(GDrawTexture, msg.pixels, msg.w, msg.h);
			// Update texture pixels
			FTexture2DMipMap& Mip = GDrawTexture->GetPlatformData()->Mips[0];
			void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(Data, msg.pixels, msg.w * msg.h * 4);
			Mip.BulkData.Unlock();
			GDrawTexture->UpdateResource();

			// Update UMG Image brush
			if (GFrameBufferImage && GDrawTexture)
			{
				GFrameBufferImage->SetDesiredSizeOverride(FVector2D(targetWidth, targetHeight));
				FSlateBrush Brush;
				Brush.SetResourceObject(GDrawTexture);
				Brush.ImageSize = FVector2D(msg.w, msg.h);
				Brush.Tiling = ESlateBrushTileType::NoTile;
				GFrameBufferImage->SetBrush(Brush);
				GFrameBufferImage->SetColorAndOpacity(FLinearColor::White);
			}
		});
	});
}

void UpdateTexture(UTexture2D* Texture, const uint8* SrcData, int32 SrcPitch)
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

void InitCallbacks()
{
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
			PC->ClientMessage(FString::Printf(TEXT("Acquired %s"), *Widget->GetName()));
		}
	}
	InitDebugScreenCallback();
	InitDebugConsoleCallback();
	InitDrawFinishedCallback();
}

	void FUnrealRS2Module::StartupModule()
	{
		// Run once when the first world is ready
		FWorldDelegates::OnPostWorldInitialization.AddLambda([](UWorld* World, const UWorld::InitializationValues)
		{
			// Trigger only on standalone builds (not in editor)
			if (!World || World->WorldType != EWorldType::Game)
				return;


			// Wait one tick so the viewport and player controller exist
			World->GetTimerManager().SetTimerForNextTick([World]()
			{
				if (!started)
				{
					InitCallbacks();
					started = true;
				}
				
				APlayerController* PC = World->GetFirstPlayerController();
				if (PC && PC->GetHUD())
				{
					rs2_start_client();
				}
			});
		});
	}



	void FUnrealRS2Module::ShutdownModule()
	{
		// Optional: cleanup if needed
	}



IMPLEMENT_PRIMARY_GAME_MODULE( FUnrealRS2Module, UnrealRS2, "UnrealRS2" );

