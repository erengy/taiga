/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

namespace win {

Dc::Dc()
    : dc_(nullptr),
      bitmap_old_(nullptr),
      brush_old_(nullptr),
      font_old_(nullptr) {
}

Dc::Dc(HDC hdc)
    : dc_(hdc),
      bitmap_old_(nullptr),
      brush_old_(nullptr),
      font_old_(nullptr) {
}

Dc::~Dc() {
  if (dc_) {
    if (bitmap_old_)
      ::DeleteObject(::SelectObject(dc_, bitmap_old_));
    if (brush_old_)
      ::DeleteObject(::SelectObject(dc_, brush_old_));
    if (font_old_)
      ::DeleteObject(::SelectObject(dc_, font_old_));

    HWND hwnd = ::WindowFromDC(dc_);
    if (hwnd) {
      ::ReleaseDC(hwnd, dc_);
    } else {
      ::DeleteDC(dc_);
    }
  }
}

void Dc::operator=(const HDC hdc) {
  AttachDc(hdc);
}

void Dc::AttachDc(HDC hdc) {
  if (dc_ || !hdc)
    return;

  dc_ = hdc;
}

HDC Dc::DetachDc() {
  if (!dc_)
    return nullptr;

  if (bitmap_old_)
    ::DeleteObject(::SelectObject(dc_, bitmap_old_));
  if (brush_old_)
    ::DeleteObject(::SelectObject(dc_, brush_old_));
  if (font_old_)
    ::DeleteObject(::SelectObject(dc_, font_old_));

  HDC hdc = dc_;
  dc_ = nullptr;

  return hdc;
}

HDC Dc::Get() const {
  return dc_;
}

void Dc::AttachBrush(HBRUSH brush) {
  if (!dc_ || !brush)
    return;

  if (brush_old_)
    ::DeleteObject(::SelectObject(dc_, brush_old_));

  brush_old_ = reinterpret_cast<HBRUSH>(::SelectObject(dc_, brush));
}

void Dc::CreateSolidBrush(COLORREF color) {
  if (!dc_)
    return;

  if (brush_old_)
    ::DeleteObject(::SelectObject(dc_, brush_old_));

  HBRUSH brush = ::CreateSolidBrush(color);
  brush_old_ = reinterpret_cast<HBRUSH>(::SelectObject(dc_, brush));
}

HBRUSH Dc::DetachBrush() {
  if (!dc_ || !brush_old_)
    return nullptr;

  HBRUSH brush = reinterpret_cast<HBRUSH>(::SelectObject(dc_, brush_old_));
  brush_old_ = nullptr;

  return brush;
}

void Dc::AttachFont(HFONT font) {
  if (!dc_ || !font)
    return;

  if (font_old_)
    ::DeleteObject(::SelectObject(dc_, font_old_));

  font_old_ = reinterpret_cast<HFONT>(::SelectObject(dc_, font));
}

HFONT Dc::DetachFont() {
  if (!dc_ || !font_old_)
    return nullptr;

  HFONT font = reinterpret_cast<HFONT>(::SelectObject(dc_, font_old_));
  font_old_ = nullptr;

  return font;
}

void Dc::EditFont(LPCWSTR face_name, INT size,
                  BOOL bold, BOOL italic, BOOL underline) {
  if (font_old_)
    ::DeleteObject(::SelectObject(dc_, font_old_));
  font_old_ = reinterpret_cast<HFONT>(::GetCurrentObject(dc_, OBJ_FONT));

  LOGFONT logfont;
  ::GetObject(font_old_, sizeof(LOGFONT), &logfont);

  if (face_name)
    ::lstrcpy(logfont.lfFaceName, face_name);
  if (size > -1) {
    logfont.lfHeight = -MulDiv(size, GetDeviceCaps(dc_, LOGPIXELSY), 72);
    logfont.lfWidth = 0;
  }
  if (bold > -1)
    logfont.lfWeight = bold ? FW_BOLD : FW_NORMAL;
  if (italic > -1)
    logfont.lfItalic = italic;
  if (underline > -1)
    logfont.lfUnderline = underline;

  HFONT font = ::CreateFontIndirect(&logfont);
  ::SelectObject(dc_, font);
}

BOOL Dc::FillRect(const RECT& rect, HBRUSH brush) const {
  return ::FillRect(dc_, &rect, brush);
}

void Dc::FillRect(const RECT& rect, COLORREF color) const {
  COLORREF old_color = ::SetBkColor(dc_, color);
  ::ExtTextOut(dc_, 0, 0, ETO_OPAQUE, &rect, L"", 0, nullptr);
  ::SetBkColor(dc_, old_color);
}

void Dc::AttachBitmap(HBITMAP bitmap) {
  if (!dc_ || !bitmap)
    return;

  if (bitmap_old_)
    ::DeleteObject(::SelectObject(dc_, bitmap_old_));

  bitmap_old_ = reinterpret_cast<HBITMAP>(::SelectObject(dc_, bitmap));
}

HBITMAP Dc::DetachBitmap() {
  if (!dc_ || !bitmap_old_)
    return nullptr;

  HBITMAP bitmap = reinterpret_cast<HBITMAP>(::SelectObject(dc_, bitmap_old_));
  bitmap_old_ = nullptr;

  return bitmap;
}

BOOL Dc::BitBlt(int x, int y, int width, int height,
                HDC dc_src, int x_src, int y_src, DWORD rop) const {
  return ::BitBlt(dc_, x, y, width, height, dc_src, x_src, y_src, rop);
}

int Dc::SetStretchBltMode(int mode) {
  return ::SetStretchBltMode(dc_, mode);
}

BOOL Dc::StretchBlt(int x, int y, int width, int height,
                    HDC dc_src, int x_src, int y_src,
                    int width_src, int height_src, DWORD rop) const {
  return ::StretchBlt(dc_, x, y, width, height,
                      dc_src, x_src, y_src, width_src, height_src, rop);
}

int Dc::DrawText(LPCWSTR text, int count, const RECT& rect, UINT format) const {
  return ::DrawText(dc_, text, count, const_cast<LPRECT>(&rect), format);
}

COLORREF Dc::GetTextColor() const {
  return ::GetTextColor(dc_);
}

COLORREF Dc::SetBkColor(COLORREF color) const {
  return ::SetBkColor(dc_, color);
}

int Dc::SetBkMode(int bk_mode) const {
  return ::SetBkMode(dc_, bk_mode);
}

COLORREF Dc::SetTextColor(COLORREF color) const {
  return ::SetTextColor(dc_, color);
}

////////////////////////////////////////////////////////////////////////////////

Brush::Brush()
    : brush_(nullptr) {
}

Brush::Brush(HBRUSH brush)
    : brush_(brush) {
}

Brush::~Brush() {
  Set(nullptr);
}

HBRUSH Brush::Get() const {
  return brush_;
}

void Brush::Set(HBRUSH brush) {
  if (brush_)
    ::DeleteObject(brush_);
  brush_ = brush;
}

Brush::operator HBRUSH() const {
  return brush_;
}

////////////////////////////////////////////////////////////////////////////////

Font::Font()
    : font_(nullptr) {
}

Font::Font(HFONT font)
    : font_(font) {
}

Font::~Font() {
  Set(nullptr);
}

HFONT Font::Get() const {
  return font_;
}

void Font::Set(HFONT font) {
  if (font_)
    ::DeleteObject(font_);
  font_ = font;
}

Font::operator HFONT() const {
  return font_;
}

////////////////////////////////////////////////////////////////////////////////

Rect::Rect() {
  left = 0; top = 0; right = 0; bottom = 0;
}

Rect::Rect(int l, int t, int r, int b) {
  left = l; top = t; right = r; bottom = b;
}

Rect::Rect(const RECT& rect) {
  ::CopyRect(this, &rect);
}

Rect::Rect(LPCRECT rect) {
  ::CopyRect(this, rect);
}

BOOL Rect::operator==(const RECT& rect) const {
  return ::EqualRect(this, &rect);
}

BOOL Rect::operator!=(const RECT& rect) const {
  return !::EqualRect(this, &rect);
}

void Rect::operator=(const RECT& rect) {
  ::CopyRect(this, &rect);
}

int Rect::Height() const {
  return bottom - top;
}

int Rect::Width() const {
  return right - left;
}

void Rect::Copy(const RECT& rect) {
  ::CopyRect(this, &rect);
}

BOOL Rect::Equal(const RECT& rect) const {
  return ::EqualRect(&rect, this);
}

BOOL Rect::Inflate(int dx, int dy) {
  return ::InflateRect(this, dx, dy);
}

BOOL Rect::Intersect(const RECT& rect1, const RECT& rect2) {
  return ::IntersectRect(this, &rect1, &rect2);
}

BOOL Rect::IsEmpty() const {
  return ::IsRectEmpty(this);
}

BOOL Rect::Offset(int dx, int dy) {
  return ::OffsetRect(this, dx, dy);
}

BOOL Rect::PtIn(POINT point) const {
  return ::PtInRect(this, point);
}

BOOL Rect::Set(int left, int top, int right, int bottom) {
  return ::SetRect(this, left, top, right, bottom);
}

BOOL Rect::SetEmpty() {
  return ::SetRectEmpty(this);
}

BOOL Rect::Subtract(const RECT& rect1, const RECT& rect2) {
  return ::SubtractRect(this, &rect1, &rect2);
}

BOOL Rect::Union(const RECT& rect1, const RECT& rect2) {
  return ::UnionRect(this, &rect1, &rect2);
}

}  // namespace win