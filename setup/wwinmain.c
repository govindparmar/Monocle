#include <Windows.h>
#include <wincred.h>
#include <commctrl.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <aclapi.h>
#include <LsaLookup.h>
#include <NTSecAPI.h>
#include <sddl.h>
#include <strsafe.h>
#include <time.h>
#include <mysql.h>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "libmysql.lib")

#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

HRESULT WINAPI CreateShortcut(WCHAR *wszFilePath, WCHAR *wszOutLnkPath, WCHAR *wszDesc);
HRESULT WINAPI CreateSchedTask(WCHAR *wszMonitorExePath);

DWORD WINAPI ReplaceACLWithSingleACE(
	LPWSTR lpObjectName,
	SE_OBJECT_TYPE objType,
	LPWSTR lpSoleTrustee
)
{
	DWORD dwError;
	PSECURITY_DESCRIPTOR psdItem;
	PACL pCurrentACL, pNewACL;
	EXPLICIT_ACCESSW ea;
	WORD wCount;

	dwError = GetNamedSecurityInfoW(lpObjectName, objType, DACL_SECURITY_INFORMATION,
		NULL, NULL, &pCurrentACL, NULL, &psdItem);
	if(dwError != ERROR_SUCCESS)
		return dwError;

	wCount = pCurrentACL->AceCount;
	while(wCount)
	{
		DeleteAce(pCurrentACL, 0);
		wCount--;
	}

	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESSW));
	ea.grfAccessPermissions = GENERIC_ALL;
	ea.grfAccessMode = GRANT_ACCESS;
	ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
	ea.Trustee.TrusteeType = TRUSTEE_IS_USER;
	ea.Trustee.ptstrName = lpSoleTrustee;
	dwError = SetEntriesInAclW(1, &ea, pCurrentACL, &pNewACL);
	if(dwError != ERROR_SUCCESS)
		return dwError;

	dwError = SetNamedSecurityInfoW(lpObjectName, objType,
		DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
		NULL, NULL, pNewACL, NULL);

	LocalFree((HLOCAL)pNewACL);
	return dwError;

}


LSTATUS WINAPI CreateRegistryEntry(CHAR *szServer, CHAR *szWrite, CHAR *szPass, WCHAR *wszSysDir, WCHAR *wszUsrDir, DWORD dwPort, DWORD dwIntervalS)
{
	HKEY hKey;
	LSTATUS lsResult;
	UINT uLen;
	

	lsResult = RegCreateKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\GovindParmar\\MONOCLE", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS | KEY_WOW64_32KEY, NULL, &hKey, NULL);
	StringCbLengthA(szServer, STRSAFE_MAX_CCH, &uLen);
	lsResult = RegSetValueExA(hKey, "Server", 0, REG_SZ, (LPBYTE)szServer, uLen + 1);
	StringCbLengthA(szWrite, STRSAFE_MAX_CCH, &uLen);
	lsResult = RegSetValueExA(hKey, "User", 0, REG_SZ, (LPBYTE)szWrite, uLen + 1);
	StringCbLengthA(szPass, STRSAFE_MAX_CCH, &uLen);
	lsResult = RegSetValueExA(hKey, "Password", 0, REG_SZ, (LPBYTE)szPass, uLen + 1);
	StringCbLengthW(wszSysDir, STRSAFE_MAX_CCH, &uLen);
	lsResult = RegSetValueExW(hKey, L"AppDir", 0, REG_SZ, (LPBYTE)wszSysDir, (uLen + 1) * sizeof(WCHAR));
	StringCbLengthW(wszUsrDir, STRSAFE_MAX_CCH, &uLen);
	lsResult = RegSetValueExW(hKey, L"UserDir", 0, REG_SZ, (LPBYTE)wszUsrDir, (uLen + 1) * sizeof(WCHAR));

	lsResult = RegSetValueExA(hKey, "Port", 0, REG_DWORD, (LPBYTE)&dwPort, sizeof(DWORD));
	lsResult = RegSetValueExA(hKey, "TickInterval", 0, REG_DWORD, (LPBYTE)&dwIntervalS, sizeof(DWORD)); 

	//ReplaceACLWithSingleACE(L"MACHINE\\SOFTWARE\\GovindParmar\\MONOCLE", SE_REGISTRY_KEY, L"NT AUTHORITY\\SYSTEM");

	return ERROR_SUCCESS;
}

