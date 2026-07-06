#ifndef __GET_HWND_H__
#define __GET_HWND_H__
#include <windows.h>
#include <tchar.h>

typedef struct{
	HWND hWND;
	DWORD myPID;
}SParam;

extern "C"{
	__declspec(dllexport) HWND GetGameHWND(void);
}

#endif