#pragma once
#include <stdint.h>
#ifdef __cplusplus
#include "Engine/Engine.h"        // for GEngine
#include "GameFramework/PlayerController.h" // for APlayerController
extern "C" {
#endif
	typedef void (*StringCallback)(const char *message);
	typedef void (*DrawFinishedCallback)(uint32_t* pixels, int w, int h);
	
	extern StringCallback GG_DebugScreenCallback;
	extern StringCallback GG_DebugConsoleCallback;
	extern DrawFinishedCallback GG_DrawFinishedCallback;



	inline void DebugScreen(const char* Message)
	{
		GG_DebugScreenCallback(Message);
	}

	inline void DebugConsole(const char* Message)
	{
		GG_DebugConsoleCallback(Message);
	}

	inline void DrawFinished(uint32_t* pixels, int w, int h)
	{
		GG_DrawFinishedCallback(pixels, w, h);
	}
	
	void SetDebugScreenCallback(StringCallback Callback);
	void SetDebugConsoleCallback(StringCallback Callback);
	void SetDrawFinishedCallback(DrawFinishedCallback Callback);
#ifdef __cplusplus
}
#endif

