#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <WtsApi32.h>
#include <strsafe.h>
#include "registry.h"
#include "database.h"
#pragma comment(lib, "wtsapi32.lib")

#define IDT_TIMER1 1001
#define IDT_TIMER2 1002

extern const WCHAR g_wszClassName[];

DWORD CALLBACK TimerWrapThread(
	LPVOID lpHinstance
);

VOID CALLBACK TimerProc1(
	HWND hWnd,
	UINT_PTR nID,
	UINT uElapse,
	DWORD dwTime
);

VOID CALLBACK TimerProc2(
	HWND hWnd, 
	UINT_PTR nID,
	UINT uElapse,
	DWORD dwTime
);