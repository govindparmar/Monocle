#pragma once
#include <Windows.h>
#include <UserEnv.h>
#include <strsafe.h>
#include <PathCch.h>
#include "registry.h"
#include "database.h"

#pragma comment(lib, "UserEnv.lib")
#pragma comment(lib, "PathCch.lib")

typedef struct _WIDE_DIR_INFO
{
	WCHAR wUserDir[MAX_PATH];
	WCHAR wAppDir[MAX_PATH];
} WIDE_DIR_INFO, *PWIDE_DIR_INFO;

DWORD WINAPI UserDirectoryWatchThread(LPVOID lpParameter);