// winscard.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#define FUZZBUFSIZE 8120
static char fuzzbuffer[FUZZBUFSIZE];
static size_t fuzzlen;
static size_t fuzzoffset;

char *atr = (char *) "\x3B\xFE\x18\x00\x00\x80\x31\xFE\x45\x45\x73\x74\x45\x49\x44\x20\x76\x65\x72\x20\x31\x2E\x30\xA8";
size_t atrlen = sizeof(atr) - 1;

#define WREADER L"Yubico"
#define AREADER "Yubico"

#ifdef DEBUG
#define LOGFUNC() printf("Enter: %s\n", __FUNCTION__)
#else
#define LOGFUNC()
#endif

// from OpenSC sc.c
static int sc_hex_to_bin(const char *in, unsigned char *out, size_t *outlen)
{
	int err = 0;
	size_t left, count = 0, in_len;

	if (in == NULL || out == NULL || outlen == NULL) {
		return -1;
	}
	left = *outlen;
	in_len = strlen(in);

	while (*in != '\0') {
		int byte = 0, nybbles = 2;

		while (nybbles-- && *in && *in != ':' && *in != ' ') {
			char c;
			byte <<= 4;
			c = *in++;
			if ('0' <= c && c <= '9')
				c -= '0';
			else
				if ('a' <= c && c <= 'f')
					c = c - 'a' + 10;
				else
					if ('A' <= c && c <= 'F')
						c = c - 'A' + 10;
					else {
						err = -1;
						goto out;
					}
					byte |= c;
		}

		/* Detect premature end of string before byte is complete */
		if (in_len > 1 && *in == '\0' && nybbles >= 0) {
			err = -1;
			break;
		}

		if (*in == ':' || *in == ' ')
			in++;
		if (left <= 0) {
			err = -1;
			break;
		}
		out[count++] = (unsigned char)byte;
		left--;
	}

out:
	*outlen = count;
	return err;
}

WINSCARDAPI const SCARD_IO_REQUEST
g_rgSCardT0Pci,
g_rgSCardT1Pci,
g_rgSCardRawPci;

LONG init() {
	char *filename;
	FILE *f;

	if (fuzzlen == 0) {
		/* setup input from afl or other fuzzer */
		filename = getenv("FUZZ_FILE");
		if (!filename)
			filename = (char *)"input.apdu";

		f = fopen(filename, "rb");
		if (!f)
			return SCARD_E_NO_MEMORY;

		fuzzlen = fread(fuzzbuffer, 1, sizeof(fuzzbuffer), f);
		fclose(f);
		fuzzoffset = 0;
	}
	return SCARD_S_SUCCESS;
}

LONG get_fuzz_bytes(char *rbuf, size_t rsize) {
	if (init() != SCARD_S_SUCCESS)
		return SCARD_E_NO_MEMORY;

	if (fuzzoffset >= fuzzlen)
		return SCARD_E_NO_MEMORY;

	if (fuzzoffset + rsize >= fuzzlen)
		return SCARD_E_NO_MEMORY;

	memcpy(rbuf, fuzzbuffer + fuzzoffset, rsize);

	fuzzoffset += rsize;

	return SCARD_S_SUCCESS;
}

//SCardEstablishContext
WINSCARDAPI LONG WINAPI SCardEstablishContext(
	_In_  DWORD dwScope,
	_Reserved_  LPCVOID pvReserved1,
	_Reserved_  LPCVOID pvReserved2,
	_Out_ LPSCARDCONTEXT phContext) {

//	LOGFUNC();
	if (phContext)
		*phContext = 0x654321;
	return SCARD_S_SUCCESS;
}

//SCardReleaseContext
WINSCARDAPI LONG WINAPI SCardReleaseContext(SCARDCONTEXT hContext) {
//	LOGFUNC();
	return SCARD_S_SUCCESS;
}

//SCardIsValidContext
WINSCARDAPI LONG WINAPI SCardIsValidContext(SCARDCONTEXT hContext) {
//	LOGFUNC();
	return SCARD_S_SUCCESS;
}

//SCardConnect
WINSCARDAPI LONG WINAPI SCardConnect(SCARDCONTEXT hContext,
	LPCSTR szReader,
	DWORD dwShareMode,
	DWORD dwPreferredProtocols,
	/*@out@*/ LPSCARDHANDLE phCard, /*@out@*/ LPDWORD pdwActiveProtocol) {

	LOGFUNC();
	if (init() != SCARD_S_SUCCESS)
		return SCARD_E_NO_MEMORY;

	if (phCard)
		*phCard = 0x123456;	// arbitrary handle

	if (pdwActiveProtocol)
		*pdwActiveProtocol = 2;	// arbitrary protocol

	return SCARD_S_SUCCESS;
}

