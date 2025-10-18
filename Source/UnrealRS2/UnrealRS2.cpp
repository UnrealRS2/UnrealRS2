// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealRS2.h"

#include "api.h"
#include "bridge.h"
#include "Blueprint/UserWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
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

static UTexture2D* GDrawTexture = nullptr;

UImage* GFrameBufferImage = nullptr;

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
				GDrawTexture = UTexture2D::CreateTransient(msg.w, msg.h, PF_B8G8R8A8);
				GDrawTexture->AddToRoot(); // prevent GC
				GDrawTexture->SRGB = false;
			}
			
			// Update texture pixels
			FTexture2DMipMap& Mip = GDrawTexture->GetPlatformData()->Mips[0];
			void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(Data, msg.pixels, msg.w * msg.h * 4);
			Mip.BulkData.Unlock();
			GDrawTexture->UpdateResource();

			// Update UMG Image brush
			if (GFrameBufferImage && GDrawTexture)
			{
				FSlateBrush Brush;
				Brush.SetResourceObject(GDrawTexture);
				Brush.ImageSize = FVector2D(msg.w, msg.h);
				GFrameBufferImage->SetBrush(Brush);
			}
		});
	});
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