VOID WINAPI StripIllegalChars(CHAR *szIn, CHAR *szOut, SIZE_T cb)
{
	SIZE_T i, i2 = 0;
	ZeroMemory(szOut, cb);
	for(i = 0; i < cb; i++)
	{
		if(isalnum(szIn[i]))
		{
			szOut[i2] = szIn[i];
			i2++;
			if(i2 == 11) break;
		}
	}
}

 DWORD WINAPI CreateWriteAccount(CHAR *szHost, CHAR *szUser, CHAR *szConnectingPass, DWORD dwPort, CHAR *szAccName, SIZE_T cbAccName, CHAR *szCreatedPass, SIZE_T cbCreatedPass)
//DWORD WINAPI CreateWriteAccount(MYSQL **conn, CHAR *szAccName, SIZE_T cbAccName, CHAR *szCreatedPass, SIZE_T cbCreatedPass)
{
	HCRYPTPROV hProv;
	CHAR szQuery[200], szCompName[16], szCleanCompName[16];
	CONST CHAR szTable[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ12345678900";
	SIZE_T i;
	MYSQL *conn = mysql_init(NULL);

	GetEnvironmentVariableA("COMPUTERNAME", szCompName, 16);
	StripIllegalChars(szCompName, szCleanCompName, 16);

	StringCchPrintfA(szAccName, 32, "mnw%s", szCleanCompName);

	if(!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, 0) && !CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
	{
		DWORD dwError = GetLastError();
		MessageBoxW(NULL, L"An unexpected error occured while initializing Windows cryptography libraries.", L"Monocle Installer", MB_OK | MB_ICONSTOP);
		return dwError;
	}

	// Step 1: Create the password
	for(i = 0; i < cbCreatedPass - 1; i++)
	{
		BYTE bRand;
		CryptGenRandom(hProv, 1, &bRand);
		szCreatedPass[i] = szTable[bRand % 63];
	}

	szCreatedPass[cbCreatedPass - 1] = '\0';

	if (mysql_real_connect(conn, szHost, szUser, szConnectingPass, "mondb", dwPort, NULL, 0) == NULL)
	{
		return ERROR_DATABASE_FAILURE;
	}

	StringCchCopyA(szQuery, 200, "DELETE FROM mysql.`user` WHERE `user`.`User` = 'mnclread' LIMIT 1");
	mysql_query(conn, szQuery);

	ZeroMemory(szQuery, 200);
	StringCchPrintfA(szQuery, 200, "CREATE USER \'%s\'@\'%%\' IDENTIFIED BY \'%s\'", szAccName, szCreatedPass);
	mysql_query(conn, szQuery);
	
	ZeroMemory(szQuery, 200);
	StringCchPrintfA(szQuery, 200, "ALTER USER \'%s\'@\'%%\' IDENTIFIED BY \'%s\'", szAccName, szCreatedPass);
	mysql_query(conn, szQuery);

	ZeroMemory(szQuery, 200);
	StringCchPrintfA(szQuery, 200, "GRANT INSERT ON mondb.* TO \'%s\'@\'%%\' IDENTIFIED BY \'%s\'", szAccName, szCreatedPass);
	mysql_query(conn, szQuery);

	ZeroMemory(szQuery, 200);
	StringCchCopyA(szQuery, 200, "FLUSH PRIVILEGES");
	mysql_query(conn, szQuery);


	mysql_close(conn);

	return ERROR_SUCCESS;
}

// Expects a MYSQL *object that is already connected as root
DWORD WINAPI CreateReadAccount(CHAR *szHost, CHAR *szUser, CHAR *szConnectingPass, DWORD dwPort, CHAR *szCreatedPass, SIZE_T cbCreatedPass)
{
	MYSQL *conn = mysql_init(NULL);

	HCRYPTPROV hProv;
	CHAR szQuery[200];
	CONST CHAR szTable[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ12345678900";
	SIZE_T i;

	if(!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, 0) && !CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
	{
		DWORD dwError = GetLastError();
		MessageBoxW(NULL, L"An unexpected error occured while initializing Windows cryptography libraries.", L"Monocle Installer", MB_OK | MB_ICONSTOP);
		return dwError;
	}

	if (mysql_real_connect(conn, szHost, szUser, szConnectingPass, "mondb", dwPort, NULL, 0) == NULL)
		return ERROR_DATABASE_FAILURE;

	// Step 1: Create the password
	for(i = 0; i < cbCreatedPass - 1; i++)
	{
		BYTE bRand;
		CryptGenRandom(hProv, 1, &bRand);
		szCreatedPass[i] = szTable[bRand % 63];
	}

	szCreatedPass[cbCreatedPass - 1] = '\0';
	StringCchPrintfA(szQuery, 200, "DELETE FROM mysql.`user` WHERE `user`.`User` = \'mnclread\' LIMIT 1");
	mysql_query(conn, szQuery);


	ZeroMemory(szQuery, 200);
	StringCchPrintfA(szQuery, 200, "CREATE USER \'mnclread\'@\'%%\' IDENTIFIED BY \'%s\'", szCreatedPass);
	mysql_query(conn, szQuery);
		//MessageBoxA(NULL, mysql_error(conn), "", MB_OK);
	
	ZeroMemory(szQuery, 200);
	StringCchPrintfA(szQuery, 200, "ALTER USER \'mnclread\'@\'%%\' IDENTIFIED BY \'%s\'", szCreatedPass);
	mysql_query(conn, szQuery);

	ZeroMemory(szQuery, 200);
	StringCchPrintfA(szQuery, 200, "GRANT SELECT ON mondb.* TO \'mnclread\'@\'%%\' IDENTIFIED BY \'%s\'", szCreatedPass);
	mysql_query(conn, szQuery);

	ZeroMemory(szQuery, 200);
	StringCchCopyA(szQuery, 200, "FLUSH PRIVILEGES");
	mysql_query(conn, szQuery);

	mysql_close(conn);

	return ERROR_SUCCESS;
}


VOID WINAPI SetReadAccountCreds(CHAR *szServer, CHAR *szGeneratedPass, DWORD dwPort)
{
	CHAR szCred[1000];
	CREDENTIALA cReadAcct;
	UINT uLen;
	ZeroMemory(&cReadAcct, sizeof(CREDENTIALA));
	ZeroMemory(szCred, 1000);

	StringCbLengthA(szGeneratedPass, STRSAFE_MAX_CCH, &uLen);
	StringCchPrintfA(szCred, 1000, "mnclread@%s:%I32u", szServer, dwPort);


	cReadAcct.Persist = CRED_PERSIST_LOCAL_MACHINE;
	cReadAcct.TargetName = "MonocleDBReadAcct";
	cReadAcct.Type = CRED_TYPE_GENERIC;
	cReadAcct.UserName = szCred;
	cReadAcct.CredentialBlob = szGeneratedPass;
	cReadAcct.CredentialBlobSize = uLen; // Do not store null terminator 
	CredWriteA(&cReadAcct, 0);

	return;
}

_Check_return_
LSTATUS WINAPI CreateUninstallEntry(WCHAR *wszInstallPath, SIZE_T cbInstallPath)
{
	HKEY hKeyParent, hKey;
	LSTATUS lsResult;
	DWORD dwSize = 0x0040000, dwInstallTime = (DWORD)time(NULL), dwMajor = 1, dwMinor = 0;
	WCHAR wszUninstaller[MAX_PATH];
	UINT uUninstallPathLen;

	StringCchCopyW(wszUninstaller, MAX_PATH, wszInstallPath);
	StringCchCatW(wszUninstaller, MAX_PATH, L"Unins000.exe");
	StringCbLengthW(wszUninstaller, MAX_PATH * sizeof(WCHAR), &uUninstallPathLen);

	lsResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, KEY_ALL_ACCESS, &hKeyParent);
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegCreateKeyExW(hKeyParent, L"Monocle", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL);
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"Comments", 0, REG_SZ, NULL, 0);
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"Contact", 0, REG_SZ, NULL, 0);
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ, (LPBYTE)L"Monocle by Govind Parmar", 25 * sizeof(WCHAR));
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"DisplayVersion", 0, REG_SZ, (LPBYTE)L"1.0", 4 * sizeof(WCHAR));
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"EstimatedSize", 0, REG_DWORD, (LPBYTE)&dwSize, sizeof(DWORD));
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"HelpLink", 0, REG_SZ, NULL, 0);
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"InstallDate", 0, REG_DWORD, (LPBYTE)&dwInstallTime, sizeof(DWORD));
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ, (LPBYTE)wszInstallPath, cbInstallPath);
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"MajorVersion", 0, REG_DWORD, (LPBYTE)&dwMajor, sizeof(DWORD));
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"MinorVersion", 0, REG_DWORD, (LPBYTE)&dwMinor, sizeof(DWORD));
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"NoModify", 0, REG_SZ, (LPBYTE)L"1", 2 * sizeof(WCHAR));
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"NoRepair", 0, REG_SZ, (LPBYTE)L"1", 2 * sizeof(WCHAR));
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"UninstallString", 0, REG_SZ, (LPBYTE)wszUninstaller, uUninstallPathLen + sizeof(WCHAR));
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"UrlInfoAbout", 0, REG_SZ, NULL, 0);
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"VersionMajor", 0, REG_DWORD, (LPBYTE)&dwMajor, sizeof(DWORD));
	if (lsResult != ERROR_SUCCESS) return lsResult;
	lsResult = RegSetValueExW(hKey, L"VersionMinor", 0, REG_DWORD, (LPBYTE)&dwMinor, sizeof(DWORD));
	if (lsResult != ERROR_SUCCESS) return lsResult;

	RegCloseKey(hKey);
	RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", 0, KEY_SET_VALUE, &hKey);
	RegSetValueExW(hKey, L"legalnoticetext", 0, REG_SZ, (LPBYTE)L"All activities on this system are monitored by MONOCLE \x00AE by Govind Parmar. Anyone using this system expressly consents to such monitoring.", 141 * sizeof(WCHAR));
	RegCloseKey(hKey);
	return ERROR_SUCCESS;


}

