#include <Windows.h>

#include "client.h"
#include "bridge.h"
#include "api.h"

DWORD WINAPI client_thread(LPVOID _)
{
	DebugConsole("RuneScape 2 : Revision 225");
	DebugScreen("RuneScape 2 : Revision 225");
	
	client_main();
	
	return 0;
}

void rs2_start_client()
{
	HANDLE hThread = CreateThread(
	NULL,       // default security
	0,          // default stack size
	client_thread,
	NULL,       // no parameters
	0,          // run immediately
	NULL        // no thread ID out
);
	if (hThread)
	{
		// Mark thread as background (won’t block process exit)
		SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);

		// Don’t keep a handle — let it close automatically when main exits
		CloseHandle(hThread);
	}
}
