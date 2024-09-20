#pragma once
#include "string"
#include <iostream>
#include "Headers.h"
#include <stdarg.h>

using namespace std;

//-------------------------------------
//						TEST SECTION
//-------------------------------------
//	To activate unit test uncomment 
//	define of UNIT_TEST

#define UNIT_TEST
#ifdef UNIT_TEST
	#define TEST(exp) exp
#else
	#define TEST(exp)
#endif // UNITEST

//-------------------------------------
//						PLATFORM DEPENDEND CODE
//-------------------------------------

#ifdef _WIN32
	#define WIN(exp) exp
	#define NIX(exp)
#else
	#define WIN(exp)
	#define NIX(exp) exp
#endif

//-------------------------------------
//						COMMON CONSTANTS
//-------------------------------------

#define ENTER_TO_CLOSE_APP     L"Press Enter to close App\n"
#define ENCODING_STRING	       "encoding=\""
#define ENCODING_WSTRING       L"encoding=\""
#define UTF16_WSTRING          L"UTF-16\""
#define SERVER_XML_FILE        L"ServerFile.xml"
#define CLIENT_XML_FILE        L"ClientFile.xml"
#define INTRO_STRING           L"Make sure, that %ls is in one folder with executable file\n"
#define ENTER_PORT             L"Enter port\n"
#define ENTER_IP               L"Enter IP address\n"
#define XML_OPEN               L"XML file successfully opened!\n"
#define UNKNOWN_CMD            L"Unknown command!"

#define CLIENT_USER            L"Client User"
#define SERVER_USER            L"Server User"

#define CLIENT_SOCKET          L"Client Socket"
#define SERVER_SOCKET          L"Server Socket"

#define NEW_CONNECTION         L"New connection"
#define CLOSE_CONNECTION       L"Connection close"
#define SEND_MESSAGE           L"Send"
#define RECIEVE_MESSAGE        L"Recieve"

#define MAX_SYSTEM_PORT        65535
#define MIN_SYSTEM_PORT        1024
#define SOCK_TIMEOUT_SEC       10
#define SOCK_TIMEOUT_MSEC      10000
#define MAX_USERS_NUMBER       10
#define INTERVAL_OF_SEND       1000ms
#define SINCE_1900_YEAR	       1900

#define MSG_FROM_SERVER	   L"Time: %i:%i:%i %ls: %ls server: %i\n"
#define MSG_FROM_CLIENT	   L"Time: %i:%i:%i %ls: %ls client: %i\n"
#define LOG_FILE_NAME	   L"LogFile_%i%i%i_%i%i%i.txt"

//-------------------------------------
//						ERROR STRINGS
//-------------------------------------

#define ERR_INVALID_PORT L"Invalid port\n"
#define ERR_INVALID_XML  L"Invalid xml file: "

//-------------------------------------

NIX(
typedef struct sockaddr_in	SOCKADDR_IN;
typedef unsigned int		SOCKET;
#define SD_BOTH				0

typedef bool				BOOL;
typedef unsigned long		DWORD;
#define FALSE				false
#define TRUE				true
#define LPCWSTR				wchar_t*

#define MAX_PATH			260
#define SOCKET_ERROR		-1
)

//-------------------------------------

inline wstring FormatWText(const wchar_t* wstrFormat...)
{
	va_list args;
	va_start(args, wstrFormat);
	wstring wstrResult(wstrFormat);
	wstring wstrText;

	while (*wstrFormat != L'\0')
	{
		if (*wstrFormat == L'%')
		{
			++wstrFormat;
			if (*wstrFormat == L'l')
			{
				++wstrFormat;
				if (*wstrFormat == L's')
				{
					wstring wstrText = va_arg(args, const wchar_t*);
					size_t pos = wstrResult.find(L"%ls");
					wstrResult.replace(pos, 3, wstrText);
				}
			}
			if (*wstrFormat == L'i')
			{
				wstring wstrText = to_wstring(va_arg(args, int));
				size_t pos = wstrResult.find(L"%i");
				wstrResult.replace(pos, 2, wstrText);
			}
		}
		++wstrFormat;
	}
	va_end(args);
	return wstrResult;
}

