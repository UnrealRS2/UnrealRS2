//Win32 BULLSHIT
#define NOMINMAX

#include "FShimTick.h"

#include "URRLGameMode.h"
#include "SharedMemoryBridge.h"
#include "URRLHud.h"
#include "Engine/Engine.h"

void FShimTick::Tick(float DeltaTime)
{
	if (FSharedMemoryBridge::RLCameraStatusPtr && FSharedMemoryBridge::RLFrameBufferPtr)
	{
		const RLCameraStatus& C = *FSharedMemoryBridge::RLCameraStatusPtr;
		const RLFrameBuffer& F = *FSharedMemoryBridge::RLFrameBufferPtr;
	
		FString CameraText = FString::Printf(TEXT("Camera: X=%d Y=%d Z=%d Pitch=%d Yaw=%d Scale=%d"),
										 C.x,
										 C.y,
										 C.z,
										 C.pitch,
										 C.yaw,
										 C.scale);
		
		FString FText = FString::Printf(TEXT("FrameBuffer: W=%d H=%d Ready=%d Consumed=%d"),
								 F.width,
								 F.height,
								 F.ready,
								 F.consumed);
		if (AURRLGameMode::URRLHud)
		{
			AURRLGameMode::URRLHud->UpdateCameraDebug(*CameraText);
			AURRLGameMode::URRLHud->UpdateFrontFrameInfoDebug(*FText);
		}
	}
}
