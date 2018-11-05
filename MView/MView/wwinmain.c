#include <Windows.h>
#include <wincred.h>
#include <Shlwapi.h>
#include <CommCtrl.h>
#include <strsafe.h>
#include <mysql.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "libmysql.lib") 
#pragma comment(lib, "Shlwapi.lib")

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

typedef struct _READCREDS
{
	CHAR szServer[128];
	CHAR szUser[64];
	CHAR *szPass;
	DWORD dwPort;
} READCREDS, *PREADCREDS;

READCREDS g_readCreds;
WCHAR g_wszGeneratedFile[MAX_PATH];
CHAR g_szDevice[100] = "<<BAD>>";

LRESULT CALLBACK ImgPreviewPaint(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT_PTR uID, DWORD_PTR dwRef)
{
	
	return DefSubclassProc(hWnd, Msg, wParam, lParam);
}

BOOL WINAPI CatWindowTextA(HWND hWnd, CHAR *szCat, BOOL fNewLine)
{
	CONST HANDLE hHeap = GetProcessHeap();
	CHAR *szCurrent = NULL;
	SIZE_T cchCurrent = GetWindowTextLengthA(hWnd) + 1, cchCat, cchAlloc;
	BOOL fSuccess = FALSE;


	StringCchLengthA(szCat, STRSAFE_MAX_CCH, &cchCat);
	cchAlloc = cchCurrent + cchCat + 1 + (fNewLine * 2);

	szCurrent = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cchAlloc);
	if(!szCurrent)
	{
		return FALSE;
	}

	if(!GetWindowTextA(hWnd, szCurrent, cchAlloc))
	{
		goto cleanup;
	}
	if(fNewLine)
	{
		StringCchCatA(szCurrent, cchAlloc, "\r\n");
	}
	StringCchCatA(szCurrent, cchAlloc, szCat);

	fSuccess = SetWindowTextA(hWnd, szCurrent);

cleanup:
	HeapFree(hHeap, 0, szCurrent);
	szCurrent = NULL;

	return fSuccess;
}

BOOL WINAPI CatWindowTextW(HWND hWnd, WCHAR *wszCat, BOOL fNewLine)
{
	CONST HANDLE hHeap = GetProcessHeap();
	WCHAR *wszCurrent = NULL;
	SIZE_T cchCurrent = GetWindowTextLengthW(hWnd) + 1, cchCat, cbAlloc;
	BOOL fSuccess = FALSE;


	StringCchLengthW(wszCat, STRSAFE_MAX_CCH, &cchCat);
	cbAlloc = (cchCurrent + cchCat + 1 + (fNewLine * 2)) * sizeof(WCHAR);

	wszCurrent = (WCHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cbAlloc);
	if(!wszCurrent)
	{
		return FALSE;
	}

	if(!GetWindowTextW(hWnd, wszCurrent, cbAlloc/sizeof(WCHAR)))
	{
		goto cleanup;
	}
	if(fNewLine)
	{
		StringCbCatW(wszCurrent, cbAlloc, L"\r\n");
	}
	StringCbCatW(wszCurrent, cbAlloc, wszCat);

	fSuccess = SetWindowTextW(hWnd, wszCurrent);

cleanup:
	HeapFree(hHeap, 0, wszCurrent);
	wszCurrent = NULL;

	return fSuccess;
}
HBITMAP WINAPI HBITMAPFromJpegFile(WCHAR *wszFileName);

