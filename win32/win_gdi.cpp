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

// =============================================================================

CDC::CDC(HDC hDC) : 
  m_hDC(hDC), m_hBitmapOld(NULL), m_hBrushOld(NULL), m_hFontOld(NULL)
{
}

CDC::~CDC() {
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

void CDC::AttachDC(HDC hDC) {
  if (m_hDC || !hDC) return;
  m_hDC = hDC;
}

HDC CDC::DetachDC() {
  if (!m_hDC) return NULL;

  if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));
  if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));
  if (m_hFontOld) ::DeleteObject(::SelectObject(m_hDC, m_hFontOld));

  HDC hDC = m_hDC;
  m_hDC   = NULL;
  return hDC;
}

HDC CDC::Get() const {
  return m_hDC;
}

void CDC::AttachBrush(HBRUSH hBrush) {
  if (!m_hDC || !hBrush) return;
  if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));
  m_hBrushOld = (HBRUSH)::SelectObject(m_hDC, hBrush);
}

void CDC::CreateSolidBrush(COLORREF color) {
  if (!m_hDC) return;
  if (m_hBrushOld) ::DeleteObject(::SelectObject(m_hDC, m_hBrushOld));
  HBRUSH hBrush = ::CreateSolidBrush(color);
  m_hBrushOld = (HBRUSH)::SelectObject(m_hDC, hBrush);
}

HBRUSH CDC::DetachBrush() {
  if (!m_hDC || !m_hBrushOld) return NULL;
  HBRUSH hBrush = (HBRUSH)::SelectObject(m_hDC, m_hBrushOld);
  m_hBrushOld = NULL;
  return hBrush;
}

void CDC::AttachFont(HFONT hFont) {
  if (!m_hDC || !hFont) return;
  if (m_hFontOld) ::DeleteObject(::SelectObject(m_hDC, m_hFontOld));
  m_hFontOld = (HFONT)::SelectObject(m_hDC, hFont);
}

HFONT CDC::DetachFont() {
  if (!m_hDC || !m_hFontOld) return NULL;
  HFONT hFont = (HFONT)::SelectObject(m_hDC, m_hFontOld);
  m_hFontOld = NULL;
  return hFont;
}

void CDC::EditFont(LPCWSTR lpFaceName, INT iSize, BOOL bBold, BOOL bItalic, BOOL bUnderline) {
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

BOOL CDC::FillRect(const RECT& rc, HBRUSH hbr) const {
  return (BOOL)::FillRect(m_hDC, &rc, hbr);
}

void CDC::FillRect(const RECT& rc, COLORREF color) const {
  COLORREF old_color = ::SetBkColor(m_hDC, color);
  ::ExtTextOut(m_hDC, 0, 0, ETO_OPAQUE, &rc, L"", 0, 0);
  ::SetBkColor(m_hDC, old_color);
}

void CDC::AttachBitmap(HBITMAP hBitmap) {
  if (!m_hDC || !hBitmap) return;
  if (m_hBitmapOld) ::DeleteObject(::SelectObject(m_hDC, m_hBitmapOld));
  m_hBitmapOld = (HBITMAP)::SelectObject(m_hDC, hBitmap);
}

HBITMAP CDC::DetachBitmap() {
  if (!m_hDC || !m_hBitmapOld) return NULL;
  HBITMAP hBitmap = (HBITMAP)::SelectObject(m_hDC, m_hBitmapOld);
  m_hBitmapOld = NULL;
  return hBitmap;
}

BOOL CDC::BitBlt(int x, int y, int nWidth, int nHeight, HDC hSrcDC, int xSrc, int ySrc, DWORD dwRop) const {
  return ::BitBlt(m_hDC, x, y, nWidth, nHeight, hSrcDC, xSrc, ySrc, dwRop);
}

int CDC::SetStretchBltMode(int mode) {
  return ::SetStretchBltMode(m_hDC, mode);
}

BOOL CDC::StretchBlt(int x, int y, int nWidth, int nHeight, HDC hSrcDC, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop) const {
  return ::StretchBlt(m_hDC, x, y, nWidth, nHeight, hSrcDC, xSrc, ySrc, nSrcWidth, nSrcHeight, dwRop);
}

int CDC::DrawText(LPCWSTR lpszString, int nCount, const RECT& rc, UINT nFormat) const {
  return ::DrawText(m_hDC, lpszString, nCount, (LPRECT)&rc, nFormat);
}

COLORREF CDC::SetBkColor(COLORREF crColor) const {
  return ::SetBkColor(m_hDC, crColor);
}

int CDC::SetBkMode(int iBkMode) const {
  return ::SetBkMode(m_hDC, iBkMode);
}

COLORREF CDC::SetTextColor(COLORREF crColor) const {
  return ::SetTextColor(m_hDC, crColor);
}

// =============================================================================

CRect::CRect() {
  left = 0; top = 0; right = 0; bottom = 0;
}

CRect::CRect(int l, int t, int r, int b) {
  left = l; top = t; right = r; bottom = b;
}

CRect::CRect(const RECT& rc) {
  ::CopyRect(this, &rc);
}

CRect::CRect(LPCRECT lprc) {
  ::CopyRect(this, lprc);
}

void CRect::Copy(const RECT& rc) {
  ::CopyRect(this, &rc);
}

BOOL CRect::Equal(const RECT& rc) {
  return ::EqualRect(&rc, this);
}

BOOL CRect::Inflate(int dx, int dy) {
  return ::InflateRect(this, dx, dy);
}

BOOL CRect::Intersect(const RECT& rc1, const RECT& rc2) {
  return ::IntersectRect(this, &rc1, &rc2);
}

BOOL CRect::IsEmpty() {
  return ::IsRectEmpty(this);
}

BOOL CRect::Offset(int dx, int dy) {
  return ::OffsetRect(this, dx, dy);
}

BOOL CRect::PtIn(POINT pt) {
  return ::PtInRect(this, pt);
}

BOOL CRect::Set(int left, int top, int right, int bottom) {
  return ::SetRect(this, left, top, right, bottom);
}

BOOL CRect::SetEmpty() {
  return ::SetRectEmpty(this);
}

BOOL CRect::Subtract(const RECT& rc1, const RECT& rc2) {
  return ::SubtractRect(this, &rc1, &rc2);
}

BOOL CRect::Union(const RECT& rc1, const RECT& rc2) {
  return ::UnionRect(this, &rc1, &rc2);
}