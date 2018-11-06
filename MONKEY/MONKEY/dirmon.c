#include "dirmon.h"

// Watches the accessible user data directory for changes and copies generated data into monocle's system folder ASAP for insertion to the database.
DWORD WINAPI UserDirectoryWatchThread(LPVOID lpWideInfo)
{
	DWORD dwStatus, dwError;
	HANDLE hChange, hFile, hCopyFile;
	WCHAR wFullPattern[MAX_PATH];
	WIDE_DIR_INFO wdi = *(PWIDE_DIR_INFO)lpWideInfo;

	hChange = FindFirstChangeNotificationW(wdi.wUserDir, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);

	while(TRUE)
	{
		dwStatus = WaitForSingleObject(hChange, INFINITE);
		if(dwStatus == WAIT_OBJECT_0)
		{
			WIN32_FIND_DATA wfd;
			BOOL fDone = FALSE;
			WCHAR  wTargetFileName[MAX_PATH];

			PathCchCombine(wFullPattern, MAX_PATH, wdi.wUserDir, L"*");
			SetCurrentDirectoryW(wdi.wUserDir);

			hFile = FindFirstFileW(wFullPattern, &wfd);
			if(hFile != INVALID_HANDLE_VALUE)
			{
				do
				{
					do
					{
						if((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY && _wcsicmp(wfd.cFileName, L"mticker.exe") != 0
							&& _wcsicmp(wfd.cFileName, L"mview.exe") != 0 && _wcsicmp(wfd.cFileName, L"libmysql.dll") != 0 && _wcsicmp(wfd.cFileName, L"unins000.exe") != 0)
						{
							Sleep(750);
							hCopyFile = CreateFileW(wfd.cFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
							dwError = GetLastError();
							if(hCopyFile == INVALID_HANDLE_VALUE && dwError == ERROR_FILE_NOT_FOUND)
							{
								break;
							}
							CloseHandle(hCopyFile);

							PathCchCombine(wTargetFileName, MAX_PATH, wdi.wAppDir, wfd.cFileName);
							fDone = (CopyFileW(wfd.cFileName, wTargetFileName, FALSE) && DeleteFileW(wfd.cFileName));
						}
						else
						{
							break;
						}
					}
					while(fDone == FALSE || hCopyFile == INVALID_HANDLE_VALUE);
				}
				while(FindNextFileW(hFile, &wfd));
				FindClose(hFile);
			}
			FindNextChangeNotification(hChange);
		}
	}
}
