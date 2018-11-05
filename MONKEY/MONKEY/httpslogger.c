#include "httpslogger.h"

// (PWEBLOGGER_INFO)lpWLI->hFile must point to a file opened with access
// mode GENERIC_WRITE and share mode FILE_SHARE_READ (at least!)
//
// Experiment with launching 1 of these thread procs per interface
DWORD CALLBACK TrafficSniffProc(LPVOID lpWLI)
{
	WSADATA wsa;
	SOCKET sGlob;
	WEBLOGGER_INFO wli = *(PWEBLOGGER_INFO)lpWLI;
	HANDLE hFile = wli.hFile, hHeap = GetProcessHeap();
	struct sockaddr_in dest;
	DWORD dwInterface = wli.dwInterfaceIndex, i = 1;
	INT nRead = INT_MAX;
	CHAR szHost[100];
	BYTE *bBuffer = NULL;
	struct hostent *local;

	WSAStartup(MAKEWORD(2, 2), &wsa);
	sGlob = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	gethostname(szHost, 100);
	local = gethostbyname(szHost);

	ZeroMemory(&dest, sizeof(struct sockaddr_in));
	CopyMemory(&dest.sin_addr.S_un.S_addr, local->h_addr_list[0], sizeof(ULONG));

	dest.sin_family = AF_INET;
	dest.sin_port = 0;

	bind(sGlob, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
	WSAIoctl(sGlob, SIO_RCVALL, &i, sizeof(int), 0, 0, (DWORD *)&dwInterface, NULL, NULL);
	bBuffer = (BYTE *)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 65536);
	if(bBuffer == NULL)
	{
		return ERROR_OUTOFMEMORY;
	}

	while(1)
	{
		nRead = recvfrom(sGlob, bBuffer, 65536, 0, NULL, NULL);
		if(nRead > 0)
		{
			IP_HEADER ipHeader;
			CopyMemory(&ipHeader, bBuffer, sizeof(IP_HEADER));
			if(ipHeader.bProto == PROTO_TCP)
			{
				TCP_HEADER tcpHeader;
				BYTE *pTcpHeader = (bBuffer + ipHeader.bLen * 4), *pData = NULL;
				CHAR szAddress[200];

				CopyMemory(&tcpHeader, pTcpHeader, sizeof(TCP_HEADER));

				// HTTPS: url is asciz @ payload + 134
				if(BE2LE_16(tcpHeader.wDestPort) == 443)
				{
					DWORD dwStart;
					pData = pTcpHeader + (tcpHeader.bDataOffset * 4);
					CopyMemory(&dwStart, pData, 4);

					if(dwStart == 0x02010316)
					{
						DWORD dwWritten;
						UINT uLen;
						StringCchPrintfA(szAddress, 200, "%s\r\n", pData + 134);
						StringCbLengthA(szAddress, 200, &uLen);
						SetFilePointer(hFile, 0, NULL, FILE_END);
						WriteFile(hFile, szAddress, uLen, &dwWritten, NULL);
						FlushFileBuffers(hFile);
					}
				}
				// HTTP: just search the header for Host: 
				else if(BE2LE_16(tcpHeader.wDestPort) == 80 || BE2LE_16(tcpHeader.wDestPort) == 81)
				{
					BYTE *bPos = 0;
					pData = pTcpHeader + (tcpHeader.bDataOffset * 4);
					if(strncmp(pData, "GET", 3) == 0)
					{
						if((bPos = strstr(pData, "Host: ")) != NULL)
						{
							CONST CHAR CRLF[2] = { '\r', '\n' };
							CHAR *pAddress = &szAddress[0];
							DWORD dwWritten;
							UINT uLen;
							bPos += 5;

							// Strip the trailing '\r' from the URL
							while(*bPos != '\r')
							{
								*pAddress = *bPos;
								pAddress++;
								bPos++;
							}
							*pAddress = '\0';

							StringCbLengthA(szAddress, 200, &uLen);
							SetFilePointer(hFile, 0, NULL, FILE_END);
							WriteFile(hFile, szAddress, uLen, &dwWritten, NULL);
							WriteFile(hFile, CRLF, 2, &dwWritten, NULL);
							FlushFileBuffers(hFile);
						}
					}
				}
			}
		}
	}

	// Cleanup code is redundant.
	HeapFree(hHeap, 0, bBuffer);
	bBuffer = NULL;
	CloseHandle(hFile);

	return ERROR_SUCCESS;
}