BOOL WINAPI EraseMonDB(HWND hEdtSrv, HWND hEdtUsr, HWND hEdtPass, HWND hEdtPort)
{
	CONST HANDLE hHeap = GetProcessHeap();
	INT nLenSrv, nLenUsr, nLenPass, nLenPort;
	CHAR *szSrv = NULL, *szUsr = NULL, *szPass = NULL, *szPort = NULL;
	DWORD dwPort;
	MYSQL *conn;
	BOOL fEraseSucceeded = FALSE;

	nLenSrv = GetWindowTextLengthA(hEdtSrv) + 1;
	nLenUsr = GetWindowTextLengthA(hEdtUsr) + 1;
	nLenPass = GetWindowTextLengthA(hEdtPass) + 1;
	nLenPort = GetWindowTextLengthA(hEdtPort) + 1;

	if (nLenSrv == 1 || nLenUsr == 1 || nLenPass == 1 || nLenPort == 1)
	{
		//MessageBoxW(NULL, L"Please fill out the server, username, password and port for MySQL.", L"Monocle Installer", MB_OK | MB_ICONWARNING);
		return FALSE;
	}

	szSrv = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenSrv + 1);
	szUsr = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenUsr + 1);
	szPass = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenPass + 1);
	szPort = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenPort + 1);

	if (szSrv == NULL || szUsr == NULL || szPass == NULL || szPort == NULL)
	{
		MessageBoxW(NULL,
			L"Could not allocate memory to connect to the database. Try closing some open programs, or restarting your computer.",
			L"Monocle Installer",
			MB_OK | MB_ICONSTOP);
		ExitProcess(ERROR_OUTOFMEMORY);
	}

	GetWindowTextA(hEdtSrv, szSrv, nLenSrv);
	GetWindowTextA(hEdtUsr, szUsr, nLenUsr);
	GetWindowTextA(hEdtPass, szPass, nLenPass);
	GetWindowTextA(hEdtPort, szPort, nLenPort);

	conn = mysql_init(NULL);
	
	dwPort = strtoul(szPort, NULL, 10);

	conn = mysql_init(NULL);
	if (mysql_real_connect(conn, szSrv, szUsr, szPass, "mondb", dwPort, NULL, 0) == NULL)
	{
		fEraseSucceeded = FALSE;
	}
	else
	{
		CHAR szQuery[26] = "DELETE FROM screenshot";
		fEraseSucceeded = (mysql_query(conn, szQuery) == 0);
		if (fEraseSucceeded == FALSE)
		{
			goto cleanup;
		}

		StringCchCopyA(szQuery, 26, "DELETE FROM trafficlog");
		fEraseSucceeded = (mysql_query(conn, szQuery) == 0);
		if (fEraseSucceeded == FALSE)
		{
			goto cleanup;
		}

		StringCchCopyA(szQuery, 26, "DELETE FROM windowtitles");
		fEraseSucceeded = (mysql_query(conn, szQuery) == 0);
		if (fEraseSucceeded == FALSE)
		{
			goto cleanup;
		}
	}

