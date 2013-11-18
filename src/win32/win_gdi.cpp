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

#include "win_gdi.h"

namespace win32 {

// =============================================================================

Dc::Dc(HDC hDC) : 
  m_hDC(hDC), m_hBitmapOld(NULL), m_hBrushOld(NULL), m_hFontOld(NULL)
{
}

Dc::~Dc() {
  if (m_hDC) {
    if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));
    if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));
    if (m_hFontOld) ::DeleteObject(::SelectObject(m_hDC, m_hFontOld));
    
    HWND hwnd = ::WindowFromDC(m_hDC);
    if (hwnd) {
      ::ReleaseDC(hwnd, m_hDC);
    } else {
      ::DeleteDC(m_hDC);
    }
  }
}

void Dc::AttachDC(HDC hDC) {
  if (m_hDC || !hDC) return;
  m_hDC = hDC;
}

HDC Dc::DetachDC() {
  if (!m_hDC) return NULL;

  if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));
  if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));
  if (m_hFontOld) ::DeleteObject(::SelectObject(m_hDC, m_hFontOld));

  HDC hDC = m_hDC;
  m_hDC   = NULL;
  return hDC;
}

HDC Dc::Get() const {
  return m_hDC;
}

void Dc::AttachBrush(HBRUSH hBrush) {
  if (!m_hDC || !hBrush) return;
  if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));
  m_hBrushOld = (HBRUSH)::SelectObject(m_hDC, hBrush);
}

void Dc::CreateSolidBrush(COLORREF color) {
  if (!m_hDC) return;
  if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));
  HBRUSH hBrush = ::CreateSolidBrush(color);
  m_hBrushOld = (HBRUSH)::SelectObject(m_hDC, hBrush);
}

HBRUSH Dc::DetachBrush() {
  if (!m_hDC || !m_hBrushOld) return NULL;
  HBRUSH hBrush = (HBRUSH)::SelectObject(m_hDC, m_hBrushOld);
  m_hBrushOld = NULL;
  return hBrush;
}

void Dc::AttachFont(HFONT hFont) {
  if (!m_hDC || !hFont) return;
  if (m_hFontOld) ::DeleteObject(::SelectObject(m_hDC, m_hFontOld));
  m_hFontOld = (HFONT)::SelectObject(m_hDC, hFont);
}

HFONT Dc::DetachFont() {
  if (!m_hDC || !m_hFontOld) return NULL;
  HFONT hFont = (HFONT)::SelectObject(m_hDC, m_hFontOld);
  m_hFontOld = NULL;
  return hFont;
}

void Dc::EditFont(LPCWSTR lpFaceName, INT iSize, BOOL bBold, BOOL bItalic, BOOL bUnderline) {
  if (m_hFontOld) ::DeleteObject(::SelectObject(m_hDC, m_hFontOld));
  m_hFontOld = (HFONT)::GetCurrentObject(m_hDC, OBJ_FONT);
  LOGFONT lFont; ::GetObject(m_hFontOld, sizeof(LOGFONT), &lFont);
  
  if (lpFaceName)
    ::lstrcpy(lFont.lfFaceName, lpFaceName);
  if (iSize > -1) {
    lFont.lfHeight = -MulDiv(iSize, GetDeviceCaps(m_hDC, LOGPIXELSY), 72);
    lFont.lfWidth = 0;
  }
  if (bBold > -1)
    lFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
  if (bItalic > -1)
    lFont.lfItalic = bItalic;
  if (bUnderline > -1)
    lFont.lfUnderline = bUnderline;

  HFONT hFont = ::CreateFontIndirect(&lFont);
  ::SelectObject(m_hDC, hFont);
}

BOOL Dc::FillRect(const RECT& rc, HBRUSH hbr) const {
  return (BOOL)::FillRect(m_hDC, &rc, hbr);
}

