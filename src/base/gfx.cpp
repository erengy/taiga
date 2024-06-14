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

#include <algorithm>
#include <memory>

#include <windows/win/gdi_plus.h>

#include "base/gfx.h"

namespace base {

bool Image::Load(const std::wstring& path) {
  ::DeleteObject(dc.DetachBitmap());

  if (dc.Get() == nullptr) {
    HDC hScreen = ::GetDC(nullptr);
    dc = ::CreateCompatibleDC(hScreen);
    ::ReleaseDC(NULL, hScreen);
  }

  Gdiplus::Bitmap bmp(path.c_str());
  rect.right = bmp.GetWidth();
  rect.bottom = bmp.GetHeight();

  HBITMAP hbmp = nullptr;
  bmp.GetHBITMAP(Gdiplus::Color::Transparent, &hbmp);

  if (!hbmp || !rect.right || !rect.bottom) {
    ::DeleteObject(hbmp);
    ::DeleteDC(dc.DetachDc());
    return false;
  }

  dc.AttachBitmap(hbmp);
  return true;
}

static win::GdiPlus* GdiPlus() {
  static const auto gdi_plus = std::make_unique<win::GdiPlus>();
  return gdi_plus.get();
}

}  // namespace base

////////////////////////////////////////////////////////////////////////////////

HBITMAP LoadImage(const std::wstring& file, UINT width, UINT height) {
  const auto gdi_plus = base::GdiPlus();
  return gdi_plus ? gdi_plus->LoadImage(file, width, height) : nullptr;
}

////////////////////////////////////////////////////////////////////////////////