VOID WINAPI PopulateImagePreviewWindow(HWND hStatic, CHAR *szDateTime, _Out_ WCHAR *wszFile)
{
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	HBITMAP hBitmap = NULL;
	CHAR szFormat[100], szQuery[200];
	HINSTANCE hInstance = GetModuleHandleW(NULL);
	HANDLE hBackingFile;
	WCHAR wszPath[MAX_PATH];
	DWORD dwWritten;
	


	GetEnvironmentVariableW(L"Temp", wszPath, MAX_PATH);
	GetTempFileNameW(wszPath, L"mncl", 0, wszFile);
	

	hBackingFile = CreateFileW(wszFile, GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SetFilePointer(hBackingFile, 0, NULL, FILE_BEGIN);

	LoadStringA(hInstance, IDS_QUERY_SCREENSHOT_SPECIFIC_DATETIME, szFormat, 100);
	StringCchPrintfA(szQuery, 200, szFormat, g_szDevice, szDateTime);

	conn = mysql_init(NULL);
	mysql_real_connect(conn, g_readCreds.szServer, g_readCreds.szUser, g_readCreds.szPass, "mondb", g_readCreds.dwPort, NULL, 0);
	mysql_query(conn, szQuery);

	res = mysql_store_result(conn);
	if(row = mysql_fetch_row(res))
	{
		WriteFile(hBackingFile, row[0], strtoul(row[1], NULL, 10), &dwWritten, NULL);
	}
	CloseHandle(hBackingFile);

	hBitmap = HBITMAPFromJpegFile(wszFile);
	

	
	SendMessageW(hStatic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
	
	mysql_free_result(res);
	mysql_close(conn);

}

VOID WINAPI PopulateTextPreviewWindow(HWND hStatic, CHAR *szText, SIZE_T cchText, CHAR *szDateTime, WPARAM wpRadioID)
{
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	CHAR szQuery[200], szFormat[100];
	HINSTANCE hInstance = GetModuleHandleW(NULL);

	conn = mysql_init(NULL);
	mysql_real_connect(conn, g_readCreds.szServer, g_readCreds.szUser, g_readCreds.szPass, "mondb", g_readCreds.dwPort, NULL, 0);

	if(wpRadioID == IDR_WINDOWTITLES)
	{
		LoadStringA(hInstance, IDS_QUERY_TITLES_SPECIFIC_DATETIME, szFormat, 100);
	}
	else
	{
		LoadStringA(hInstance, IDS_QUERY_WEBSITES_SPECIFIC_DATETIME, szFormat, 100);
	}
	
	StringCchPrintfA(szQuery, 200, szFormat, g_szDevice, szDateTime);

	mysql_query(conn, szQuery);

	res = mysql_store_result(conn);
	while(row = mysql_fetch_row(res))
	{
		StringCchCatA(szText, cchText, row[0]);
		StringCchCatA(szText, cchText, "\r\n");
	}

	mysql_free_result(res);
	mysql_close(conn);

	SetWindowTextA(hStatic, szText);
}

SIZE_T WINAPI GetTextPreviewLength(WPARAM wpRadioID, CHAR *szDateTime)
{
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	HINSTANCE hInstance = GetModuleHandleW(NULL);
	CHAR szFormat[150], szQuery[300];
	SIZE_T cchEntry = (SIZE_T)-1;

	if(wpRadioID == IDR_WEBTRAFFIC)
	{
		LoadStringA(hInstance, IDS_QUERY_WEBSITES_SPECIFIC_DATETIME_LENGTH, szFormat, 150);
		StringCchPrintfA(szQuery, 300, szFormat, g_szDevice, szDateTime);
	}
	else if(wpRadioID == IDR_WINDOWTITLES)
	{
		LoadStringA(hInstance, IDS_QUERY_TITLES_SPECIFIC_DATETIME_LENGTH, szFormat, 150);
		StringCchPrintfA(szQuery, 300, szFormat, g_szDevice, szDateTime);
	}
	else
	{
		return (SIZE_T)-1;			
	}

	conn = mysql_init(NULL);
	mysql_real_connect(conn, g_readCreds.szServer, g_readCreds.szUser, g_readCreds.szPass, "mondb", g_readCreds.dwPort, NULL, 0);
	mysql_query(conn, szQuery);
	res = mysql_store_result(conn);
	if(row = mysql_fetch_row(res))
	{
		cchEntry = strtoul(row[0], NULL, 10);
	}
	mysql_free_result(res);
	mysql_close(conn);
	return cchEntry;
}

VOID WINAPI PopulateListBox(HWND hLbx, WPARAM wpRadioID, SYSTEMTIME stStart, SYSTEMTIME stEnd, CHAR *szFilter)
{
	MYSQL *conn;
	MYSQL_RES *res;
	MYSQL_ROW row;
	INT nCount = SendMessageW(hLbx, LB_GETCOUNT, 0, 0);
	HINSTANCE hInstance = GetModuleHandleW(NULL);
	while(nCount)
	{
		SendMessageW(hLbx, LB_DELETESTRING, 0, 0);
		nCount--;
	}
	conn = mysql_init(NULL);
	mysql_real_connect(conn, g_readCreds.szServer, g_readCreds.szUser, g_readCreds.szPass, "mondb", g_readCreds.dwPort, NULL, 0);
	switch(wpRadioID)
	{
		case IDR_SCREENSHOTS:
		{
			CHAR szFormat[200], szQuery[400];
			LoadStringA(hInstance, IDS_QUERY_SCREENSHOTS_DISTINCT_TIMES_RANGE, szFormat, 200);
			StringCchPrintfA(szQuery, 400, szFormat, g_szDevice, stStart.wYear, stStart.wMonth, stStart.wDay, stEnd.wYear, stEnd.wMonth, stEnd.wDay);	
			mysql_query(conn, szQuery);

		}
			break;
		case IDR_WEBTRAFFIC:
		{
			CHAR szFormat[300], szQuery[600];
			LoadStringA(hInstance, szFilter ? IDS_QUERY_WEBSITES_DISTINCT_TIMES_FILTER_RANGE : IDS_QUERY_WEBSITES_DISTINCT_TIMES_RANGE,
				szFormat, 300);
			if(szFilter != NULL)
			{
				StringCchPrintfA(szQuery, 600, szFormat, g_szDevice, szFilter, stStart.wYear, stStart.wMonth, stStart.wDay, stEnd.wYear, stEnd.wMonth, stEnd.wDay);
			}
			else
			{
				StringCchPrintfA(szQuery, 600, szFormat, g_szDevice, stStart.wYear, stStart.wMonth, stStart.wDay, stEnd.wYear, stEnd.wMonth, stEnd.wDay);
			}
			mysql_query(conn, szQuery);

		}
			break;
		case IDR_WINDOWTITLES:
		{
			CHAR szFormat[300], szQuery[600];
			LoadStringA(hInstance, szFilter ? IDS_QUERY_TITLES_TIME_FILTER_RANGE : IDS_QUERY_TITLES_TIME_RANGE,
				szFormat, 300);
			if(szFilter != NULL)
			{
				StringCchPrintfA(szQuery, 600, szFormat, g_szDevice, szFilter, stStart.wYear, stStart.wMonth, stStart.wDay, stEnd.wYear, stEnd.wMonth, stEnd.wDay);
			}
			else
			{
				StringCchPrintfA(szQuery, 600, szFormat, g_szDevice, stStart.wYear, stStart.wMonth, stStart.wDay, stEnd.wYear, stEnd.wMonth, stEnd.wDay);
			}
			mysql_query(conn, szQuery);

		}
			break;
		default:
			mysql_close(conn);
			return;

	}

	res = mysql_store_result(conn);
	while(row = mysql_fetch_row(res))
	{
		SendMessageA(hLbx, LB_ADDSTRING, 0, (LPARAM)(LPSTR)row[0]);
	}
	mysql_free_result(res);
	mysql_close(conn);
}

WORD WINAPI GetLastDayOfMonth(WORD wMonth, WORD wYear)
{
	switch(wMonth)
	{
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			return 31;
		case 4:
		case 6:
		case 9:
		case 11:
			return 30;
		case 2:
			return ((wYear % 4 == 0) && (wYear % 100 != 0 || wYear % 400 == 0)) ? 29 : 28;
		default:
			return (WORD)-1; // Lousy smarch weather
	}
}

VOID WINAPI SwapVisibility(HWND hDlg, HWND hHide, HWND hShow)
{
	LONG_PTR lpStyle;
	lpStyle = GetWindowLongPtrW(hHide, GWL_STYLE);
	lpStyle &= ~WS_VISIBLE;
	SetWindowLongPtrW(hHide, GWL_STYLE, lpStyle);

	lpStyle = GetWindowLongPtrW(hShow, GWL_STYLE);
	lpStyle |= WS_VISIBLE;
	SetWindowLongPtrW(hShow, GWL_STYLE, lpStyle);

	RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
}

VOID WINAPI PopulateComboBox(HWND hComboBox)
{
	MYSQL *conn = NULL;
	MYSQL_RES *res;
	MYSQL_ROW row;
	CHAR szQuery[200];
	HINSTANCE hInstance = GetModuleHandleW(NULL);

	conn = mysql_init(NULL);
	mysql_real_connect(conn, g_readCreds.szServer, g_readCreds.szUser, g_readCreds.szPass, "mondb", 0, NULL, 0);
	LoadStringA(hInstance, IDS_QUERY_ORIGINS, szQuery, 199);
	mysql_query(conn, szQuery);

	res = mysql_store_result(conn);

	while(row = mysql_fetch_row(res))
	{
		SendMessageA(hComboBox, CB_ADDSTRING, 0, (LPARAM)row[0]);
	}
	SendMessageA(hComboBox, CB_SETCURSEL, 0, 0);
	mysql_free_result(res);
	mysql_close(conn);
}
INT_PTR CALLBACK DialogProc2(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	CONST HANDLE hHeap = GetProcessHeap();
	HWND hStcTxtPreview, hStcImgPreview, hRadio, hRadio2, hRadio3, hCalStart, hCalStop, hEdtFilter, hLbxDT;
	CHAR szTitle[200];
	SYSTEMTIME stStart, stEnd;

	hStcTxtPreview = FindWindowExW(hDlg, NULL, L"Static", NULL);
	hStcTxtPreview = FindWindowExW(hDlg, hStcTxtPreview, L"Static", NULL);
	hStcTxtPreview = FindWindowExW(hDlg, hStcTxtPreview, L"Static", NULL);
	hStcTxtPreview = FindWindowExW(hDlg, hStcTxtPreview, L"Static", NULL);
	hStcImgPreview = FindWindowExW(hDlg, hStcTxtPreview, L"Static", NULL);
	hRadio = FindWindowExW(hDlg, NULL, L"Button", NULL);
	hRadio2 = FindWindowExW(hDlg, hRadio, L"Button", NULL);
	hRadio3 = FindWindowExW(hDlg, hRadio2, L"Button", NULL);
	hCalStart = FindWindowExW(hDlg, NULL, L"SysMonthCal32", NULL);
	hCalStop = FindWindowExW(hDlg, hCalStart, L"SysMonthCal32", NULL);
	hEdtFilter = FindWindowExW(hDlg, NULL, L"Edit", NULL);
	hLbxDT = FindWindowExW(hDlg, NULL, L"ListBox", NULL);

	switch(Msg)
	{
		NMHDR nHeader;
		case WM_NOTIFY:
			nHeader = *(NMHDR *)lParam;
			if(nHeader.code == MCN_SELCHANGE)
			{
				switch(wParam)
				{
					WPARAM wpRadioID;
					INT nFltLen = 0;
					CHAR *szFilter = NULL;
					case IDT_STARTDATE:
					case IDT_STOPDATE:
						MonthCal_GetCurSel(hCalStart, &stStart);
						MonthCal_GetCurSel(hCalStop, &stEnd);
						if(SendMessageW(hRadio, BM_GETCHECK, 0, 0) == BST_CHECKED)
							wpRadioID = IDR_SCREENSHOTS;
						else if(SendMessageW(hRadio2, BM_GETCHECK, 0, 0) == BST_CHECKED)
							wpRadioID = IDR_WINDOWTITLES;
						else
							wpRadioID = IDR_WEBTRAFFIC;

						if((nFltLen = GetWindowTextLengthA(hEdtFilter) + 1) > 3)
						{
							szFilter = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nFltLen + 1);
							GetWindowTextA(hEdtFilter, szFilter, nFltLen);
						}
						else
						{
							szFilter = NULL;
						}

						PopulateListBox(hLbxDT, wpRadioID, stStart, stEnd, szFilter);
						if(szFilter != NULL)
						{
							HeapFree(hHeap, 0, szFilter);
						}
						break;
				}
			}
			break;
		case WM_COMMAND:
		{
			CHAR *szFilter = NULL;
			INT nFltLen = 0;
			if((nFltLen = GetWindowTextLengthA(hEdtFilter) + 1) > 3)
			{
				szFilter = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nFltLen + 1);
				GetWindowTextA(hEdtFilter, szFilter, nFltLen);
			}
			else
			{
				szFilter = NULL;
			}

			MonthCal_GetCurSel(hCalStart, &stStart);
			MonthCal_GetCurSel(hCalStop, &stEnd);
			switch(LOWORD(wParam))
			{
				INT cchText = 0;
				CHAR *szText = NULL;
				HANDLE hPreviewFile;
				DWORD dwWritten;
				case IDE_FILTERTEXT:
					if(HIWORD(wParam) == EN_CHANGE)
					{
						WPARAM wpRadioID;
						if(SendMessageW(hRadio, BM_GETCHECK, 0, 0) == BST_CHECKED)
							wpRadioID = IDR_SCREENSHOTS;
						else if(SendMessageW(hRadio2, BM_GETCHECK, 0, 0) == BST_CHECKED)
							wpRadioID = IDR_WINDOWTITLES;
						else
							wpRadioID = IDR_WEBTRAFFIC;

						PopulateListBox(hLbxDT, wpRadioID, stStart, stEnd, szFilter);
					}
					break;
				case IDS_TEXTPREVIEW:
					cchText = GetWindowTextLengthA(hStcTxtPreview) + 1;
					if(cchText > 1)
					{
						WCHAR wszPopDir[MAX_PATH], wszNewDir[MAX_PATH]; 
						GetCurrentDirectoryW(MAX_PATH, wszPopDir);
						GetEnvironmentVariableW(L"USERPROFILE", wszNewDir, MAX_PATH);
						StringCchCatW(wszNewDir, MAX_PATH, L"\\preview.txt");
						szText = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cchText + 1);
						if(szText == NULL)
						{
							MessageBoxW(NULL, L"Unable to allocate memory to copy the text into an external viewer.", L"Monocle Log Viewer", MB_OK | MB_ICONSTOP);
							ExitProcess(ERROR_OUTOFMEMORY);
						}
						GetWindowTextA(hStcTxtPreview, szText, cchText);
						SetCurrentDirectoryW(wszNewDir);
						hPreviewFile = CreateFileW(wszNewDir, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
						WriteFile(hPreviewFile, szText, cchText, &dwWritten, NULL);
						CloseHandle(hPreviewFile);
						SetCurrentDirectoryW(wszPopDir);
						ShellExecuteW(NULL, L"open", wszNewDir, NULL, NULL, SW_SHOW);
					}
					break;
				case IDS_IMGPREVIEW:
					ShellExecuteW(NULL, L"open", g_wszGeneratedFile, NULL, NULL, SW_SHOW);
					break;
				case IDR_SCREENSHOTS:
					SwapVisibility(hDlg, hStcTxtPreview, hStcImgPreview);
					SetWindowTextW(hEdtFilter, L"");
					EnableWindow(hEdtFilter, FALSE);
					PopulateListBox(hLbxDT, IDR_SCREENSHOTS, stStart, stEnd, NULL);
					break;
				case IDR_WEBTRAFFIC:
					SwapVisibility(hDlg, hStcImgPreview, hStcTxtPreview);
					EnableWindow(hEdtFilter, TRUE);
					PopulateListBox(hLbxDT, IDR_WEBTRAFFIC, stStart, stEnd, szFilter);
					break;
				case IDR_WINDOWTITLES:
					SwapVisibility(hDlg, hStcImgPreview, hStcTxtPreview);
					EnableWindow(hEdtFilter, TRUE);
					PopulateListBox(hLbxDT, IDR_WINDOWTITLES, stStart, stEnd, szFilter);
					break;
				case IDC_LIST1:
					if(HIWORD(wParam) == LBN_SELCHANGE)
					{
						CHAR szDateTime[24], *szResult = NULL;
						INT nSel = SendMessageW(hLbxDT, LB_GETCURSEL, 0, 0);
						SIZE_T cchEntry;
						
						SendMessageA(hLbxDT, LB_GETTEXT, nSel, (LPARAM)szDateTime);
						if(SendMessageW(hRadio, BM_GETCHECK, 0, 0) == BST_UNCHECKED)
						{
								cchEntry = GetTextPreviewLength(
									SendMessageW(hRadio2, BM_GETCHECK, 0, 0) == BST_CHECKED ?
									IDR_WINDOWTITLES : IDR_WEBTRAFFIC,
									szDateTime);

								szResult = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, cchEntry + 1);
								if(szResult == NULL)
								{
									MessageBoxW(NULL, 
										L"Not enough memory to complete this operation. Try closing some programs or restarting your computer.", 
										L"Monocle Viewer",
										MB_OK | MB_ICONSTOP);
								}

								PopulateTextPreviewWindow(
									hStcTxtPreview, szResult, cchEntry, szDateTime, 
									SendMessageW(hRadio2, BM_GETCHECK, 0, 0) == BST_CHECKED ? 
									IDR_WINDOWTITLES : IDR_WEBTRAFFIC);

								HeapFree(hHeap, 0, szResult);
								szResult = NULL;
						}
						else // screenshot
						{
							CHAR szDateTime[24];
							INT nSel = SendMessageW(hLbxDT, LB_GETCURSEL, 0, 0);
							SendMessageA(hLbxDT, LB_GETTEXT, nSel, (LPARAM)szDateTime);
							PopulateImagePreviewWindow(hStcImgPreview, szDateTime, g_wszGeneratedFile);
						}
					}
					break;


			}
			if(szFilter != NULL)
			{
				HeapFree(hHeap, 0, szFilter);
			}
		}
			return (INT_PTR)TRUE;
		case WM_INITDIALOG:
			//SetWindowSubclass(hStcImgPreview, ImgPreviewPaint, 0, 0);
			MonthCal_GetCurSel(hCalStart, &stStart);
			stStart.wDay = 1;
			MonthCal_GetCurSel(hCalStop, &stEnd);
			stEnd.wDay = GetLastDayOfMonth(stEnd.wMonth, stEnd.wYear);
			MonthCal_SetCurSel(hCalStart, &stStart);
			MonthCal_SetCurSel(hCalStop, &stEnd);
			StringCchPrintfA(szTitle, 200, "Viewing logs for %s", g_szDevice);
			SendMessageW(hRadio, BM_SETCHECK, BST_CHECKED, 0);
			SendMessageW(hDlg, WM_COMMAND, MAKEWPARAM(IDR_SCREENSHOTS, BM_SETCHECK), (LPARAM)hRadio);
			SwapVisibility(hDlg, hStcTxtPreview, hStcImgPreview);
			SetWindowTextA(hDlg, szTitle);
			return (INT_PTR)TRUE;
		case WM_CLOSE:
		case WM_DESTROY:
			EndDialog(hDlg, (INT_PTR)0);
		default:
			return (INT_PTR)FALSE;
	}
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK DialogProc1(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	HWND hOK = FindWindowExW(hDlg, NULL, L"Button", NULL);
	HWND hCbx = FindWindowExW(hDlg, NULL, L"ComboBox", NULL);
	switch(Msg)
	{
		case WM_COMMAND:
			if(LOWORD(wParam) == IDB_OK)
			{
				INT nSel = SendMessageW(hCbx, CB_GETCURSEL, 0, 0);
				SendMessageA(hCbx, CB_GETLBTEXT, nSel, (LPARAM)g_szDevice);
				EndDialog(hDlg, (INT_PTR)0);

			}
			else if(LOWORD(wParam) == IDB_CANCEL)
			{
				EndDialog(hDlg, (INT_PTR)0);
			}
			return (INT_PTR)TRUE;
		case WM_INITDIALOG:
			PopulateComboBox(hCbx);
			return (INT_PTR)TRUE;
		case WM_CLOSE:
		case WM_DESTROY:
			EndDialog(hDlg, (INT_PTR)0);
		default:
			return (INT_PTR)FALSE;
	}
}



