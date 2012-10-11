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

#ifndef GFX_H
#define GFX_H

#include "std.h"
#include "win32/win_gdi.h"
#include "win32/win_gdiplus.h"

extern win32::GdiPlus GdiPlus;

// =============================================================================

HFONT ChangeDCFont(HDC hdc, LPCWSTR lpFaceName, INT iSize, BOOL bBold, BOOL bItalic, BOOL bUnderline);
int GetTextHeight(HDC hdc);
BOOL GradientRect(HDC hdc, const LPRECT lpRect, DWORD dwColor1, DWORD dwColor2, bool bVertical);
BOOL DrawProgressBar(HDC hdc, const LPRECT lpRect, DWORD dwColor1, DWORD dwColor2, DWORD dwColor3);

COLORREF HexToARGB(const wstring& text);
win32::Rect ResizeRect(const win32::Rect& rect_dest, int src_width, int src_height, bool stretch, bool center_x, bool center_y);
int ScaleX(int value);
int ScaleY(int value);

class Image {
public:
  Image() : height(0), width(0), data(0) {}
  bool Load(const wstring& file);
  win32::Dc dc;
  win32::Rect rect;
  LPARAM data;
  int height, width;
};

#endif // GFX_H