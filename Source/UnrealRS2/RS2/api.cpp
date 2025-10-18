#include "api.h"

#ifdef __cplusplus
extern "C" {
#endif
	StringCallback GG_DebugScreenCallback = nullptr;
	StringCallback GG_DebugConsoleCallback = nullptr;
	DrawFinishedCallback GG_DrawFinishedCallback = nullptr;
	void SetDebugScreenCallback(StringCallback Callback)
	{
		GG_DebugScreenCallback = std::move(Callback);
	}
	void SetDebugConsoleCallback(StringCallback Callback)
	{
		GG_DebugConsoleCallback = std::move(Callback);
	}
	void SetDrawFinishedCallback(DrawFinishedCallback Callback)
	{
		GG_DrawFinishedCallback = std::move(Callback);
	}
#ifdef __cplusplus
}
#endif
