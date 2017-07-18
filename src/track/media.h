/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#pragma once

#include <string>
#include <vector>

#include <anisthesia/src/player.h>

enum class PlayStatus {
  Stopped,
  Playing,
  Updated,
};

class MediaPlayer : public anisthesia::Player {
public:
  MediaPlayer(const anisthesia::Player& player);
  bool enabled = true;
};

class MediaPlayers {
public:
  bool Load();

  bool IsPlayerActive() const;

  HWND current_window_handle() const;
  std::string current_player_name() const;
  bool player_running() const;
  void set_player_running(bool player_running);
  std::wstring current_title() const;
  bool title_changed() const;
  void set_title_changed(bool title_changed);

  MediaPlayer* CheckRunningPlayers();
  MediaPlayer* GetRunningPlayer();

  void EditTitle(std::wstring& str, const MediaPlayer& media_player);
  std::wstring GetTitle(HWND hwnd, const MediaPlayer& media_player);

  bool GetTitleFromProcessHandle(HWND hwnd, ULONG process_id, std::wstring& title);
  std::wstring GetTitleFromBrowser(HWND hwnd, const MediaPlayer& media_player);
  std::wstring GetTitleFromStreamingMediaProvider(const std::wstring& url, std::wstring& title);

public:
  std::vector<MediaPlayer> items;
  PlayStatus play_status = PlayStatus::Stopped;

private:
  HWND current_window_handle_ = nullptr;
  std::string current_player_name_;
  bool player_running_ = false;

  std::wstring current_title_;
  bool title_changed_ = false;
};

extern class MediaPlayers MediaPlayers;

void ProcessMediaPlayerStatus(const MediaPlayer* media_player);
void ProcessMediaPlayerTitle(const MediaPlayer& media_player);
