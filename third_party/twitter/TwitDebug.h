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

#ifndef _DEBUG_H_INCLUDED_
#define _DEBUG_H_INCLUDED_
#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
// MessageBoxFmt

UINT MessageBoxFmt(HWND hwnd, LPCTSTR pszTextFmt, LPCTSTR pszTitle, UINT uType, ...);

/////////////////////////////////////////////////////////////////////////////
// DebugReport

#ifdef _DEBUG
	#ifdef ASSERT_NO_BREAK
		int _DebugReport(LPCSTR pszFile, int iLine, LPCSTR pszExpr, UINT gle, BOOL bConsoleOnly);
		#ifdef _ASSERTE
			#undef _ASSERTE
		#endif
		#define _ASSERTE(x) if(!(x) && _DebugReport(__FILE__, __LINE__, #x, GetLastError(), true)) while(0)
		#define _ASSERTC(x) if(!(x) && _DebugReport(__FILE__, __LINE__, #x, GetLastError(), true)) while(0)
	#else
		int _DebugReport(LPCSTR pszFile, int iLine, LPCSTR pszExpr, UINT gle, BOOL bConsoleOnly);
		#ifdef _ASSERTE
			#undef _ASSERTE
		#endif
		#define _ASSERTE(x) if(!(x) && _DebugReport(__FILE__, __LINE__, #x, GetLastError(), false)) DebugBreak()
		#define _ASSERTC(x) if(!(x) && _DebugReport(__FILE__, __LINE__, #x, GetLastError(), true)) DebugBreak()
	#endif
#else
	#ifndef _ASSERTE
	#define _ASSERTE(x)
	#endif
	#define _ASSERTC(x)
#endif

/////////////////////////////////////////////////////////////////////////////
// DebugTrace

#if defined(_DEBUG)
	void _TRACE(LPCSTR fmt, ...);
#else
	__inline void _TRACE(LPCSTR fmt, ...) {}
#endif//defined(_DEBUG)


#ifdef __cplusplus
}
#endif
#endif//_DEBUG_H_INCLUDED_
