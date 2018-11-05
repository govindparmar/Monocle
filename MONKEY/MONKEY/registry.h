#pragma once
#include <Windows.h>
#include <strsafe.h>
#include <sal.h>

typedef struct _PROGRAM_OPTIONS
{
	CHAR wDBName[128];
	CHAR wDBServer[200];
	CHAR wDBUser[100];
	CHAR wDBPass[100];
	DWORD dwPort;
	CHAR wAppDir[MAX_PATH];
	CHAR wUserDir[MAX_PATH];
	DWORD dwInterval;
} PROGRAM_OPTIONS, *PPROGRAM_OPTIONS;

LSTATUS WINAPI ReadRegistrySettings(_Out_ PPROGRAM_OPTIONS pOptions);