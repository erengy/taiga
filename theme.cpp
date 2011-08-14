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

#include "std.h"
#include "gfx.h"
#include "string.h"
#include "taiga.h"
#include "theme.h"
#include "xml.h"

Theme UI;

// =============================================================================

Theme::Theme() {
  // Create image lists
  ImgList16.Create(16, 16);
  ImgList24.Create(24, 24);
  Menus.SetImageList(ImgList16.GetHandle());
}

bool Theme::Read(const wstring& name) {
  // Initialize
  folder_ = Taiga.GetDataPath() + L"Theme\\" + name + L"\\";
  file_ = folder_ + L"Theme.xml";
  
  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file_.c_str());
  if (result.status != status_ok) {
    MessageBox(NULL, L"Could not read theme file.", file_.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  
  // Read theme
  xml_node theme = doc.child(L"theme");
  xml_node progress = theme.child(L"list").child(L"progress");
  xml_node menus = theme.child(L"menus");

  // Read list
  #define READ_PROGRESS_DATA(x, name) \
    list_progress.x.type = progress.child(name).attribute(L"type").value(); \
    list_progress.x.value[0] = HexToARGB(progress.child(name).attribute(L"value_1").value()); \
    list_progress.x.value[1] = HexToARGB(progress.child(name).attribute(L"value_2").value()); \
    list_progress.x.value[2] = HexToARGB(progress.child(name).attribute(L"value_3").value());
  READ_PROGRESS_DATA(background, L"background");
  READ_PROGRESS_DATA(border,     L"border");
  READ_PROGRESS_DATA(buffer,     L"buffer");
  READ_PROGRESS_DATA(completed,  L"completed");
  READ_PROGRESS_DATA(dropped,    L"dropped");
  READ_PROGRESS_DATA(separator,  L"separator");
  READ_PROGRESS_DATA(watching,   L"watching");
  #undef READ_PROGRESS_DATA

  // Read menus
  Menus.Menu.clear();
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

bool Theme::LoadImages() {
  // Clear image lists
  ImgList16.Remove(-1);
  ImgList24.Remove(-1);
  
  // Populate image lists
  HBITMAP hBitmap;
  for (int i = 1; i <= ICONCOUNT_16PX; i++) {
    hBitmap = GdiPlus.LoadImage(folder_ + L"16px\\" + (i < 10 ? L"0" : L"") + ToWSTR(i) + L".png");
    ImgList16.AddBitmap(hBitmap, CLR_NONE);
    DeleteObject(hBitmap);
  }
  for (int i = 1; i <= ICONCOUNT_24PX; i++) {
    hBitmap = GdiPlus.LoadImage(folder_ + L"24px\\" + (i < 10 ? L"0" : L"") + ToWSTR(i) + L".png");
    ImgList24.AddBitmap(hBitmap, CLR_NONE);
    DeleteObject(hBitmap);
  }

  return true;
}

// =============================================================================

void Theme::ListProgress::Item::Draw(HDC hdc, const LPRECT rect) {
  // Solid
  if (type == L"solid") {
    HBRUSH hbrSolid = CreateSolidBrush(value[0]);
    FillRect(hdc, rect, hbrSolid);
    DeleteObject(hbrSolid);

  // Gradient
  } else if (type == L"gradient") {
    GradientRect(hdc, rect, value[0], value[1], value[2] > 0);

  // Progress bar
  } else if (type == L"progress") {
    DrawProgressBar(hdc, rect, value[0], value[1], value[2]);
  }
}