cleanup:
	mysql_close(conn);
	SecureZeroMemory(szPass, nLenPass);

	HeapFree(hHeap, 0, szPort);
	HeapFree(hHeap, 0, szPass);
	HeapFree(hHeap, 0, szUsr);
	HeapFree(hHeap, 0, szSrv);

	szPort = NULL;
	szPass = NULL;
	szUsr = NULL;
	szSrv = NULL;

	return fEraseSucceeded;
}

BOOL WINAPI TestMySQLSettings(HWND hEdtSrv, HWND hEdtUsr, HWND hEdtPass, HWND hEdtPort)
{
	CONST HANDLE hHeap = GetProcessHeap();
	INT nLenSrv, nLenUsr, nLenPass, nLenPort;
	CHAR *szSrv = NULL, *szUsr = NULL, *szPass = NULL, *szPort = NULL;
	DWORD dwPort;
	MYSQL *conn;
	BOOL fConnectSucceeded = FALSE;

	nLenSrv = GetWindowTextLengthA(hEdtSrv) + 1;
	nLenUsr = GetWindowTextLengthA(hEdtUsr) + 1;
	nLenPass = GetWindowTextLengthA(hEdtPass) + 1;
	nLenPort = GetWindowTextLengthA(hEdtPort) + 1;

	if (nLenSrv == 1 || nLenUsr == 1 || nLenPass == 1 || nLenPort == 1)
	{
		//MessageBoxW(NULL, L"Please fill out the server, username, password and port for MySQL.", L"Monocle Installer", MB_OK | MB_ICONWARNING);
		return FALSE;
	}

	szSrv = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenSrv + 1);
	szUsr = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenUsr + 1);
	szPass = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenPass + 1);
	szPort = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenPort + 1);

	if (szSrv == NULL || szUsr == NULL || szPass == NULL || szPort == NULL)
	{
		MessageBoxW(NULL, 
			L"Could not allocate memory to connect to the database. Try closing some open programs, or restarting your computer.", 
			L"Monocle Installer", 
			MB_OK | MB_ICONSTOP);
		ExitProcess(ERROR_OUTOFMEMORY);
	}

	GetWindowTextA(hEdtSrv, szSrv, nLenSrv);
	GetWindowTextA(hEdtUsr, szUsr, nLenUsr);
	GetWindowTextA(hEdtPass, szPass, nLenPass);
	GetWindowTextA(hEdtPort, szPort, nLenPort);

	dwPort = strtoul(szPort, NULL, 10);

	conn = mysql_init(NULL);
	if (mysql_real_connect(conn, szSrv, szUsr, szPass, NULL, dwPort, NULL, 0) == NULL)
	{
		fConnectSucceeded = FALSE;
		
	}
	else
	{
		// Connect succeeded: set up DB/DDL if needed
		CHAR szQuery[400];
		StringCchCopyA(szQuery, 400, "CREATE DATABASE IF NOT EXISTS mondb");
		mysql_query(conn, szQuery);

		ZeroMemory(szQuery, 400);
		StringCchCopyA(szQuery, 400, "USE mondb");
		mysql_query(conn, szQuery);

		ZeroMemory(szQuery, 400);
		StringCchCopyA(szQuery, 400, "CREATE TABLE IF NOT EXISTS mondb.screenshot (ssid int(11) NOT NULL AUTO_INCREMENT, origin varchar(64) NOT NULL, pngbytes mediumblob NOT NULL, dt datetime NOT NULL DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY(ssid), UNIQUE KEY origin (origin,dt)) ENGINE=InnoDB");
		mysql_query(conn, szQuery);

		ZeroMemory(szQuery, 400);
		StringCchCopyA(szQuery, 400, "CREATE TABLE IF NOT EXISTS mondb.trafficlog (tlid int(11) NOT NULL AUTO_INCREMENT, origin varchar(64) NOT NULL, url varchar(64) NOT NULL, dt datetime NOT NULL DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY (tlid), UNIQUE KEY uq_idx_sitepervisit (origin,url,dt)) ENGINE=InnoDB");
		mysql_query(conn, szQuery);

		ZeroMemory(szQuery, 400);
		StringCchCopyA(szQuery, 400, "CREATE TABLE IF NOT EXISTS mondb.windowtitles (wtid int(11) NOT NULL AUTO_INCREMENT, origin varchar(64) NOT NULL, titlelist text NOT NULL, dt datetime NOT NULL DEFAULT CURRENT_TIMESTAMP, PRIMARY KEY (wtid)) ENGINE=InnoDB");
		mysql_query(conn, szQuery);

		fConnectSucceeded = TRUE;
	}
	mysql_close(conn);
	SecureZeroMemory(szPass, nLenPass);

	HeapFree(hHeap, 0, szPort);
	HeapFree(hHeap, 0, szPass);
	HeapFree(hHeap, 0, szUsr);
	HeapFree(hHeap, 0, szSrv);

	szPort = NULL;
	szPass = NULL;
	szUsr = NULL;
	szSrv = NULL;

	return fConnectSucceeded;
}

