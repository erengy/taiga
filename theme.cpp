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

#include "std.h"

#include "theme.h"

#include "gfx.h"
#include "string.h"
#include "taiga.h"
#include "xml.h"

Theme UI;

// =============================================================================

Theme::Theme() {
  // Create image lists
  ImgList16.Create(16, 16);
  ImgList24.Create(24, 24);
  Menus.SetImageList(ImgList16.GetHandle());
}

bool Theme::Load(const wstring& name) {
  // Initialize
  folder_ = Taiga.GetDataPath() + L"theme\\" + name + L"\\";
  file_ = folder_ + L"Theme.xml";
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file_.c_str());
  if (result.status != status_ok) {
    MessageBox(NULL, L"Could not read theme file.", file_.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }
  
  // Read theme
  xml_node theme = doc.child(L"theme");
  xml_node icons16 = theme.child(L"icons").child(L"set_16px");
  xml_node icons24 = theme.child(L"icons").child(L"set_24px");
  xml_node progress = theme.child(L"list").child(L"progress");

  // Read icons
  icons16_.clear();
  icons24_.clear();
  for (xml_node icon = icons16.child(L"icon"); icon; icon = icon.next_sibling(L"icon"))
    icons16_.push_back(icon.attribute(L"name").value());
  for (xml_node icon = icons24.child(L"icon"); icon; icon = icon.next_sibling(L"icon"))
    icons24_.push_back(icon.attribute(L"name").value());

  // Read list
  #define READ_PROGRESS_DATA(x, name) \
    list_progress.x.type = progress.child(name).attribute(L"type").value(); \
    list_progress.x.value[0] = HexToARGB(progress.child(name).attribute(L"value_1").value()); \
    list_progress.x.value[1] = HexToARGB(progress.child(name).attribute(L"value_2").value()); \
    list_progress.x.value[2] = HexToARGB(progress.child(name).attribute(L"value_3").value());
  READ_PROGRESS_DATA(aired,      L"aired");
  READ_PROGRESS_DATA(available,  L"available");
  READ_PROGRESS_DATA(background, L"background");
  READ_PROGRESS_DATA(border,     L"border");
  READ_PROGRESS_DATA(button,     L"button");
  READ_PROGRESS_DATA(completed,  L"completed");
  READ_PROGRESS_DATA(dropped,    L"dropped");
  READ_PROGRESS_DATA(separator,  L"separator");
  READ_PROGRESS_DATA(watching,   L"watching");
  #undef READ_PROGRESS_DATA

  // Read menus
  Menus.Menu.clear();
  wstring menu_resource;
  ReadStringFromResource(L"IDR_MENU", L"DATA", menu_resource);
  result = doc.load(menu_resource.data());
  xml_node menus = doc.child(L"menus");
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
  for (size_t i = 0; i < ICONCOUNT_16PX && i < icons16_.size(); i++) {
    hBitmap = GdiPlus.LoadImage(folder_ + L"16px\\" + icons16_.at(i) + L".png");
    ImgList16.AddBitmap(hBitmap, CLR_NONE);
    DeleteObject(hBitmap);
  }
  for (size_t i = 0; i < ICONCOUNT_24PX && i < icons24_.size(); i++) {
    hBitmap = GdiPlus.LoadImage(folder_ + L"24px\\" + icons24_.at(i) + L".png");
    ImgList24.AddBitmap(hBitmap, CLR_NONE);
    DeleteObject(hBitmap);
  }

  return true;
}

// =============================================================================

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

bool Theme::CreateFonts(HDC hdc) {
  LOGFONT lFont = {0};
  lFont.lfCharSet = DEFAULT_CHARSET;
  lFont.lfOutPrecision = OUT_STRING_PRECIS;
  lFont.lfClipPrecision = CLIP_STROKE_PRECIS;
  lFont.lfQuality = PROOF_QUALITY;
  lFont.lfPitchAndFamily = VARIABLE_PITCH;

  // Bold font
  lFont.lfHeight = -MulDiv(9, GetDeviceCaps(hdc, LOGPIXELSY), 72);
  lFont.lfWeight = FW_BOLD;
  lstrcpy(lFont.lfFaceName, L"Segoe UI");
  font_bold.Set(::CreateFontIndirect(&lFont));

  // Header font
  lFont.lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
  lFont.lfWeight = FW_NORMAL;
  lstrcpy(lFont.lfFaceName, L"Segoe UI");
  font_header.Set(::CreateFontIndirect(&lFont));

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