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

#include "std.h"
#include "win32/win_control.h"
#include "win32/win_menu.h"

enum Icons16px {
  ICON16_GREEN,
  ICON16_BLUE,
  ICON16_RED,
  ICON16_GRAY,
  ICON16_PLAY,
  ICON16_ASTERISK,
  ICON16_TICK,
  ICON16_ERROR,
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
  ICON16_DOCUMENT_M,
  ICON16_CHART,
  ICON16_FEED,
  ICONCOUNT_16PX
};

enum Icons24px {
  ICON24_SYNC,
  ICON24_MAL,
  ICON24_FOLDERS,
  ICON24_SHARE,
  ICON24_TOOLS,
  ICON24_SETTINGS,
  ICON24_ABOUT,
  ICONCOUNT_24PX
};

// =============================================================================

class Theme {
public:
  Theme();
  ~Theme() {}
  
  bool LoadImages();
  bool Load(const wstring& name);

public:
  win32::MenuList Menus;

  win32::ImageList ImgList16;
  win32::ImageList ImgList24;
  
  class ListProgress {
  public:
    class Item {
    public:
      void Draw(HDC hdc, const LPRECT rect);
      COLORREF value[3];
      wstring type;
    }
      background,
      border,
      buffer,
      completed,
      dropped,
      separator,
      watching;
  } list_progress;

private:
  wstring file_, folder_;
};

extern Theme UI;

#endif // THEME_H