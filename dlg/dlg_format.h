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

#ifndef DLG_FORMAT_H
#define DLG_FORMAT_H

#include "../std.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

enum {
  FORMAT_MODE_HTTP,
  FORMAT_MODE_MESSENGER,
  FORMAT_MODE_MIRC,
  FORMAT_MODE_SKYPE,
  FORMAT_MODE_TWITTER,
  FORMAT_MODE_BALLOON
};

// =============================================================================

class FormatDialog : public CDialog {
public:
  FormatDialog();
  ~FormatDialog() {}
  
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  void OnOK();

public:
  void ColorizeText();
  void RefreshPreviewText();

public:
  int mode;

private:
  CRichEdit rich_edit_;
};

extern FormatDialog FormatDialog;

#endif // DLG_FORMAT_H