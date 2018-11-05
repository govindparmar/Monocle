#include "httpslogger.h"
#include "database.h"
#include "timers.h"
#include "dirmon.h"

INT APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd)
{
	HANDLE hFile
		= CreateFileA("weblogfile.log", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL),
		hThread[3/*Test*/];
	WEBLOGGER_INFO wi;
	PROGRAM_OPTIONS opt;
	WIDE_DIR_INFO wdi;

	ReadRegistrySettings(&opt);
	wi.hFile = hFile;
	wi.dwInterfaceIndex = 0;

	MultiByteToWideChar(CP_ACP, 0, opt.wAppDir, -1, wdi.wAppDir, MAX_PATH);
	MultiByteToWideChar(CP_ACP, 0, opt.wUserDir, -1, wdi.wUserDir, MAX_PATH);
	
	hThread[0] = CreateThread(NULL, 16384, (LPTHREAD_START_ROUTINE)&TrafficSniffProc, (LPVOID)&wi, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&TimerWrapThread, (LPVOID)&hInstance, 0, NULL);
	hThread[2] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&UserDirectoryWatchThread, (LPVOID)&wdi, 0, NULL);
	//WaitForSingleObject(hThread, INFINITE);
	WaitForMultipleObjects(3, hThread, TRUE, INFINITE);

	//InsertScreenshot(L"C:\\Temp\\tryquery.png");
	//InsertTitleList(L"C:\\Temp\\titles.txt");
	//InsertBrowseHistory(L"C:\\Temp\\sywelote.txt");

	
	return 0;
}