#include <Windows.h>
#include <strsafe.h>
#define MAX_TITLE_LEN 200

BOOL WINAPI SaveBitmap(WCHAR *wPath, SYSTEMTIME st);

BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam)
{
	LONG lStyle = GetWindowLongPtrW(hWnd, GWL_STYLE);

	if(((lStyle & WS_VISIBLE) == WS_VISIBLE) && ((lStyle & WS_SYSMENU) == WS_SYSMENU))
	{
		CONST CHAR CRLF[2] = { L'\r', L'\n' };
		HANDLE hFile = *(HANDLE *)lParam;
		DWORD dwWritten;
		CHAR szTitle[MAX_TITLE_LEN];
		HRESULT hr;
		UINT uLen;
		CHAR szClass[50];

		// On Windows 10, ApplicationFrameWindow may run in the background and
		// WS_VISIBLE will be true even if the window isn't actually visible,
		// for various UWP apps. I don't know of any method for predicting when 
		// this will happen, and for which app(s).
		// 
		// The only way to test if a window is actually *visible* in this case
		// is to test that the child class "Windows.UI.Core.CoreWindow" exists in it.
		GetClassNameA(hWnd, szClass, 50);
		if(strcmp(szClass, "ApplicationFrameWindow") == 0)
		{
			if(FindWindowExA(hWnd, NULL, "Windows.UI.Core.CoreWindow", NULL) == NULL)
				return TRUE;
		}
		GetWindowTextA(hWnd, szTitle, MAX_TITLE_LEN);
		hr = StringCbLengthA(szTitle, MAX_TITLE_LEN * sizeof(WCHAR), &uLen);
		if(SUCCEEDED(hr) && uLen > 0)
		{
			SetFilePointer(hFile, 0, NULL, FILE_END);
			WriteFile(hFile, szTitle, uLen, &dwWritten, NULL);
			WriteFile(hFile, CRLF, 2, &dwWritten, NULL);
		}
	}
	return TRUE;
}

INT APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd)
{
	HANDLE hFile;// = CreateFileW(L"C:\\Temp\\DumpDir\\titles.txt", GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SYSTEMTIME st;
	WCHAR wFileName[MAX_PATH], wSavePath[MAX_PATH];
	DWORD cbPath = MAX_PATH * sizeof(WCHAR);
	DWORD dwWritten = 0;
	HKEY hKey;

	RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\GovindParmar\\MONOCLE", 0, KEY_READ, &hKey);
	RegQueryValueExW(hKey, L"UserDir", NULL, NULL, (LPBYTE)wSavePath, &cbPath);
	RegCloseKey(hKey);

	GetLocalTime(&st);
	StringCchPrintfW(wFileName, MAX_PATH, L"%.4hu-%.2hu-%.2hu-%.2hu-%.2hu-%.2hu.txt", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	hFile = CreateFileW(wFileName, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	EnumWindows(EnumProc, (LPARAM)&hFile);
	CloseHandle(hFile);
	SaveBitmap(wSavePath, st);
	return 0;
}
