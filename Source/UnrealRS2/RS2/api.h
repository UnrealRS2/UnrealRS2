#pragma once

#ifdef __cplusplus
extern "C" {
#endif
	typedef void (*StringCallback)(const char *message);
	extern StringCallback GG_DebugScreenCallback;
	extern StringCallback GG_DebugConsoleCallback;

	inline void DebugScreen(const char* Message)
	{
		GG_DebugScreenCallback(Message);
	}

	inline void DebugConsole(const char* Message)
	{
		GG_DebugConsoleCallback(Message);
	}
	
	void SetDebugScreenCallback(StringCallback Callback);
	void SetDebugConsoleCallback(StringCallback Callback);
#ifdef __cplusplus
}
#endif

