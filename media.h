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

#ifndef MEDIA_H
#define MEDIA_H

#include "std.h"

// =============================================================================

/* Media players class */

class CMediaPlayers {
public:
  BOOL Read();
  BOOL Write();
  int Check();
  void EditTitle(wstring& str);
  wstring GetTitle(HWND hwnd, const wstring& class_name, int mode);
  bool TitleChanged() { return m_bTitleChanged; }

  //
  wstring GetTitleFromProcessHandle(HWND hwnd);
  wstring GetTitleFromWinampAPI(HWND hwnd, bool use_unicode);
  wstring GetTitleFromSpecialMessage(HWND hwnd, const wstring& class_name);
  
  // Variables
  int Index, IndexOld;
  wstring CurrentCaption, NewCaption;

  // Player class
  class CMediaPlayer {
  public:
    wstring Name;
    BOOL Enabled, Visible;
    int Mode;
    HWND WindowHandle;
    vector<wstring> Class, File, Folder;
    wstring GetPath();
    class CEditTitle {
    public:
      int Mode;
      wstring Value;
    };
    vector<CEditTitle> Edit;
  };
  vector<CMediaPlayer> Item;

private:
  bool m_bTitleChanged;
};

extern CMediaPlayers MediaPlayers;

#endif // MEDIA_H