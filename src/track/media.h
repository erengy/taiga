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
  kMediaModeWindowTitle,
  kMediaModeFileHandle,
  kMediaModeWinampApi,
  kMediaModeSpecialMessage,
  kMediaModeMplayer,
  kMediaModeWebBrowser
};

class MediaPlayer {
public:
  std::wstring GetPath() const;
  bool IsActive() const;

  std::wstring name;
  BOOL enabled;
  BOOL visible;
  int mode;
  std::vector<std::wstring> classes;
  std::vector<std::wstring> files;
  std::vector<std::wstring> folders;
  std::wstring engine;

  struct EditTitle {
    int mode;
    std::wstring value;
  };
  std::vector<EditTitle> edits;

  HWND window_handle;
};

class MediaPlayers {
public:
  MediaPlayers();
  ~MediaPlayers() {}

  bool Load();

  MediaPlayer* FindPlayer(const std::wstring& name);
  HWND GetCurrentWindowHandle();

  std::wstring current_player() const;
  bool player_running() const;
  void set_player_running(bool player_running);
  std::wstring current_title() const;
  void set_current_title(const std::wstring& title);
  bool title_changed() const;
  void set_title_changed(bool title_changed);

  MediaPlayer* CheckRunningPlayers();

  void EditTitle(std::wstring& str, const MediaPlayer* media_player);
  std::wstring GetTitle(HWND hwnd, const std::wstring& class_name, int mode);

  std::wstring GetTitleFromProcessHandle(HWND hwnd, ULONG process_id = 0);
  std::wstring GetTitleFromWinampAPI(HWND hwnd, bool use_unicode);
  std::wstring GetTitleFromSpecialMessage(HWND hwnd, const std::wstring& class_name);
  std::wstring GetTitleFromMPlayer();
  std::wstring GetTitleFromBrowser(HWND hwnd);
  std::wstring GetTitleFromStreamingMediaProvider(const std::wstring& url, std::wstring& title);

public:
  std::vector<MediaPlayer> items;

  class BrowserAccessibleObject : public base::AccessibleObject {
  public:
    bool AllowChildTraverse(base::AccessibleChild& child, LPARAM param = 0L);
  } acc_obj;

private:
  std::wstring current_player_;
  bool player_running_;

  std::wstring current_title_;
  bool title_changed_;
};

extern MediaPlayers MediaPlayers;

void ProcessMediaPlayerStatus(const MediaPlayer* media_player);
void ProcessMediaPlayerTitle(const MediaPlayer& media_player);

#endif  // TAIGA_TRACK_MEDIA_H