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

#ifndef WIN_GDI_H
#define WIN_GDI_H

#include "win_main.h"

namespace win32 {

// =============================================================================

class Dc {
public:
  Dc(HDC hDC = NULL);
  virtual ~Dc();

  void operator = (const HDC hDC) {
    AttachDC(hDC);
  }

  void AttachDC(HDC hDC);
  HDC  DetachDC();
  HDC  Get() const;

  // Brush
  void   AttachBrush(HBRUSH hBrush);
  void   CreateSolidBrush(COLORREF color);
  HBRUSH DetachBrush();

  // Font
  void  AttachFont(HFONT hFont);
  HFONT DetachFont();
  void  EditFont(LPCWSTR lpFaceName = NULL, INT iSize = -1, BOOL bBold = -1, BOOL bItalic = -1, BOOL bUnderline = -1);

  // Painting
  BOOL FillRect(const RECT& rc, HBRUSH hbr) const;
  void FillRect(const RECT& rc, COLORREF color) const;
  
  // Bitmap
  void    AttachBitmap(HBITMAP hBitmap);
  BOOL    BitBlt(int x, int y, int nWidth, int nHeight, HDC hSrcDC, int xSrc, int ySrc, DWORD dwRop) const;
  HBITMAP DetachBitmap();
  int     SetStretchBltMode(int mode);
  BOOL    StretchBlt(int x, int y, int nWidth, int nHeight, HDC hSrcDC, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop) const;

  // Text
  int      DrawText(LPCWSTR lpszString, int nCount, const RECT& rc, UINT nFormat) const;
  COLORREF GetTextColor() const;
  COLORREF SetBkColor(COLORREF crColor) const;
  int      SetBkMode(int iBkMode) const;
  COLORREF SetTextColor(COLORREF crColor) const;

private:
  HDC     m_hDC;
  HBITMAP m_hBitmapOld;
  HBRUSH  m_hBrushOld;
  HFONT   m_hFontOld;
};

// =============================================================================

class Rect : public RECT {
public:
  Rect();
  Rect(int l, int t, int r, int b);
  Rect(const RECT& rc);
  Rect(LPCRECT lprc);

  BOOL operator == (const RECT& rc) {
    return ::EqualRect(this, &rc);
  }
  BOOL operator != (const RECT& rc) {
    return !::EqualRect(this, &rc);
  }
  void operator = (const RECT& srcRect) {
    ::CopyRect(this, &srcRect);
  }

  int Height() const {
    return bottom - top;
  }
  int Width() const {
    return right - left;
  }

  void Copy(const RECT& rc);
  BOOL Equal(const RECT& rc);
  BOOL Inflate(int dx, int dy);
  BOOL Intersect(const RECT& rc1, const RECT& rc2);
  BOOL IsEmpty();
  BOOL Offset(int dx, int dy);
  BOOL PtIn(POINT pt);
  BOOL Set(int left, int top, int right, int bottom);
  BOOL SetEmpty();
  BOOL Subtract(const RECT& rc1, const RECT& rc2);
  BOOL Union(const RECT& rc1, const RECT& rc2);
};

} // namespace win32

#endif // WIN_GDI_H