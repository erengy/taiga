/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

CGdiPlus GdiPlus;

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
  vertex[0].Red   = GetRValue(dwColor1) << 8;
  vertex[0].Green = GetGValue(dwColor1) << 8;
  vertex[0].Blue  = GetBValue(dwColor1) << 8;
  vertex[0].Alpha = 0x0000;
  vertex[1].x     = lpRect->right;
  vertex[1].y     = lpRect->bottom;
  vertex[1].Red   = GetRValue(dwColor2) << 8;
  vertex[1].Green = GetGValue(dwColor2) << 8;
  vertex[1].Blue  = GetBValue(dwColor2) << 8;
  vertex[1].Alpha = 0x0000;
  GRADIENT_RECT gRect = {0, 1};
  return GdiGradientFill(hdc, vertex, 2, &gRect, 1, static_cast<ULONG>(bVertical));
}

BOOL DrawProgressBar(HDC hdc, const LPRECT lpRect, DWORD dwColor1, DWORD dwColor2, DWORD dwColor3) {
  // Draw bottom rect
  RECT rect = *lpRect;
  rect.top += (rect.bottom - rect.top) / 2;
  HBRUSH brush = CreateSolidBrush(dwColor3);
  FillRect(hdc, &rect, brush);
  DeleteObject(brush);
  // Draw top gradient
  rect.bottom = rect.top;
  rect.top = lpRect->top;
  return GradientRect(hdc, &rect, dwColor1, dwColor2, true);
}

// =============================================================================

COLORREF HexToARGB(const wstring& text) {
  int i = text.length() - 6;
  if (i < 0) return 0;

  unsigned int r, g, b;
  r = wcstoul(text.substr(i + 0, 2).c_str(), NULL, 16);
  g = wcstoul(text.substr(i + 2, 2).c_str(), NULL, 16);
  b = wcstoul(text.substr(i + 4, 2).c_str(), NULL, 16);

  return RGB(r, g, b);
}

CRect ResizeRect(const CRect& rect_dest, int src_width, int src_height, bool stretch, bool center_x, bool center_y) {
  CRect rect = rect_dest;

  float dest_width   = static_cast<float>(rect_dest.Width());
  float dest_height  = static_cast<float>(rect_dest.Height());
  float image_width  = static_cast<float>(src_width);
  float image_Height = static_cast<float>(src_height);
  
  // Source < Destination (No need to resize)
  if ((image_width < dest_width) && (image_Height < dest_height) && !stretch) {
    rect.right = rect.left + src_width;
    rect.bottom = rect.top + src_height;
    if (center_x) rect.Offset(static_cast<int>((image_width - image_width) / 2), 0);
    if (center_y) rect.Offset(0, static_cast<int>((dest_height - image_Height) / 2));

  // Source > Destination (Resize)
  } else {
    // Calculate aspect ratios
    float dest_ratio = dest_width / dest_height;
    float image_ratio = image_width / image_Height;
    
    // Width > Height
    if (image_ratio > dest_ratio) {
      rect.bottom = rect.top + static_cast<int>(dest_width * (1.0f / image_ratio));
      if (center_y) rect.Offset(0, static_cast<int>((dest_height - rect.Height()) / 2.0f));
    // Height > Width
    } else if (image_ratio < dest_ratio) {
      rect.right = rect.left + static_cast<int>(dest_height * image_ratio);
      if (center_x) rect.Offset(static_cast<int>((dest_width - rect.Width()) / 2.0f), 0);
    }
  }
  
  return rect;
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

// =============================================================================

bool Image::Load(const wstring& path) {
  ::DeleteObject(dc.DetachBitmap());
  
  HBITMAP hbmp = nullptr;
  if (dc.Get() == nullptr) {
    HDC hScreen = ::GetDC(nullptr);
    dc = ::CreateCompatibleDC(hScreen);
    ::ReleaseDC(NULL, hScreen);
  }

  Gdiplus::Bitmap bmp(path.c_str());
  width = bmp.GetWidth();
  height = bmp.GetHeight();
  bmp.GetHBITMAP(NULL, &hbmp);
  
  if (!hbmp || !width || !height) {
    ::DeleteObject(hbmp);
    ::DeleteDC(dc.DetachDC());
    return false;
  } else {
    rect.bottom = rect.top + static_cast<int>(rect.Width() * // TODO: Move to dlg_anime_info
      (static_cast<float>(height) / static_cast<float>(width)));
    dc.AttachBitmap(hbmp);
    return true;
  }
}