INT APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd)
{
	INITCOMMONCONTROLSEX iccx;
	PCREDENTIALA pReadAcct;
	CONST HANDLE hHeap = GetProcessHeap();
	CHAR *szPass = NULL, *pDelim1 = NULL, *pDelim2 = NULL;

	iccx.dwICC = ICC_STANDARD_CLASSES | ICC_DATE_CLASSES;
	iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);

	InitCommonControlsEx(&iccx);

	if(CredReadA("MonocleDBReadAcct", CRED_TYPE_GENERIC, 0, &pReadAcct) == FALSE)
	{
		DWORD dwError = GetLastError();
		MessageBoxW(
			NULL,
			L"Unable to load credentials for viewing log files. Ensure that the generic credential MonocleDBReadAcct exists and has the correct server name, account name, port and password.",
			L"Error",
			MB_OK | MB_ICONSTOP
		);
		return dwError;
	}

	szPass = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (pReadAcct->CredentialBlobSize + 1) * sizeof(CHAR));
	if(szPass == NULL)
	{
		MessageBoxW(NULL, L"Out of memory", L"Error", MB_OK | MB_ICONSTOP);
		return ERROR_OUTOFMEMORY;
	}

	CopyMemory(szPass, pReadAcct->CredentialBlob, pReadAcct->CredentialBlobSize);

	// TODO: mysql connect, populate combo box, load main window 
	g_readCreds.szPass = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (pReadAcct->CredentialBlobSize + 1));
	if(g_readCreds.szPass == NULL)
	{
		MessageBoxW(NULL, L"Out of memory", L"Error", MB_OK | MB_ICONSTOP);
	}

	pDelim1 = strchr(pReadAcct->UserName, '@');
	pDelim2 = strchr(pReadAcct->UserName, ':');

	g_readCreds.dwPort = strtoul(pDelim2 + 1, NULL, 10);

	StringCchCopyNA(g_readCreds.szUser, 64, pReadAcct->UserName, pDelim1 - pReadAcct->UserName);
	StringCchCopyNA(g_readCreds.szServer, 128, pDelim1 + 1, pDelim2 - pDelim1 - 1);
	StringCchCopyA(g_readCreds.szPass, pReadAcct->CredentialBlobSize + 1, szPass);

	DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_CHOOSECOMP), NULL, DialogProc1);
	if(strcmp(g_szDevice, "<<BAD>>") == 0)
	{
		goto cleanup;
	}
	DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_VIEWLOG), NULL, DialogProc2);
	
	/*{
	CHAR msg[1000];
	StringCchPrintfA(msg, 1000, "Server: %s\nPort: %I32u\nUsername: %s\nPass: %s", g_readCreds.szServer, g_readCreds.dwPort, g_readCreds.szUser, g_readCreds.szPass);
	MessageBoxA(NULL, msg, "Report: ", MB_OK | MB_ICONASTERISK);
	}*/
cleanup:
	CredFree((PVOID)pReadAcct);

	HeapFree(hHeap, 0, szPass);
	szPass = NULL;
	return 0;
}