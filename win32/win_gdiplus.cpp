/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

using namespace Gdiplus;

// =============================================================================

CGdiPlus::CGdiPlus() {
  Gdiplus::GdiplusStartupInput input;
  Gdiplus::GdiplusStartup(&m_Token, &input, NULL);
}

CGdiPlus::~CGdiPlus() {
  Gdiplus::GdiplusShutdown(m_Token);
  m_Token = NULL;
}

HICON CGdiPlus::LoadIcon(const wstring& file) {
  HICON hIcon = NULL;
  
  Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(file.c_str());
  if (pBitmap) {
    pBitmap->GetHICON(&hIcon);
    delete pBitmap; pBitmap = NULL;
  }

  return hIcon;
}

HBITMAP CGdiPlus::LoadImage(const wstring& file) {
  HBITMAP hBitmap = NULL;
  
  Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(file.c_str());
  if (pBitmap) {
    pBitmap->GetHBITMAP(NULL, &hBitmap);
    delete pBitmap; pBitmap = NULL;
  }

  return hBitmap;
}