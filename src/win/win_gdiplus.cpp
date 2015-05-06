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
#include <memory>

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
  HICON icon_handle = nullptr;

  std::unique_ptr<Gdiplus::Bitmap> bitmap(
      Gdiplus::Bitmap::FromFile(file.c_str()));

  if (bitmap)
    bitmap->GetHICON(&icon_handle);

  return icon_handle;
}

HBITMAP GdiPlus::LoadImage(const std::wstring& file, UINT width, UINT height) {
  HBITMAP bitmap_handle = nullptr;

  std::unique_ptr<Gdiplus::Bitmap> bitmap(
      Gdiplus::Bitmap::FromFile(file.c_str()));

  if (bitmap) {
    const Gdiplus::Color color(Gdiplus::Color::AlphaMask);

    if ((width > 0 && width != bitmap->GetWidth()) ||
        (height > 0 && height != bitmap->GetHeight())) {
      std::unique_ptr<Gdiplus::Bitmap> resized_bitmap(
          new Gdiplus::Bitmap(width, height));

      Gdiplus::Graphics graphics(resized_bitmap.get());
      graphics.ScaleTransform(
          width / static_cast<Gdiplus::REAL>(bitmap->GetWidth()),
          height / static_cast<Gdiplus::REAL>(bitmap->GetHeight()));
      graphics.DrawImage(bitmap.get(), 0, 0);

      resized_bitmap->GetHBITMAP(color, &bitmap_handle);

    } else {
      bitmap->GetHBITMAP(color, &bitmap_handle);
    }
  }

  return bitmap_handle;
}

}  // namespace win