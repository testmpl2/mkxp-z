#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "tktk_error.h"
#include "TktkBitmap.h"
#include "get_hwnd.h"


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch(fdwReason)
    {
        case    DLL_PROCESS_ATTACH:
            break;

        case    DLL_PROCESS_DETACH:
            break;

        case    DLL_THREAD_ATTACH:
            break;

        case    DLL_THREAD_DETACH:
            break;
    }
    return  TRUE;
}

int Version(void)
{
	return 10200;
}