HFONT ChangeDCFont(HDC hdc, LPCWSTR lpFaceName, INT iSize,
                   BOOL bBold, BOOL bItalic, BOOL bUnderline) {
  LOGFONT lFont;
  HFONT hFont = reinterpret_cast<HFONT>(GetCurrentObject(hdc, OBJ_FONT));
  GetObject(hFont, sizeof(LOGFONT), &lFont);

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

int GetTextHeight(HDC hdc) {
  SIZE size = {0};
  GetTextExtentPoint32(hdc, L"T", 1, &size);
  return size.cy;
}

int GetTextWidth(HDC hdc, const std::wstring& text) {
  SIZE size = {0};
  GetTextExtentPoint32(hdc, text.c_str(), static_cast<int>(text.size()), &size);
  return size.cx;
}

BOOL GradientRect(HDC hdc, const LPRECT lpRect, DWORD dwColor1, DWORD dwColor2,
                  bool bVertical) {
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

  return GdiGradientFill(hdc, vertex, 2, &gRect, 1,
                         static_cast<ULONG>(bVertical));
}

BOOL DrawIconResource(int icon_id, HDC hdc, win::Rect& rect,
                      bool center_x, bool center_y) {
  const auto avialable_icons = {128, 64, 48, 32, 16};
  const int available_width = rect.Width();
  const int available_height = rect.Height();

  int icon_size = 0;
  for (const auto icon_width : avialable_icons) {
    if (available_width >= icon_width) {
      icon_size = icon_width;
      break;
    }
  }

  HICON icon = reinterpret_cast<HICON>(LoadImage(GetModuleHandle(nullptr),
      MAKEINTRESOURCE(icon_id), IMAGE_ICON, icon_size, icon_size, LR_SHARED));

  if (icon && icon_size) {
    if (center_x)
      rect.left += (available_width - icon_size) / 2;
    if (center_y)
      rect.top += (available_height - icon_size) / 2;
    rect.right = rect.left + icon_size;
    rect.bottom = rect.top + icon_size;

    return DrawIconEx(hdc, rect.left, rect.top, icon, icon_size, icon_size,
                      0, nullptr, DI_NORMAL);
  }

  return FALSE;
}

BOOL DrawProgressBar(HDC hdc, const LPRECT lpRect, DWORD dwColor1, DWORD dwColor2,
                     DWORD dwColor3) {
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

////////////////////////////////////////////////////////////////////////////////

COLORREF HexToARGB(const std::wstring& text) {
  if (text.length() < 6)
    return 0;

  const size_t i = text.length() - 6;

  unsigned int r, g, b;
  r = wcstoul(text.substr(i + 0, 2).c_str(), NULL, 16);
  g = wcstoul(text.substr(i + 2, 2).c_str(), NULL, 16);
  b = wcstoul(text.substr(i + 4, 2).c_str(), NULL, 16);

  return RGB(r, g, b);
}

win::Rect ResizeRect(const win::Rect& rect_dest,
                     int src_width, int src_height,
                     bool stretch,
                     bool center_x, bool center_y) {
  win::Rect rect = rect_dest;

  float dest_width = static_cast<float>(rect_dest.Width());
  float dest_height = static_cast<float>(rect_dest.Height());
  float image_width = static_cast<float>(src_width);
  float image_Height = static_cast<float>(src_height);

  // Source < Destination (no need to resize)
  if ((image_width < dest_width) && (image_Height < dest_height) && !stretch) {
    rect.right = rect.left + src_width;
    rect.bottom = rect.top + src_height;
    if (center_x)
      rect.Offset(static_cast<int>((image_width - image_width) / 2.0f), 0);
    if (center_y)
      rect.Offset(0, static_cast<int>((dest_height - image_Height) / 2.0f));

  // Source > Destination (resize)
  } else {
    // Calculate aspect ratios
    float dest_ratio = dest_width / dest_height;
    float image_ratio = image_width / image_Height;

    // Width > Height
    if (image_ratio > dest_ratio) {
      rect.bottom = rect.top +
          static_cast<int>(dest_width * (1.0f / image_ratio));
      if (center_y)
        rect.Offset(0, static_cast<int>((dest_height - rect.Height()) / 2.0f));
    // Height > Width
    } else if (image_ratio < dest_ratio) {
      rect.right = rect.left + static_cast<int>(dest_height * image_ratio);
      if (center_x)
        rect.Offset(static_cast<int>((dest_width - rect.Width()) / 2.0f), 0);
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

void RgbToHsv(float r, float g, float b, float& h, float& s, float& v) {
  float rgb_min = std::min(r, std::min(g, b));
  float rgb_max = std::max(r, std::max(g, b));
  float rgb_delta = rgb_max - rgb_min;

  s = rgb_delta / (rgb_max + 1e-20f);
  v = rgb_max;

  if (r == rgb_max) {
    h = (g - b) / (rgb_delta + 1e-20f);
  } else if (g == rgb_max) {
    h = 2 + (b - r) / (rgb_delta + 1e-20f);
  } else {
    h = 4 + (r - g) / (rgb_delta + 1e-20f);
  }

  if (h < 0)
    h += 6.0f;
  h *= (1.0f / 6.0f);
}

void HsvToRgb(float& r, float& g, float& b, float h, float s, float v) {
  if (s == 0) {
    r = g = b = v;
    return;
  }

  h /= 60;
  int i = static_cast<int>(floor(h));
  float f = h - i;
  float p = v * (1 - s);
  float q = v * (1 - s * f);
  float t = v * (1 - s * (1 - f));

  switch (i) {
    case 0:
      r = v; g = t; b = p;
      break;
    case 1:
      r = q; g = v; b = p;
      break;
    case 2:
      r = p; g = v; b = t;
      break;
    case 3:
      r = p; g = q; b = v;
      break;
    case 4:
      r = t; g = p; b = v;
      break;
    default:
      r = v; g = p; b = q;
      break;
  }
}

COLORREF ChangeColorBrightness(COLORREF color, float modifier) {
  float r = static_cast<float>(GetRValue(color)) / 255.0f;
  float g = static_cast<float>(GetGValue(color)) / 255.0f;
  float b = static_cast<float>(GetBValue(color)) / 255.0f;

  float h, s, v;

  RgbToHsv(r, g, b, h, s, v);

  v += modifier;
  if (v < 0.0f) v = 0.0f;
  if (v > 1.0f) v = 1.0f;

  HsvToRgb(r, g, b, h, s, v);

  return RGB(static_cast<BYTE>(r * 255),
             static_cast<BYTE>(g * 255),
             static_cast<BYTE>(b * 255));
}
