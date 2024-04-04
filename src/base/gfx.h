/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>

#include <windows/win/gdi.h>

namespace base {

class Image {
public:
  bool Load(const std::wstring& file);

  win::Dc dc;
  win::Rect rect;
  LPARAM data = 0;
};

}  // namespace base

HBITMAP LoadImage(const std::wstring& file, UINT width, UINT height);

HFONT ChangeDCFont(HDC hdc, LPCWSTR lpFaceName, INT iSize, BOOL bBold, BOOL bItalic, BOOL bUnderline);
int GetTextHeight(HDC hdc);
int GetTextWidth(HDC hdc, const std::wstring& text);
BOOL GradientRect(HDC hdc, const LPRECT lpRect, DWORD dwColor1, DWORD dwColor2, bool bVertical);
BOOL DrawIconResource(int icon_id, HDC hdc, win::Rect& rect, bool center_x, bool center_y);
BOOL DrawProgressBar(HDC hdc, const LPRECT lpRect, DWORD dwColor1, DWORD dwColor2, DWORD dwColor3);

COLORREF HexToARGB(const std::wstring& text);
win::Rect ResizeRect(const win::Rect& rect_dest, int src_width, int src_height, bool stretch, bool center_x, bool center_y);
int ScaleX(int value);
int ScaleY(int value);
void RgbToHsv(float r, float g, float b, float& h, float& s, float& v);
void HsvToRgb(float& r, float& g, float& b, float h, float s, float v);
COLORREF ChangeColorBrightness(COLORREF color, float modifier);
