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

#ifndef MEDIA_H
#define MEDIA_H

#include "std.h"

// =============================================================================

/* Media players class */

class MediaPlayers {
public:
  MediaPlayers();
  virtual ~MediaPlayers() {}

  BOOL Load();
  BOOL Save();
  int Check();

  void EditTitle(wstring& str);
  wstring GetTitle(HWND hwnd, const wstring& class_name, int mode);
  bool TitleChanged() { return title_changed_; }

  wstring GetTitleFromProcessHandle(HWND hwnd, ULONG process_id = 0);
  wstring GetTitleFromWinampAPI(HWND hwnd, bool use_unicode);
  wstring GetTitleFromSpecialMessage(HWND hwnd, const wstring& class_name);
  wstring GetTitleFromMPlayer();
  
public:
  int Index, IndexOld;
  wstring CurrentCaption, NewCaption;

  class MediaPlayer {
  public:
    wstring Name;
    BOOL Enabled, Visible;
    int Mode;
    HWND WindowHandle;
    vector<wstring> Class, File, Folder;
    wstring GetPath();
    class EditTitle {
    public:
      int Mode;
      wstring Value;
    };
    vector<EditTitle> Edit;
  };
  vector<MediaPlayer> Items;

private:
  bool title_changed_;
};

extern MediaPlayers MediaPlayers;

#endif // MEDIA_H