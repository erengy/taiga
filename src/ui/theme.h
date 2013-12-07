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

#ifndef THEME_H
#define THEME_H

#include "base/std.h"
#include "win/ctrl/win_ctrl.h"
#include "win/win_menu.h"

enum Icons16px {
  ICON16_GREEN,
  ICON16_BLUE,
  ICON16_RED,
  ICON16_GRAY,
  ICON16_PLAY,
  ICON16_SEARCH,
  ICON16_FOLDER,
  ICON16_APP_BLUE,
  ICON16_APP_GRAY,
  ICON16_REFRESH,
  ICON16_DOWNLOAD,
  ICON16_SETTINGS,
  ICON16_CROSS,
  ICON16_PLUS,
  ICON16_MINUS,
  ICON16_ARROW_UP,
  ICON16_ARROW_DOWN,
  ICON16_FUNNEL,
  ICON16_FUNNEL_CROSS,
  ICON16_FUNNEL_TICK,
  ICON16_FUNNEL_PLUS,
  ICON16_FUNNEL_PENCIL,
  ICON16_CALENDAR,
  ICON16_CATEGORY,
  ICON16_SORT,
  ICON16_BALLOON,
  ICON16_CLOCK,
  ICON16_HOME,
  ICON16_DOCUMENT_A,
  ICON16_CHART,
  ICON16_FEED,
  ICON16_DETAILS,
  ICONCOUNT_16PX
};

enum Icons24px {
  ICON24_SYNC,
  ICON24_MAL,
  ICON24_FOLDERS,
  ICON24_TOOLS,
  ICON24_SETTINGS,
  ICON24_ABOUT,
  ICON24_GLOBE,
  ICON24_LIBRARY,
  ICON24_APPLICATION,
  ICON24_RECOGNITION,
  ICON24_SHARING,
  ICON24_FEED,
  ICONCOUNT_24PX
};

namespace theme {
const COLORREF COLOR_DARKBLUE = RGB(46, 81, 162);
const COLORREF COLOR_GRAY = RGB(230, 230, 230);
const COLORREF COLOR_LIGHTBLUE = RGB(225, 231, 245);
const COLORREF COLOR_LIGHTGRAY = RGB(248, 248, 248);
const COLORREF COLOR_LIGHTGREEN = RGB(225, 245, 231);
const COLORREF COLOR_LIGHTRED = RGB(245, 225, 231);
const COLORREF COLOR_MAININSTRUCTION = RGB(0x00, 0x33, 0x99);
}

// =============================================================================

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

class Theme {
public:
  Theme();
  ~Theme() {}
  
  bool CreateFonts(HDC hdc);
  bool Load();
  bool LoadImages();

public:
  win::MenuList Menus;

  win::ImageList ImgList16;
  win::ImageList ImgList24;

  Font font_bold;
  Font font_header;
  
  class ListBackground {
  public:
    ListBackground();
    ~ListBackground();
    wstring name;
    DWORD flags;
    int offset_x, offset_y;
    HBITMAP bitmap;
  } list_background;

  class ListProgress {
  public:
    class Item {
    public:
      void Draw(HDC hdc, const LPRECT rect);
      COLORREF value[3];
      wstring type;
    }
      aired,
      available,
      background,
      border,
      button,
      completed,
      dropped,
      separator,
      watching;
  } list_progress;

private:
  vector<wstring> icons16_, icons24_;
};

extern Theme UI;

#endif // THEME_H