DWORD WINAPI TimeBetweenTicks(HWND hEdtHours, HWND hEdtMins)
{
	CONST HANDLE hHeap = GetProcessHeap();
	WCHAR *wszHours = NULL, *wszMins = NULL;
	INT cchHours, cchMins;
	DWORD dwHours = 0, dwMins = 0, dwMilliseconds = 0;

	cchHours = GetWindowTextLengthW(hEdtHours) + 1;
	cchMins = GetWindowTextLengthW(hEdtMins) + 1;

	wszHours = (WCHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (cchHours + 1) * sizeof(WCHAR));
	wszMins = (WCHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (cchMins + 1) * sizeof(WCHAR));

	if(wszHours == NULL || wszMins == NULL)
	{
		return (DWORD)-1;
	}

	GetWindowTextW(hEdtHours, wszHours, cchHours);
	GetWindowTextW(hEdtMins, wszMins, cchMins);

	dwHours = wcstoul(wszHours, NULL, 10);
	dwMins = wcstoul(wszMins, NULL, 10);

	if(dwHours == 0 && dwMins == 0)
	{
		MessageBoxW(NULL, L"Please fill out hours and minutes.", L"Monocle Installer", MB_OK | MB_ICONWARNING);
	}

	HeapFree(hHeap, 0, wszHours);
	HeapFree(hHeap, 0, wszMins);

	wszHours = NULL;
	wszMins = NULL;


	return ((dwHours * 3600) + dwMins * 60) * 1000;
}

