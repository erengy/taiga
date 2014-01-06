/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#ifndef TAIGA_TRACK_MEDIA_H
#define TAIGA_TRACK_MEDIA_H

#include <string>
#include <vector>

#include "base/accessibility.h"

enum MediaPlayerModes {
  MEDIA_MODE_WINDOWTITLE,
  MEDIA_MODE_FILEHANDLE,
  MEDIA_MODE_WINAMPAPI,
  MEDIA_MODE_SPECIALMESSAGE,
  MEDIA_MODE_MPLAYER,
  MEDIA_MODE_WEBBROWSER
};

class MediaPlayers {
public:
  MediaPlayers();
  ~MediaPlayers() {}

  BOOL Load();
  int Check();

  void EditTitle(std::wstring& str, int player_index);
  std::wstring GetTitle(HWND hwnd, const std::wstring& class_name, int mode);
  bool TitleChanged() const;
  void SetTitleChanged(bool title_changed);

  std::wstring GetTitleFromProcessHandle(HWND hwnd, ULONG process_id = 0);
  std::wstring GetTitleFromWinampAPI(HWND hwnd, bool use_unicode);
  std::wstring GetTitleFromSpecialMessage(HWND hwnd, const std::wstring& class_name);
  std::wstring GetTitleFromMPlayer();
  std::wstring GetTitleFromBrowser(HWND hwnd);

public:
  int index, index_old;
  std::wstring current_title, new_title;

  class MediaPlayer {
  public:
    std::wstring engine, name;
    BOOL enabled, visible;
    int mode;
    HWND window_handle;
    std::vector<std::wstring> classes, files, folders;
    std::wstring GetPath();
    class EditTitle {
    public:
      int mode;
      std::wstring value;
    };
    std::vector<EditTitle> edits;
  };
  std::vector<MediaPlayer> items;

  class BrowserAccessibleObject : public base::AccessibleObject {
  public:
    bool AllowChildTraverse(base::AccessibleChild& child, LPARAM param = 0L);
  } acc_obj;

private:
  bool title_changed_;
};

extern MediaPlayers MediaPlayers;

#endif  // TAIGA_TRACK_MEDIA_H