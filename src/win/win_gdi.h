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

#ifndef TAIGA_WIN_GDI_H
#define TAIGA_WIN_GDI_H

#include "win_main.h"

namespace win {

class Dc {
public:
  Dc();
  Dc(HDC hdc);
  virtual ~Dc();

  void operator=(const HDC hdc);

  void AttachDc(HDC hdc);
  HDC  DetachDc();
  HDC  Get() const;

  // Brush
  void   AttachBrush(HBRUSH brush);
  void   CreateSolidBrush(COLORREF color);
  HBRUSH DetachBrush();

  // Font
  void  AttachFont(HFONT font);
  HFONT DetachFont();
  void  EditFont(LPCWSTR face_name = nullptr, INT size = -1, BOOL bold = -1, BOOL italic = -1, BOOL underline = -1);

  // Painting
  BOOL FillRect(const RECT& rect, HBRUSH brush) const;
  void FillRect(const RECT& rect, COLORREF color) const;

  // Bitmap
  void    AttachBitmap(HBITMAP bitmap);
  BOOL    BitBlt(int x, int y, int width, int height, HDC dc_src, int x_src, int y_src, DWORD rop) const;
  HBITMAP DetachBitmap();
  int     SetStretchBltMode(int mode);
  BOOL    StretchBlt(int x, int y, int width, int height, HDC dc_src, int x_src, int y_src, int width_src, int height_src, DWORD rop) const;

  // Text
  int      DrawText(LPCWSTR text, int count, const RECT& rect, UINT format) const;
  COLORREF GetTextColor() const;
  COLORREF SetBkColor(COLORREF color) const;
  int      SetBkMode(int bk_mode) const;
  COLORREF SetTextColor(COLORREF color) const;

private:
  HDC     dc_;
  HBITMAP bitmap_old_;
  HBRUSH  brush_old_;
  HFONT   font_old_;
};

////////////////////////////////////////////////////////////////////////////////

class Brush {
public:
  Brush();
  Brush(HBRUSH brush);
  ~Brush();

  HBRUSH Get() const;
  void Set(HBRUSH brush);

  operator HBRUSH() const;

private:
  HBRUSH brush_;
};

////////////////////////////////////////////////////////////////////////////////

class Font {
public:
  Font();
  Font(HFONT font);
  ~Font();

  HFONT Get() const;
  void Set(HFONT font);

  operator HFONT() const;

private:
  HFONT font_;
};

////////////////////////////////////////////////////////////////////////////////

class Rect : public RECT {
public:
  Rect();
  Rect(int l, int t, int r, int b);
  Rect(const RECT& rect);
  Rect(LPCRECT rect);

  BOOL operator==(const RECT& rect) const;
  BOOL operator!=(const RECT& rect) const;
  void operator=(const RECT& rect);

  int Height() const;
  int Width() const;

  void Copy(const RECT& rect);
  BOOL Equal(const RECT& rect) const;
  BOOL Inflate(int dx, int dy);
  BOOL Intersect(const RECT& rect1, const RECT& rect2);
  BOOL IsEmpty() const;
  BOOL Offset(int dx, int dy);
  BOOL PtIn(POINT point) const;
  BOOL Set(int left, int top, int right, int bottom);
  BOOL SetEmpty();
  BOOL Subtract(const RECT& rect1, const RECT& rect2);
  BOOL Union(const RECT& rect1, const RECT& rect2);
};

}  // namespace win

#endif  // TAIGA_WIN_GDI_H