//-------------------------------------

inline wstring ToWstring(string strText)
{
	wchar_t wchResult[MAX_PATH];
	memset(wchResult, 0, MAX_PATH);
	size_t sChConverted;
	WIN(errno_t)NIX(error_t) result =	WIN(mbstowcs_s(&sChConverted, wchResult, MAX_PATH, &strText[0], MAX_PATH))
										NIX(mbstowcs(wchResult, &strText[0], MAX_PATH));
	if (result == -1)
	{
		wprintf(L"ToWstring error!\n");
	}
	return wchResult;
}

//-------------------------------------

inline wstring ToWstringError(string strError)
{
	if (strError[strError.length() - 1] != '\n')
	{
		strError.push_back('\n');
	}
	return ToWstring(strError);
}

//-------------------------------------

inline string ToString(wstring wstrText)
{
	char chResult[MAX_PATH];
	memset(chResult, 0, MAX_PATH);
	size_t sChConverted;
	WIN(errno_t)NIX(error_t) result =	WIN(wcstombs_s(&sChConverted, chResult, MAX_PATH, &wstrText[0], MAX_PATH))
										NIX(wcstombs(chResult, &wstrText[0], MAX_PATH));
	if (result == -1)
	{
		wprintf(L"ToString error!\n");
	}
	return chResult;
}

//-------------------------------------

inline int GetErrorCode()
{
	return WIN(::GetLastError()) NIX(errno);
}
//-------------------------------------

inline int GetSocketErrorCode()
{
	return WIN(::WSAGetLastError()) NIX(errno);
}

//-------------------------------------

template<typename T> class CFreeMem
{
public:
	CFreeMem(int iNumber)
	{
		p = new T[iNumber];
	}
	~CFreeMem()
	{
		delete[] p;
	}
public:
	T *p;
};

//-------------------------------------

NIX(
inline int _wtoi(const wchar_t *wstr)
{
	return (int)wcstol(wstr, 0, 10);
}
)

//-------------------------------------

NIX(inline wstring ConvertEnconding(string strTo, string strFrom, char* pchIn, size_t iSizeIn, char* pchOut, size_t iSizeOut)
{
	wstring wstrError;
	iconv_t ic = iconv_open(strTo.c_str(), strFrom.c_str());
	if (ic == (iconv_t)-1)
	{
		wstrError = FormatWText(L"Failed iconv_open, error %i\n", GetErrorCode());
		iconv_close(ic);
		return wstrError;
	}

	size_t lres = iconv(ic, &pchIn, &iSizeIn, &pchOut, &iSizeOut);
	if (lres == -1)
	{
		wstrError = FormatWText(L"Failed to convert via iconv, error %i\n", GetErrorCode());
		iconv_close(ic);
		return wstrError;
	}

	iconv_close(ic);
	return wstrError;
})

//-------------------------------------


NIX(inline wstring ConvertUTF16LEtoUTF32LE(char* pchIn, size_t iSizeIn, char* pchOut)
{ 
	size_t iSizeOut = iSizeIn * 2;
	return ConvertEnconding("utf32le", "utf16le", pchIn, iSizeIn, pchOut, iSizeOut);
})

//-------------------------------------

NIX(inline wstring ConvertUTF8toUTF32LE(char* pchIn, size_t iSizeIn, char* pchOut)
{
	size_t iSizeOut = iSizeIn*4;
	return ConvertEnconding("utf32le", "utf8", pchIn, iSizeIn, pchOut, iSizeOut);
})

//-------------------------------------

NIX(inline wstring ConvertUTF32LEtoUTF16LE(char* pchIn, size_t iSizeIn, char* pchOut)
{
	size_t iSizeOut = iSizeIn / 2;
	return ConvertEnconding("utf16le", "utf32le", pchIn, iSizeIn, pchOut, iSizeOut);
})

//-------------------------------------

inline tm GetTime()
{
	chrono::time_point<chrono::system_clock> now = chrono::system_clock::now();
	time_t time = chrono::system_clock::to_time_t(now);
	tm tm;

	WIN(localtime_s(&tm, &time);)
	NIX(localtime_r(&time, &tm);)

	return tm;
}
