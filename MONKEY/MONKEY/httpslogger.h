#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <mstcpip.h>
#include <strsafe.h>
#pragma comment(lib, "ws2_32.lib")

// Big endian to little endian (16-bits)
#define BE2LE_16(w) ((WORD) ((HIBYTE(w) | (LOBYTE(w) << 8))))

#define PROTO_TCP 6

#pragma pack(push, 1)
typedef struct _IP_HEADER
{
	BYTE bLen : 4;
	BYTE bVer : 4;
	BYTE bTyp;
	WORD wPktLen;
	WORD wID;
	BYTE bOffset : 5;
	BYTE bFragMore : 1;
	BYTE bDontFrag : 1;
	BYTE bReserved : 1;
	BYTE bFragOffset;
	BYTE bTTL;
	BYTE bProto;
	WORD wChksum;
	DWORD dwSrc;
	DWORD dwDest;
} IP_HEADER, *PIP_HEADER;

typedef struct _TCP_HEADER
{
	WORD wSrcPort;
	WORD wDestPort;
	DWORD dwSequence;
	DWORD dwAcknowledge;
	BYTE bNonce : 1;
	BYTE bReserved : 3;
	BYTE bDataOffset : 4;
	BYTE bFlags; /* FSRPAUEC */
	WORD wWindow;
	WORD wChksum;
	WORD pwUrgent;
} TCP_HEADER, *PTCP_HEADER;
#pragma pack(pop)

typedef struct _WEBLOGGER_INFO
{
	HANDLE hFile;
	DWORD dwInterfaceIndex;
} WEBLOGGER_INFO, *PWEBLOGGER_INFO;

DWORD CALLBACK TrafficSniffProc(LPVOID lpWLI);