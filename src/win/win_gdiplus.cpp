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

#include "win_gdiplus.h"
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

namespace win {

GdiPlus::GdiPlus()
    : token_(0) {
  Gdiplus::GdiplusStartupInput input;
  Gdiplus::GdiplusStartup(&token_, &input, nullptr);
}

GdiPlus::~GdiPlus() {
  Gdiplus::GdiplusShutdown(token_);
  token_ = 0;
}

void GdiPlus::DrawRectangle(const HDC hdc, const RECT& rect,
                            Gdiplus::ARGB color) {
  const Gdiplus::SolidBrush brush = Gdiplus::Color(color);
  Gdiplus::Graphics graphics(hdc);
  graphics.FillRectangle(&brush, rect.left, rect.top,
                         rect.right - rect.left, rect.bottom - rect.top);
}

HICON GdiPlus::LoadIcon(const std::wstring& file) {
  HICON icon = nullptr;

  Gdiplus::Bitmap* gp_bitmap = Gdiplus::Bitmap::FromFile(file.c_str());
  if (gp_bitmap) {
    gp_bitmap->GetHICON(&icon);
    delete gp_bitmap;
    gp_bitmap = nullptr;
  }

  return icon;
}

HBITMAP GdiPlus::LoadImage(const std::wstring& file) {
  HBITMAP bitmap = nullptr;

  Gdiplus::Bitmap* gp_bitmap = Gdiplus::Bitmap::FromFile(file.c_str());
  if (gp_bitmap) {
    Gdiplus::Color color(Gdiplus::Color::AlphaMask);
    gp_bitmap->GetHBITMAP(color, &bitmap);
    delete gp_bitmap;
    gp_bitmap = nullptr;
  }

  return bitmap;
}

}  // namespace win