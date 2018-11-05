#include "registry.h"

LSTATUS WINAPI ReadRegistrySettings(_Out_ PPROGRAM_OPTIONS pOptions)
{
	HKEY hKey;
	LSTATUS lsResult;
	DWORD cbDBName = 128, cbServer = 200, cbUser = 100, cbPass = 100, cbPort = sizeof(DWORD), cbAppDir = MAX_PATH , cbUserDir = MAX_PATH, cbInterval = sizeof(DWORD);

	if((lsResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\GovindParmar\\MONOCLE", 0, KEY_READ, &hKey)) != ERROR_SUCCESS)
	{
		return lsResult;
	}

	// this field exists because it used to be configurable; now its hardcoded as "mondb"
	StringCchCopyA(pOptions->wDBName, 128, "mondb");

	if((lsResult = RegQueryValueExA(hKey, "Server", NULL, NULL, (BYTE *)pOptions->wDBServer, &cbServer)) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return lsResult;
	}

	if((lsResult = RegQueryValueExA(hKey, "User", NULL, NULL, (BYTE *)pOptions->wDBUser, &cbUser)) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return lsResult;
	}

	if((lsResult = RegQueryValueExA(hKey, "Password", NULL, NULL, (BYTE *)pOptions->wDBPass, &cbPass)) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return lsResult;
	}

	if((lsResult = RegQueryValueExA(hKey, "Port", NULL, NULL, (BYTE *)&pOptions->dwPort, &cbPort)) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return lsResult;
	}


	if((lsResult = RegQueryValueExA(hKey, "AppDir", NULL, NULL, (BYTE *)pOptions->wAppDir, &cbAppDir)) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return lsResult;
	}

	if((lsResult = RegQueryValueExA(hKey, "UserDir", NULL, NULL, (BYTE *)pOptions->wUserDir, &cbUserDir)) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return lsResult;
	}

	if((lsResult = RegQueryValueExA(hKey, "TickInterval", NULL, NULL, (BYTE *)&pOptions->dwInterval, &cbInterval)) != ERROR_SUCCESS)
	{
		RegCloseKey(hKey);
		return lsResult;
	}

	return ERROR_SUCCESS;
}