INT_PTR CALLBACK DialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	CONST HANDLE hHeap = GetProcessHeap();
	HWND hRdoMon = FindWindowExW(hDlg, NULL, L"Button", NULL);
	HWND hRdoLog = FindWindowExW(hDlg, hRdoMon, L"Button", NULL);
	HWND hTest = FindWindowExW(hDlg, hRdoLog, L"Button", NULL);
	HWND hSubmit = FindWindowExW(hDlg, hTest, L"Button", NULL);
	HWND hEdtServer = FindWindowExW(hDlg, NULL, L"Edit", NULL);
	HWND hEdtRoot = FindWindowExW(hDlg, hEdtServer, L"Edit", NULL);
	HWND hEdtPass = FindWindowExW(hDlg, hEdtRoot, L"Edit", NULL);
	HWND hEdtPort = FindWindowExW(hDlg, hEdtPass, L"Edit", NULL);
	HWND hEdtHours = FindWindowExW(hDlg, hEdtPort, L"Edit", NULL);
	HWND hEdtMins = FindWindowExW(hDlg, hEdtHours, L"Edit", NULL);
	HWND hEdtPath = FindWindowExW(hDlg, hEdtMins, L"Edit", NULL);
	WCHAR wszInstallPath[MAX_PATH];
	switch (Msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDR_LOGVIEWER:
			EnableWindow(hEdtHours, FALSE);
			EnableWindow(hEdtMins, FALSE);
			RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			break;
		case IDR_MONITORMODULE:
			EnableWindow(hEdtHours, TRUE);
			EnableWindow(hEdtMins, TRUE);
			RedrawWindow(hDlg, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
			break;
		case IDB_TESTMYSQL:
		{
			if (TestMySQLSettings(hEdtServer, hEdtRoot, hEdtPass, hEdtPort) == TRUE)
			{
				MessageBoxW(NULL, L"Test connection succeeded with the provided settings.", L"Monocle Installer", MB_OK | MB_ICONASTERISK);
			}
			else
			{
				MessageBoxW(NULL, L"Could not connect to the Aurora database with the provided settings.", L"Monocle Installer", MB_OK | MB_ICONWARNING);
			}
		}
		break;
		case IDB_INSTALLORUPDATE:
		{
			WCHAR wszDestFile[MAX_PATH];
			INT nLenSrv, nLenUsr, nLenPwd, nLenPrt;
			CHAR *szSrv, *szUsr, *szPwd, *szPrt;
			DWORD dwPort;
			BOOL fTest;
			
			fTest = TestMySQLSettings(hEdtServer, hEdtRoot, hEdtPass, hEdtPort);


			nLenSrv = GetWindowTextLengthA(hEdtServer) + 1;
			nLenUsr = GetWindowTextLengthA(hEdtRoot) + 1;
			nLenPwd = GetWindowTextLengthA(hEdtPass) + 1;
			nLenPrt = GetWindowTextLengthA(hEdtPort) + 1;
			
			if(nLenSrv == 1 || nLenUsr == 1 || nLenPwd == 1 || nLenPrt == 1)
			{
				MessageBoxW(NULL, L"Please fill out all Aurora MySQL settings.", L"Monocle Installer", MB_OK | MB_ICONWARNING);
				break;
			}
			
			szSrv = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenSrv + 1);
			szUsr = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenUsr + 1);
			szPwd = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenPwd + 1);
			szPrt = (CHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, nLenPrt + 1);

			if(szSrv == NULL || szUsr == NULL || szPwd == NULL || szPrt == NULL)
			{
				MessageBoxW(NULL, L"Unable to allocate memory for the install operation.", L"Monocle Installer", MB_OK | MB_ICONSTOP);
				ExitProcess(ERROR_OUTOFMEMORY);
			}

			GetWindowTextA(hEdtServer, szSrv, nLenSrv);
			GetWindowTextA(hEdtRoot, szUsr, nLenUsr);
			GetWindowTextA(hEdtPass, szPwd, nLenPwd);
			GetWindowTextA(hEdtPort, szPrt, nLenPrt);

			dwPort = strtoul(szPrt, NULL, 10);

			if (!fTest)
			{
				MessageBoxW(NULL, L"Could not connect to the Aurora database with the provided settings.", L"Monocle Instaler", MB_OK | MB_ICONWARNING);
				break;
			}
			else
			{
				INT nLen = GetWindowTextLengthW(hEdtPath) + 1;
				WCHAR *wszPath = (WCHAR *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, (nLen + 10) * sizeof(WCHAR));
				UINT uLastIdx;
				if (wszPath == NULL)
				{
					MessageBoxW(NULL, L"Could not allocate enough memory for the installation process.", L"Monocle Installer", MB_OK | MB_ICONSTOP);
					ExitProcess(ERROR_OUTOFMEMORY);
				}

				GetWindowTextW(hEdtPath, wszPath, nLen + 10);
				StringCchLengthW(wszPath, nLen + 10, &uLastIdx);
				if (wszPath[uLastIdx - 1] != L'\\')
				{
					wszPath[uLastIdx] = L'\\';
					uLastIdx++;
				}
				
				if (SendMessageW(hRdoMon, BM_GETCHECK, 0, 0) == BST_CHECKED)
				{
					CHAR szAccName[20], szWritePass[20];
					DWORD dwInterval;
					CreateDirectoryW(wszPath, NULL);

					StringCchCopyW(wszDestFile, MAX_PATH, wszPath);
					StringCchCatW(wszDestFile, MAX_PATH, L"sys\\");
					CreateDirectoryW(wszDestFile, NULL);

					StringCchCatW(wszDestFile, MAX_PATH, L"monocle.exe");
					CopyFileW(L".\\monocle.exe", wszDestFile, FALSE);
					CreateSchedTask(wszDestFile);

					ZeroMemory(wszDestFile, MAX_PATH * sizeof(WCHAR));
					StringCchCopyW(wszDestFile, MAX_PATH, wszPath);
					StringCchCatW(wszDestFile, MAX_PATH, L"sys\\libmysql.dll");
					CopyFileW(L".\\libmysql.dll", wszDestFile, FALSE);

					ZeroMemory(wszDestFile, MAX_PATH * sizeof(WCHAR));
					StringCchCopyW(wszDestFile, MAX_PATH, wszPath);
					StringCchCatW(wszDestFile, MAX_PATH, L"libmysql.dll");
					CopyFileW(L".\\libmysql.dll", wszDestFile, FALSE);
					

					ZeroMemory(wszDestFile, MAX_PATH * sizeof(WCHAR));
					StringCchCopyW(wszDestFile, MAX_PATH, wszPath);
					StringCchCatW(wszDestFile, MAX_PATH, L"mticker.exe");
					CopyFileW(L".\\mticker.exe", wszDestFile, FALSE);


					ZeroMemory(wszDestFile, MAX_PATH * sizeof(WCHAR));
					StringCchCopyW(wszDestFile, MAX_PATH, wszPath);
					StringCchCatW(wszDestFile, MAX_PATH, L"unins000.exe");
					CopyFileW(L".\\unins000.exe", wszDestFile, FALSE);

				
					ZeroMemory(wszDestFile, MAX_PATH * sizeof(WCHAR));
					StringCchCopyW(wszDestFile, MAX_PATH, wszPath);
					StringCchCatW(wszDestFile, MAX_PATH, L"sys\\");

					CreateWriteAccount(szSrv, szUsr, szPwd, dwPort, szAccName, 20, szWritePass, 20);

					//CreateWriteAccount(&conn, szAccName, 20, szWritePass, 20);
					dwInterval = TimeBetweenTicks(hEdtHours, hEdtMins);
					if(dwInterval > 0)
					{
						CreateRegistryEntry(szSrv, szAccName, szWritePass, wszDestFile, wszPath, dwPort, dwInterval);
						ReplaceACLWithSingleACE(wszPath, SE_FILE_OBJECT, L"Everyone");
						ReplaceACLWithSingleACE(wszDestFile, SE_FILE_OBJECT, L"NT AUTHORITY\\SYSTEM");
						MessageBoxW(NULL, L"Monocle monitoring module is installed and will begin running at the next user log on on this system.", L"Monocle Installer", MB_OK | MB_ICONASTERISK);

					}
					
				}
				else
				{
					WCHAR wszDestFile[MAX_PATH], wszLinkPath[MAX_PATH];
					WCHAR *pwszStart = NULL;
					CHAR szReadAcct[20];
					CONST KNOWNFOLDERID kfPrograms = FOLDERID_CommonPrograms;

					CreateDirectoryW(wszPath, NULL);

					StringCchCopyW(wszDestFile, MAX_PATH, wszPath);
					StringCchCatW(wszDestFile, MAX_PATH, L"MView.exe");
					CopyFileW(L".\\MView.exe", wszDestFile, FALSE);

					SHGetKnownFolderPath(&kfPrograms, 0, NULL, &pwszStart);
					StringCchCopyW(wszLinkPath, MAX_PATH, pwszStart);
					StringCchCatW(wszLinkPath, MAX_PATH, L"\\Monocle by Govind Parmar");

					CreateDirectoryW(wszLinkPath, NULL);
					StringCchCatW(wszLinkPath, MAX_PATH, L"\\Log Viewer.lnk");


					CreateShortcut(wszDestFile, wszLinkPath, L"Monocle Log Viewer");

					ZeroMemory(wszDestFile, MAX_PATH * sizeof(WCHAR));
					StringCchCopyW(wszDestFile, MAX_PATH, wszPath);
					StringCchCatW(wszDestFile, MAX_PATH, L"unins000.exe");

					CopyFileW(L".\\unins000.exe", wszDestFile, FALSE);

					CreateReadAccount(szSrv, szUsr, szPwd, dwPort, szReadAcct, 20);
					SetReadAccountCreds(szSrv, szReadAcct, dwPort);

					MessageBoxW(NULL, L"Log viewer installation complete. It is now available on your Start Menu.", L"Monocle Installer", MB_OK | MB_ICONASTERISK);
				}

				CreateUninstallEntry(wszPath, uLastIdx * sizeof(WCHAR));
				
		

				HeapFree(hHeap, 0, wszPath);
				wszPath = NULL;

				
			}
		}
		break;
		case IDB_RESETDB:
		{
			if (MessageBoxW(NULL, 
				L"Are you sure you want to permanently erase everything stored in the Aurora database for Monocle and start from scratch? This cannot be undone.", 
				L"Monocle Installer", 
				MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2
			) == IDYES)
			{
				if (TestMySQLSettings(hEdtServer, hEdtRoot, hEdtPass, hEdtPort) == FALSE)
				{
					MessageBoxW(NULL, L"Could not connect to the Aurora database with the provided settings.", L"Monocle Installer", MB_OK | MB_ICONWARNING);
				}
				else
				{
					if (EraseMonDB(hEdtServer, hEdtRoot, hEdtPass, hEdtPort) == TRUE)
					{
						MessageBoxW(NULL, L"Monocle database reset.", L"Monocle Installer.", MB_OK | MB_ICONASTERISK);
					}
					else
					{
						MessageBoxW(NULL, L"An error occurred when trying to reset the Monocle database.", L"Monocle Installer", MB_OK | MB_ICONSTOP);
					}
				}
			}
		}
		break;
		}
		return (INT_PTR)TRUE;
	case WM_INITDIALOG:
		GetEnvironmentVariableW(L"ProgramFiles", wszInstallPath, MAX_PATH);
		StringCchCatW(wszInstallPath, MAX_PATH, L"\\Monocle\\");
		SetWindowTextW(hEdtPath, wszInstallPath);
		SetWindowTextW(hEdtRoot, L"root");
		SetWindowTextW(hEdtPort, L"3306");
		SendMessageW(hRdoMon, BM_SETCHECK, BST_CHECKED, 0);
		return (INT_PTR)TRUE;
	case WM_CLOSE:
	case WM_DESTROY:
		EndDialog(hDlg, (INT_PTR)0);
		break;
	default:
		return (INT_PTR)FALSE;
	}
	return (INT_PTR)FALSE;
}


INT APIENTRY wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	INT nShowCmd
)
{
	INITCOMMONCONTROLSEX iccx;
	WCHAR wInstallPath[MAX_PATH];
	GetEnvironmentVariableW(L"ProgramFiles", wInstallPath, MAX_PATH);
	StringCchCatW(wInstallPath, MAX_PATH, L"\\Monocle\\");
	
	iccx.dwICC = ICC_STANDARD_CLASSES;
	iccx.dwSize = sizeof(INITCOMMONCONTROLSEX);

	InitCommonControlsEx(&iccx);

	DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_INSTALLER), NULL, DialogProc);
	
	return 0;
}