//SCardReconnect
WINSCARDAPI LONG WINAPI SCardReconnect(SCARDHANDLE hCard,
	DWORD dwShareMode,
	DWORD dwPreferredProtocols,
	DWORD dwInitialization, /*@out@*/ LPDWORD pdwActiveProtocol) {

	LOGFUNC();
	if (pdwActiveProtocol)
		*pdwActiveProtocol = 2;	// arbitrary protocol

	return SCARD_S_SUCCESS;
}

//SCardDisconnect
WINSCARDAPI LONG WINAPI SCardDisconnect(SCARDHANDLE hCard, DWORD dwDisposition) {
	LOGFUNC();
	return SCARD_S_SUCCESS;
}

//SCardBeginTransaction
WINSCARDAPI LONG WINAPI SCardBeginTransaction(SCARDHANDLE hCard) {
	LOGFUNC();
	return SCARD_S_SUCCESS;
}

//SCardEndTransaction
WINSCARDAPI LONG WINAPI SCardEndTransaction(SCARDHANDLE hCard, DWORD dwDisposition) {
	LOGFUNC();
	return SCARD_S_SUCCESS;
}

WINSCARDAPI LONG WINAPI SCardStatusW(SCARDHANDLE hCard,
	/*@null@*/ /*@out@*/ LPWSTR mszReaderNames,
	/*@null@*/ /*@out@*/ LPDWORD pcchReaderLen,
	/*@null@*/ /*@out@*/ LPDWORD pdwState,
	/*@null@*/ /*@out@*/ LPDWORD pdwProtocol,
	/*@null@*/ /*@out@*/ LPBYTE pbAtr,
	/*@null@*/ /*@out@*/ LPDWORD pcbAtrLen) {

	LOGFUNC();
	if (init() != SCARD_S_SUCCESS)
		return SCARD_E_NO_MEMORY;

	if (mszReaderNames && pcchReaderLen && *pcchReaderLen > wcslen(WREADER)) {
		memset(mszReaderNames, 0, *pcchReaderLen);
		wcscpy(mszReaderNames, WREADER);
	}
	if (pcchReaderLen)
		*pcchReaderLen = wcslen(WREADER) + 2;

	if (pdwState)
		*pdwState = SCARD_PRESENT;

	if (pdwProtocol)
		*pdwProtocol = 2;

	if (pbAtr && pcbAtrLen && *pcbAtrLen >= atrlen) {
		memset(pbAtr, 0, *pcbAtrLen);
		memcpy(pbAtr, atr, atrlen);
	}

	if (pcbAtrLen)
		*pcbAtrLen = atrlen;

	return SCARD_S_SUCCESS;

}

//SCardStatus
WINSCARDAPI LONG WINAPI SCardStatusA(SCARDHANDLE hCard,
	/*@null@*/ /*@out@*/ LPSTR mszReaderName,
	/*@null@*/ /*@out@*/ LPDWORD pcchReaderLen,
	/*@null@*/ /*@out@*/ LPDWORD pdwState,
	/*@null@*/ /*@out@*/ LPDWORD pdwProtocol,
	/*@null@*/ /*@out@*/ LPBYTE pbAtr,
	/*@null@*/ /*@out@*/ LPDWORD pcbAtrLen) {

	LOGFUNC();
	if (init() != SCARD_S_SUCCESS)
		return SCARD_E_NO_MEMORY;

	if (mszReaderName && pcchReaderLen && *pcchReaderLen > strlen(AREADER)) {
		memset(mszReaderName, 0, *pcchReaderLen);
		strcpy(mszReaderName, AREADER);
	}
	if (pcchReaderLen)
		*pcchReaderLen = strlen(AREADER) + 2;

	if (pdwState)
		*pdwState = SCARD_PRESENT;

	if (pdwProtocol)
		*pdwProtocol = 2;

	if (pbAtr && pcbAtrLen && *pcbAtrLen >= atrlen) {
		memcpy(pcbAtrLen, atr, atrlen);
	}

	if (pcbAtrLen)
		*pcbAtrLen = atrlen;

	return SCARD_S_SUCCESS;
}

// SCardGetStatusChange
WINSCARDAPI LONG WINAPI SCardGetStatusChange(SCARDCONTEXT hContext,
	DWORD dwTimeout,
	SCARD_READERSTATE *rgReaderStates, DWORD cReaders) {

	LOGFUNC();
	if (init() != SCARD_S_SUCCESS)
		return SCARD_E_NO_MEMORY;

	if (cReaders < 1)
		return SCARD_E_NO_MEMORY;

	rgReaderStates[0].cbAtr = atrlen;
	memset(rgReaderStates[0].rgbAtr, 0, sizeof(rgReaderStates[0].rgbAtr));
	memcpy(rgReaderStates[0].rgbAtr, atr, atrlen);

	if (rgReaderStates[0].dwCurrentState & SCARD_STATE_PRESENT) {
		rgReaderStates[0].dwEventState = rgReaderStates[0].dwCurrentState | SCARD_STATE_PRESENT;
	} else {
		rgReaderStates[0].dwEventState = rgReaderStates[0].dwCurrentState | SCARD_STATE_PRESENT | SCARD_STATE_CHANGED;
	}

	return SCARD_S_SUCCESS;
}