void Dc::FillRect(const RECT& rc, COLORREF color) const {
  COLORREF old_color = ::SetBkColor(m_hDC, color);
  ::ExtTextOut(m_hDC, 0, 0, ETO_OPAQUE, &rc, L"", 0, 0);
  ::SetBkColor(m_hDC, old_color);
}

void Dc::AttachBitmap(HBITMAP hBitmap) {
  if (!m_hDC || !hBitmap) return;
  if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));
  m_hBitmapOld = (HBITMAP)::SelectObject(m_hDC, hBitmap);
}

HBITMAP Dc::DetachBitmap() {
  if (!m_hDC || !m_hBitmapOld) return NULL;
  HBITMAP hBitmap = (HBITMAP)::SelectObject(m_hDC, m_hBitmapOld);
  m_hBitmapOld = NULL;
  return hBitmap;
}

BOOL Dc::BitBlt(int x, int y, int nWidth, int nHeight, HDC hSrcDC, int xSrc, int ySrc, DWORD dwRop) const {
  return ::BitBlt(m_hDC, x, y, nWidth, nHeight, hSrcDC, xSrc, ySrc, dwRop);
}

int Dc::SetStretchBltMode(int mode) {
  return ::SetStretchBltMode(m_hDC, mode);
}

BOOL Dc::StretchBlt(int x, int y, int nWidth, int nHeight, HDC hSrcDC, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop) const {
  return ::StretchBlt(m_hDC, x, y, nWidth, nHeight, hSrcDC, xSrc, ySrc, nSrcWidth, nSrcHeight, dwRop);
}

int Dc::DrawText(LPCWSTR lpszString, int nCount, const RECT& rc, UINT nFormat) const {
  return ::DrawText(m_hDC, lpszString, nCount, (LPRECT)&rc, nFormat);
}

COLORREF Dc::GetTextColor() const {
  return ::GetTextColor(m_hDC);
}

COLORREF Dc::SetBkColor(COLORREF crColor) const {
  return ::SetBkColor(m_hDC, crColor);
}

int Dc::SetBkMode(int iBkMode) const {
  return ::SetBkMode(m_hDC, iBkMode);
}

COLORREF Dc::SetTextColor(COLORREF crColor) const {
  return ::SetTextColor(m_hDC, crColor);
}

// =============================================================================

Rect::Rect() {
  left = 0; top = 0; right = 0; bottom = 0;
}

Rect::Rect(int l, int t, int r, int b) {
  left = l; top = t; right = r; bottom = b;
}

Rect::Rect(const RECT& rc) {
  ::CopyRect(this, &rc);
}

Rect::Rect(LPCRECT lprc) {
  ::CopyRect(this, lprc);
}

void Rect::Copy(const RECT& rc) {
  ::CopyRect(this, &rc);
}

BOOL Rect::Equal(const RECT& rc) {
  return ::EqualRect(&rc, this);
}

BOOL Rect::Inflate(int dx, int dy) {
  return ::InflateRect(this, dx, dy);
}

BOOL Rect::Intersect(const RECT& rc1, const RECT& rc2) {
  return ::IntersectRect(this, &rc1, &rc2);
}

BOOL Rect::IsEmpty() {
  return ::IsRectEmpty(this);
}

BOOL Rect::Offset(int dx, int dy) {
  return ::OffsetRect(this, dx, dy);
}

BOOL Rect::PtIn(POINT pt) {
  return ::PtInRect(this, pt);
}

BOOL Rect::Set(int left, int top, int right, int bottom) {
  return ::SetRect(this, left, top, right, bottom);
}

BOOL Rect::SetEmpty() {
  return ::SetRectEmpty(this);
}

BOOL Rect::Subtract(const RECT& rc1, const RECT& rc2) {
  return ::SubtractRect(this, &rc1, &rc2);
}

BOOL Rect::Union(const RECT& rc1, const RECT& rc2) {
  return ::UnionRect(this, &rc1, &rc2);
}

} // namespace win32