/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <anisthesia.hpp>

namespace track {
namespace recognition {

enum class PlayStatus {
  Stopped,
  Playing,
  Updated,
};

using MediaPlayer = anisthesia::Player;

class MediaPlayers {
public:
  bool Load();

  bool IsPlayerActive() const;

  std::string current_player_name() const;
  std::wstring current_title() const;

  bool player_running() const;
  void set_player_running(bool player_running);

  bool title_changed() const;
  void set_title_changed(bool title_changed);

  bool CheckRunningPlayers();
  MediaPlayer* GetRunningPlayer();

public:
  std::vector<MediaPlayer> items;
  PlayStatus play_status = PlayStatus::Stopped;

private:
  std::unique_ptr<anisthesia::win::Result> current_result_;
  std::wstring current_title_;
  std::wstring current_page_title_;
  bool player_running_ = false;
  bool title_changed_ = false;
};

}  // namespace recognition

void ProcessMediaPlayerStatus(const recognition::MediaPlayer* media_player);
void ProcessMediaPlayerTitle(const recognition::MediaPlayer& media_player);

inline recognition::MediaPlayers media_players;

}  // namespace track