// SCardControl
WINSCARDAPI LONG WINAPI SCardControl(SCARDHANDLE hCard, DWORD dwControlCode,
	LPCVOID pbSendBuffer, DWORD cbSendLength,
	/*@out@*/ LPVOID pbRecvBuffer, DWORD cbRecvLength,
	LPDWORD lpBytesReturned) {
	size_t rsize = cbRecvLength;
	char *rbuf = (char *) pbRecvBuffer;

	LOGFUNC();
	memset(rbuf, 0, rsize);
	*lpBytesReturned = 0;
	return SCARD_S_SUCCESS;

}

//SCardTransmit
WINSCARDAPI LONG WINAPI SCardTransmit(SCARDHANDLE hCard,
	const SCARD_IO_REQUEST *pioSendPci,
	LPCBYTE pbSendBuffer, DWORD cbSendLength,
	/*@out@*/ SCARD_IO_REQUEST *pioRecvPci,
	/*@out@*/ LPBYTE pbRecvBuffer, LPDWORD pcbRecvLength) {
	size_t rsize = *pcbRecvLength;
	char *rbuf = (char *) pbRecvBuffer;

	LOGFUNC();
	return get_fuzz_bytes(rbuf, rsize);
}

WINSCARDAPI LONG WINAPI SCardListReaderGroups(SCARDCONTEXT hContext,
	/*@out@*/ LPSTR mszGroups, LPDWORD pcchGroups) {

	// TODO: update mszGroups
	abort();
	return SCARD_S_SUCCESS;
}

//SCardListReaders
WINSCARDAPI LONG WINAPI SCardListReaders(SCARDCONTEXT hContext,
	/*@null@*/ /*@out@*/ LPCSTR mszGroups,
	/*@null@*/ /*@out@*/ LPSTR mszReaders,
	/*@out@*/ LPDWORD pcchReaders) {

	if (mszReaders) {
		if (pcchReaders && *pcchReaders > strlen(AREADER) + 1) {
			memset(mszReaders, 0, *pcchReaders);
			strcpy(mszReaders, AREADER);
		}
	}

	if (pcchReaders)
		*pcchReaders = strlen(AREADER) + 2;

	return SCARD_S_SUCCESS;
}


//SCardFreeMemory
WINSCARDAPI LONG WINAPI SCardFreeMemory(SCARDCONTEXT hContext, LPCVOID pvMem) {
	LOGFUNC();
	return SCARD_S_SUCCESS;
}

//SCardCancel
WINSCARDAPI LONG WINAPI SCardCancel(SCARDCONTEXT hContext) {
	LOGFUNC();
	return SCARD_S_SUCCESS;
}

WINSCARDAPI LONG WINAPI SCardGetAttrib(SCARDHANDLE hCard, DWORD dwAttrId,
	/*@out@*/ LPBYTE pbAttr, LPDWORD pcbAttrLen) {

	LOGFUNC();
	//TODO: implement
	abort();
	return SCARD_S_SUCCESS;
}

WINSCARDAPI LONG WINAPI SCardSetAttrib(SCARDHANDLE hCard, DWORD dwAttrId,
	LPCBYTE pbAttr, DWORD cbAttrLen) {
	LOGFUNC();
	return SCARD_S_SUCCESS;
}

extern WINSCARDAPI LONG WINAPI
SCardReadCacheW(
	_In_  SCARDCONTEXT hContext,
	_In_  UUID *CardIdentifier,
	_In_  DWORD FreshnessCounter,
	_In_  LPWSTR LookupName,
	_Out_writes_bytes_(*DataLen) PBYTE Data,
	_Out_ DWORD *DataLen) {
	LOGFUNC();
	return SCARD_W_CACHE_ITEM_NOT_FOUND;
}

extern WINSCARDAPI LONG WINAPI SCardWriteCacheW(
	_In_ SCARDCONTEXT hContext,
	_In_ UUID *CardIdentifier,
	_In_ DWORD FreshnessCounter,
	_In_ LPWSTR LookupName,
	_In_reads_bytes_(DataLen) PBYTE Data,
	_In_ DWORD DataLen) {
	LOGFUNC();
	return SCARD_W_CACHE_ITEM_TOO_BIG;
}
