/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

#include "std.h"
#include "gfx.h"
#include "string.h"
#include "taiga.h"
#include "theme.h"
#include "xml.h"

CTheme UI;

// =============================================================================

CTheme::CTheme() {
  // Create image lists
  ImgList16.Create(16, 16);
  ImgList24.Create(24, 24);
  Menus.SetImageList(ImgList16.GetHandle());
}

bool CTheme::Read(const wstring& name) {
  // Initialize
  Folder = Taiga.GetDataPath() + L"Theme\\" + name + L"\\";
  File = Folder + L"Theme.xml";
  
  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(ToANSI(File));
  if (result.status != status_ok) {
    MessageBox(NULL, L"Could not read theme file.", File.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  
  // Read theme
  xml_node theme = doc.child(L"theme");
  xml_node progress = theme.child(L"list").child(L"progress");
  xml_node menus = theme.child(L"menus");

  // Read list
  #define READ_PROGRESS_DATA(x, name) \
    ListProgress.x.Type     = progress.child(name).attribute(L"type").value(); \
    ListProgress.x.Value[0] = HexToARGB(progress.child(name).attribute(L"value_1").value()); \
    ListProgress.x.Value[1] = HexToARGB(progress.child(name).attribute(L"value_2").value()); \
    ListProgress.x.Value[2] = HexToARGB(progress.child(name).attribute(L"value_3").value());
  READ_PROGRESS_DATA(Background, L"background");
  READ_PROGRESS_DATA(Border,     L"border");
  READ_PROGRESS_DATA(Completed,  L"completed");
  READ_PROGRESS_DATA(Dropped,    L"dropped");
  READ_PROGRESS_DATA(Watching,   L"watching");
  READ_PROGRESS_DATA(Unknown,    L"unknown");

  // Read menus
  for (xml_node menu = menus.child(L"menu"); menu; menu = menu.next_sibling(L"menu")) {
    Menus.Create(menu.attribute(L"name").value(), menu.attribute(L"type").value());
    for (xml_node item = menu.child(L"item"); item; item = item.next_sibling(L"item")) {
      UI.Menus.Menu.back().CreateItem(
        item.attribute(L"action").value(), 
        item.attribute(L"name").value(), 
        item.attribute(L"sub").value(),  
        item.attribute(L"checked").as_bool(), 
        item.attribute(L"default").as_bool(), 
        !item.attribute(L"disabled").as_bool(), 
        item.attribute(L"column").as_bool(), 
        item.attribute(L"radio").as_bool());
    }
  }

  return true;
}

bool CTheme::LoadImages() {
  // Clear image lists
  ImgList16.Remove(-1);
  ImgList24.Remove(-1);
  
  // Populate image lists
  HBITMAP hBitmap;
  for (int i = 1; i <= ICONCOUNT_16PX; i++) {
    hBitmap = Gdiplus_LoadImage(Folder + L"16px\\" + (i < 10 ? L"0" : L"") + ToWSTR(i) + L".png");
    ImgList16.AddBitmap(hBitmap, CLR_NONE);
    DeleteObject(hBitmap);
  }
  for (int i = 1; i <= ICONCOUNT_24PX; i++) {
    hBitmap = Gdiplus_LoadImage(Folder + L"24px\\" + (i < 10 ? L"0" : L"") + ToWSTR(i) + L".png");
    ImgList24.AddBitmap(hBitmap, CLR_NONE);
    DeleteObject(hBitmap);
  }

  return true;
}

// =============================================================================

void CTheme::CThemeListProgress::CThemeListProgressItem::Draw(HDC hdc, const LPRECT lpRect) {
  // Solid
  if (Type == L"solid") {
    static const HBRUSH hbrSolid = CreateSolidBrush(Value[0]);
    FillRect(hdc, lpRect, hbrSolid);

  // Gradient
  } else if (Type == L"gradient") {
    GradientRect(hdc, lpRect, Value[0], Value[1], Value[2] > 0);

  // Progress bar
  } else if (Type == L"progress") {
    DrawProgressBar(hdc, lpRect, Value[0], Value[1], Value[2]);
  }
}