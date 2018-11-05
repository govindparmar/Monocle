#include <Windows.h>
#include <wincred.h>
#include <ShlObj.h>
#include <strsafe.h>

HRESULT WINAPI DeleteMonocleTask();

INT APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd)
{
	HKEY hKey;
	WCHAR wszInstallPath[MAX_PATH], *wszStartMenuPath, wszUninstallCommand[MAX_PATH + 20];
	DWORD cbInstallPath = MAX_PATH * sizeof(WCHAR);
	CONST KNOWNFOLDERID kfPrograms = FOLDERID_CommonPrograms;
	UINT uLen;

	if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Monocle", 0, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
	{								   /*L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"*/
		MessageBoxW(NULL, L"Unable to open uninstall registry information for Monocle.", L"Monocle Uninstaller", MB_OK | MB_ICONSTOP);
		//return ERROR_FILE_NOT_FOUND;
	}

	RegQueryValueExW(hKey, L"InstallLocation", 0, NULL, (LPBYTE)wszInstallPath, &cbInstallPath);
	StringCchCatW(wszInstallPath, MAX_PATH, L"mview.exe");
	DeleteFileW(wszInstallPath);

	RegQueryValueExW(hKey, L"InstallLocation", 0, NULL, (LPBYTE)wszInstallPath, &cbInstallPath);
	StringCchCatW(wszInstallPath, MAX_PATH, L"mticker.exe");
	DeleteFileW(wszInstallPath);

	CredDeleteW(L"MonocleDBReadAcct", CRED_TYPE_GENERIC, 0);
	DeleteMonocleTask();

	// For later use in the RunOnce key addition
	RegQueryValueExW(hKey, L"InstallLocation", 0, NULL, (LPBYTE)wszInstallPath, &cbInstallPath);
	RegCloseKey(hKey);

	RegDeleteKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Monocle");

	SHGetKnownFolderPath(&kfPrograms, 0, NULL, &wszStartMenuPath);
	StringCchCatW(wszStartMenuPath, MAX_PATH, L"\\Monocle by Govind Parmar\\Log Viewer.lnk");
	DeleteFileW(wszStartMenuPath);

	SHGetKnownFolderPath(&kfPrograms, 0, NULL, &wszStartMenuPath);
	StringCchCatW(wszStartMenuPath, MAX_PATH, L"\\Monocle by Govind Parmar");
	RemoveDirectoryW(wszStartMenuPath);

	// Removes the program folder on the next reboot
	RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce", 0, KEY_SET_VALUE, &hKey);
	StringCchPrintfW(wszUninstallCommand, MAX_PATH + 20, L"CMD /k RD /s \"%s\" /q", wszInstallPath);
	StringCbLengthW(wszUninstallCommand, (MAX_PATH + 20) * sizeof(WCHAR), &uLen);
	RegSetValueExW(hKey, L"MonocleUninstall", 0, REG_SZ, (LPBYTE)wszUninstallCommand, uLen + sizeof(WCHAR));
	RegCloseKey(hKey);
	MessageBoxW(NULL, L"Uninstalled Monocle.", L"Monocle Uninstaller.", MB_OK | MB_ICONASTERISK);
	return 0;
}