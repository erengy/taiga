/*

Copyright (c) 2010 Brook Miles

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#define WIN32_LEAN_AND_MEAN
#include <tchar.h>
#include <windows.h>
#include <strsafe.h>
#include <stdlib.h>

#include "TwitDebug.h"

#define MESSAGEBOXFMT_BUF_SIZE	256

#define DEBUG_REPORT_BUF_SIZE	128
#define DEBUG_ABORT_CODE		0xBADD061E

#define DEBUG_TRACE_BUF_SIZE 1024
#define DEBUG_TRACE_FMT_SIZE 128

#define SIZEOF(x) (sizeof(x)/sizeof(*x))

UINT MessageBoxFmt(HWND hwnd, LPCTSTR pszTextFmt, LPCTSTR pszTitle, UINT uType, ...)
{
	TCHAR buf[MESSAGEBOXFMT_BUF_SIZE];
	va_list args;
	va_start(args, uType);
	_vsntprintf_s(buf, SIZEOF(buf), _TRUNCATE, pszTextFmt, args);
	va_end(args);
	return MessageBox(hwnd, buf, pszTitle, uType);
}

#if defined(_DEBUG)
/////////////////////////////////////////////////////////////////////////////
//	_DebugReport is used by the ASSERTE macro.  Since the preprocessor
//	constants __FILE__ and __LINE__ are always ANSI, it takes LPCSTR
//	instead of LPCTSTR, and then converts to wide strings.
//

int _DebugReport(LPCSTR pszFile, int iLine, LPCSTR pszExpr, UINT gle, BOOL bConsoleOnly)
{
	char buf[DEBUG_REPORT_BUF_SIZE];
	wchar_t wbuf[DEBUG_REPORT_BUF_SIZE];
	wchar_t wbufTitle[DEBUG_REPORT_BUF_SIZE];
	UINT ret;
	
	size_t tmp = 0;

	_snprintf_s(buf, DEBUG_REPORT_BUF_SIZE, _TRUNCATE, "Assertion Failed! (0x%08X)", gle);
	mbstowcs_s(&tmp, wbufTitle, SIZEOF(wbufTitle), buf, _TRUNCATE);

	_snprintf_s(buf, DEBUG_REPORT_BUF_SIZE, _TRUNCATE, "%s:%d\r\n%s", 
		strrchr(pszFile, '\\') + 1, iLine, pszExpr);
	mbstowcs_s(&tmp, wbuf, SIZEOF(wbuf), buf, _TRUNCATE);

	OutputDebugString(wbufTitle);
	OutputDebugString(_T("\r\n"));
	OutputDebugString(wbuf);
	OutputDebugString(_T("\r\n"));

	if(!bConsoleOnly)
	{
		ret = MessageBox(NULL, wbuf, wbufTitle, MB_ICONERROR | MB_ABORTRETRYIGNORE);
		if(ret == IDABORT)
			ExitThread(DEBUG_ABORT_CODE);
		return (ret == IDRETRY);
	}
	else
	{
		return TRUE;
	}
}

void _TRACE(LPCSTR fmt, ...)
{
	int len;
	TCHAR buf[DEBUG_TRACE_BUF_SIZE + 3] = _T("");
	va_list va;

#ifndef UNICODE
	char* tfmt = fmt;
#else
	wchar_t tfmt[DEBUG_TRACE_FMT_SIZE];
	size_t tmp = 0;
	mbstowcs_s(&tmp, tfmt, SIZEOF(tfmt), fmt, _TRUNCATE);
#endif

	va_start(va, fmt);
	len = _vsntprintf_s(buf, SIZEOF(buf), _TRUNCATE, tfmt, va);
	va_end(va);

	if(len >= -1)
	{
		if(len == -1)
		{
			len = DEBUG_TRACE_BUF_SIZE;
		}
		_tcscpy_s(&buf[len], SIZEOF(buf) - len, _T("\r\n"));
	}
	else
	{
		_tcscpy_s(buf, SIZEOF(buf), _T("_DEBUG FORMATTING ERROR\r\n"));
	}

	//_tprintf(buf);
	OutputDebugString(buf);
}

#endif//defined(_DEBUG)