/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "std.h"
#include "gfx.h"

#pragma comment(lib, "gdiplus.lib")
static ULONG_PTR gdiplusToken;
using namespace Gdiplus;

// =============================================================================

void Gdiplus_Initialize() {
  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
}

void Gdiplus_Shutdown() {
  Gdiplus::GdiplusShutdown(gdiplusToken);
  gdiplusToken = NULL;
}

HBITMAP Gdiplus_LoadImage(wstring file) {
  HBITMAP hBitmap = NULL;
  
  Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(file.c_str());
  if (pBitmap) {
    pBitmap->GetHBITMAP(NULL, &hBitmap);
    delete pBitmap; pBitmap = NULL;
  }

  return hBitmap;
}

HBITMAP Gdiplus_LoadPNGResource(HINSTANCE hInstance, int nResource, LPCWSTR lpResourceType) {
  HBITMAP hBitmap = NULL;

  if (HRSRC hResource = FindResource(hInstance, MAKEINTRESOURCE(nResource), lpResourceType)) {
    if (DWORD dwSize = SizeofResource(hInstance, hResource)) {
      if (const void* pResourceData = LockResource(LoadResource(hInstance, hResource))) {
        if (HGLOBAL hBuffer = GlobalAlloc(GHND, dwSize)) {
          if (void* pBuffer = GlobalLock(hBuffer)) {
            CopyMemory(pBuffer, pResourceData, dwSize);
            IStream* pStream = NULL;
            if (CreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK) {
              Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
              pStream->Release();
              if (pBitmap) { 
                pBitmap->GetHBITMAP(NULL, &hBitmap);
                delete pBitmap; pBitmap = NULL;
              }
            } GlobalUnlock(hBuffer);
          } GlobalFree(hBuffer);
  } } } }

  return hBitmap;
}

// =============================================================================

HFONT ChangeDCFont(HDC hdc, LPCWSTR lpFaceName, INT iSize, BOOL bBold, BOOL bItalic, BOOL bUnderline) {
  HFONT hFont = reinterpret_cast<HFONT>(GetCurrentObject(hdc, OBJ_FONT));
  LOGFONT lFont; GetObject(hFont, sizeof(LOGFONT), &lFont);
  
  if (lpFaceName)
    lstrcpy(lFont.lfFaceName, lpFaceName);
  if (iSize > -1) {
    lFont.lfHeight = -MulDiv(iSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    lFont.lfWidth = 0;
  }
  if (bBold > -1)
    lFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
  if (bItalic > -1)
    lFont.lfItalic = bItalic;
  if (bUnderline > -1)
    lFont.lfUnderline = bUnderline;

  hFont = CreateFontIndirect(&lFont);
  return reinterpret_cast<HFONT>(SelectObject(hdc, hFont));
}

BOOL GradientRect(HDC hdc, const LPRECT lpRect, DWORD dwColor1, DWORD dwColor2, bool bVertical) { 
  TRIVERTEX vertex[2];
  vertex[0].x     = lpRect->left;
  vertex[0].y     = lpRect->top;
  vertex[0].Red   = GetBValue(dwColor1) << 8;
  vertex[0].Green = GetGValue(dwColor1) << 8;
  vertex[0].Blue  = GetRValue(dwColor1) << 8;
  vertex[0].Alpha = 0x0000;
  vertex[1].x     = lpRect->right;
  vertex[1].y     = lpRect->bottom;
  vertex[1].Red   = GetBValue(dwColor2) << 8;
  vertex[1].Green = GetGValue(dwColor2) << 8;
  vertex[1].Blue  = GetRValue(dwColor2) << 8;
  vertex[1].Alpha = 0x0000;
  GRADIENT_RECT gRect = {0, 1};
  return GdiGradientFill(hdc, vertex, 2, &gRect, 1, static_cast<ULONG>(bVertical));
}

BOOL DrawProgressBar(HDC hdc, const LPRECT lpRect, DWORD dwColor1, DWORD dwColor2, DWORD dwColor3) {
  // Draw bottom rect
  RECT rect = *lpRect;
  rect.top += (rect.bottom - rect.top) / 2;
  HBRUSH brush = CreateSolidBrush(BGR2RGB(dwColor3));
  FillRect(hdc, &rect, brush);
  DeleteObject(brush);
  // Draw top gradient
  rect.bottom = rect.top;
  rect.top = lpRect->top;
  return GradientRect(hdc, &rect, dwColor1, dwColor2, true);
}

// =============================================================================

COLORREF BGR2RGB(COLORREF color) {
  return (color & 0x000000ff) << 16 | (color & 0x0000FF00) | (color & 0x00FF0000) >> 16;
}

COLORREF HexToARGB(LPCWSTR lpText) {
  wchar_t* stopwcs;
  COLORREF buffer = wcstoul(lpText, &stopwcs, 16);
  return buffer;
}

int ScaleX(int value) {
  static int dpi_x = 0;
  if (!dpi_x) {
    HDC hdc = GetDC(NULL);
    if (hdc) {
      dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
      ReleaseDC(NULL, hdc);
    }
  }
  return MulDiv(value, dpi_x, 96);
}

int ScaleY(int value) {
  static int dpi_y = 0;
  if (!dpi_y) {
    HDC hdc = GetDC(NULL);
    if (hdc) {
      dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
      ReleaseDC(NULL, hdc);
    }
  }
  return MulDiv(value, dpi_y, 96);
}