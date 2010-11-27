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

#ifndef THEME_H
#define THEME_H

#include "std.h"
#include "ui/ui_control.h"
#include "ui/ui_menu.h"

enum IconList16 {
  Icon16_Green,
  Icon16_Blue,
  Icon16_Red,
  Icon16_Gray,
  Icon16_Play,
  Icon16_New,
  Icon16_Tick,
  Icon16_Error,
  Icon16_Search,
  Icon16_Folder,
  Icon16_AppBlue,
  Icon16_AppGray,
  Icon16_Refresh,
  Icon16_Download,
  Icon16_Settings,
  Icon16_Cross,
  Icon16_Plus,
  Icon16_Minus,
  Icon16_TickSmall,
  ICONCOUNT_16PX
};

enum IconList24 {
  Icon24_Offline,
  Icon24_Online,
  Icon24_Sync,
  Icon24_MAL,
  Icon24_Folders,
  Icon24_Tools,
  Icon24_RSS,
  Icon24_Filter,
  Icon24_Settings,
  Icon24_About,
  ICONCOUNT_24PX
};

// =============================================================================

class CTheme {
public:
  CTheme();
  ~CTheme() {}

  // Image lists
  CImageList ImgList16;
  CImageList ImgList24;
  
  // Functions
  bool LoadImages();
  bool Read(const wstring& name);

  // Variables
  wstring File, Folder;
  CMenuList Menus;
  
  // Progress class
  class CThemeListProgress {
  public:
    class CThemeListProgressItem {
    public:
      void Draw(HDC hdc, const LPRECT lpRect);
      COLORREF Value[3];
      wstring  Type;
    }
      Background,
      Border,
      Buffer,
      Completed,
      Dropped,
      Seperator,
      Watching;
  } ListProgress;
};

extern CTheme UI;

#endif // THEME_H