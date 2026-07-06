#include <windows.h>
#include <tchar.h>
#include "get_hwnd.h"


BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	SParam* data = (SParam*)lParam;
	DWORD tempPID = 0;
	GetWindowThreadProcessId(hWnd, &tempPID);
	if (data->myPID == tempPID){
		TCHAR tempClassname[32];
		ZeroMemory(tempClassname, sizeof(tempClassname));
		GetClassName(hWnd, tempClassname,32);
		if ( CompareString(LOCALE_SYSTEM_DEFAULT,0, TEXT("RGSS Player"),-1,tempClassname,-1) == CSTR_EQUAL ){
			data->hWND = hWnd;
			return FALSE;
		}else{
			return TRUE;
		}
	}else{
		return TRUE;
	}
}


HWND GetGameHWND()
{
	SParam data;
	data.hWND = 0;
	data.myPID = GetCurrentProcessId();
	EnumWindows(EnumWindowsProc, (LPARAM)&data);
	if(data.hWND > 0){
		return data.hWND;
	}else{
		return 0;
	}
}
