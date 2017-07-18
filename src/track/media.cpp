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

#include <algorithm>
#include <regex>
#include <windows.h>

#include <anisthesia/src/win/strategy/open_files.h>
#include <anisthesia/src/win/strategy/window_title.h>

#include "base/file.h"
#include "base/process.h"
#include "base/string.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "library/anime_util.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/timer.h"
#include "track/media.h"
#include "track/recognition.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dialog.h"
#include "ui/ui.h"

class MediaPlayers MediaPlayers;

MediaPlayer::MediaPlayer(const anisthesia::Player& player)
    : anisthesia::Player(player) {
}

bool MediaPlayers::Load() {
  items.clear();

  const auto path = taiga::GetPath(taiga::Path::Media);
  std::vector<anisthesia::Player> players;

  if (!anisthesia::ParsePlayersData(WstrToStr(path), players)) {
    ui::DisplayErrorMessage(L"Could not read media list.", path.c_str());
    return false;
  }

  for (auto& player : players) {
    for (auto& executable : player.executables) {
      executable += ".exe";
    }
    items.push_back(player);
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MediaPlayers::IsPlayerActive() const {
  if (!Settings.GetBool(taiga::kSync_Update_CheckPlayer))
    return true;

  return current_window_handle_ &&
         current_window_handle_ == GetForegroundWindow();
}

HWND MediaPlayers::current_window_handle() const {
  return current_window_handle_;
}

std::string MediaPlayers::current_player_name() const {
  return current_player_name_;
}

bool MediaPlayers::player_running() const {
  return player_running_;
}

void MediaPlayers::set_player_running(bool player_running) {
  player_running_ = player_running;
}

std::wstring MediaPlayers::current_title() const {
  return current_title_;
}

bool MediaPlayers::title_changed() const {
  return title_changed_;
}

void MediaPlayers::set_title_changed(bool title_changed) {
  title_changed_ = title_changed;
}

////////////////////////////////////////////////////////////////////////////////

const MediaPlayer* GetPlayerFromWindow(const anisthesia::win::Window& window,
                                       const std::vector<MediaPlayer>& players) {
  for (const auto& player : players) {
    if (!player.enabled)
      continue;
    switch (player.type) {
      default:
      case anisthesia::PlayerType::Default:
        if (!Settings.GetBool(taiga::kRecognition_DetectMediaPlayers))
          continue;
        break;
      case anisthesia::PlayerType::WebBrowser:
        if (!Settings.GetBool(taiga::kRecognition_DetectStreamingMedia))
          continue;
        break;
    }

    auto class_name = std::find(
        player.windows.begin(), player.windows.end(), WstrToStr(window.class_name));
    if (class_name == player.windows.end())
      continue;

    auto file_name = std::find_if(
        player.executables.begin(), player.executables.end(),
        [&window](const std::string& filename) {
          return IsEqual(StrToWstr(filename), window.process_file_name);
        });
    if (file_name == player.executables.end())
      continue;

    return &player;
  }

  return nullptr;
}

MediaPlayer* MediaPlayers::CheckRunningPlayers() {
  using running_player_t = std::pair<HWND, const MediaPlayer*>;
  std::vector<running_player_t> running_players;

  auto enum_windows_proc = [&](const anisthesia::win::Window& window) -> bool {
    const auto player = GetPlayerFromWindow(window, items);
    if (player)
      running_players.push_back(std::make_pair(window.handle, player));
    return true;
  };

  anisthesia::win::EnumerateWindows(enum_windows_proc);

  const MediaPlayer* current_player = nullptr;
  std::string current_player_name;
  std::wstring current_title;
  HWND current_window_handle = nullptr;

  if (!running_players.empty()) {
    auto set_running_player = [&](running_player_t& running_player) {
      if (!running_player.first || !running_player.second)
        return false;
      current_title = GetTitle(running_player.first, *running_player.second);
      if (!current_title.empty()) {
        current_player = running_player.second;
        current_player_name = running_player.second->name;
        current_window_handle = running_player.first;
        return true;
      } else {
        running_player.second = nullptr;
        return false;
      }
    };

    // Stick with the previously detected window if possible
    bool anime_identified = anime::IsValidId(CurrentEpisode.anime_id);
    if (current_window_handle_ && anime_identified) {
      for (auto& running_player : running_players) {
        if (running_player.first == current_window_handle_)
          if (set_running_player(running_player))
            break;
      }
    }
    // Otherwise, get the first one with a valid title
    if (!current_player) {
      for (auto& running_player : running_players) {
        if (set_running_player(running_player))
          break;
      }
    }
  }

  if (current_player)
    player_running_ = true;
  current_player_name_ = current_player_name;
  if (current_title_ != current_title) {
    current_title_ = current_title;
    set_title_changed(true);
  }
  current_window_handle_ = current_window_handle;

  return const_cast<MediaPlayer*>(current_player);
}

MediaPlayer* MediaPlayers::GetRunningPlayer() {
  if (!current_player_name_.empty())
    for (auto& item : items)
      if (item.name == current_player_name_)
        return &item;

  return nullptr;
}

void MediaPlayers::EditTitle(std::wstring& str, const MediaPlayer& media_player) {
  if (str.empty() || media_player.window_title_format.empty())
    return;

  const std::string title = WstrToStr(str);
  const std::regex pattern(media_player.window_title_format);
  std::smatch match;
  std::regex_match(title, match, pattern);

  if (match.size() == 2)
    str = StrToWstr(match[1].str());
}

std::wstring MediaPlayers::GetTitle(HWND hwnd, const MediaPlayer& media_player) {
  for (const auto strategy : media_player.strategies) {
    switch (strategy) {
      default:
      case anisthesia::Strategy::WindowTitle: {
        std::wstring title = GetWindowTitle(hwnd);
        EditTitle(title, media_player);
        if (!title.empty())
          return title;
        break;
      }
      case anisthesia::Strategy::OpenFiles: {
        std::wstring title;
        if (GetTitleFromProcessHandle(hwnd, 0, title) && !title.empty())
          return title;
        break;
      }
      case anisthesia::Strategy::UiAutomation:
        return GetTitleFromBrowser(hwnd, media_player);
    }
  }

  return std::wstring();
}

////////////////////////////////////////////////////////////////////////////////

void ProcessMediaPlayerStatus(const MediaPlayer* media_player) {
  // Media player is running
  if (media_player) {
    ProcessMediaPlayerTitle(*media_player);

  // Media player is not running
  } else {
    auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id, false);

    // Media player was running, and the media was recognized
    if (anime_item) {
      bool processed = CurrentEpisode.processed;  // TODO: temporary solution...
      CurrentEpisode.Set(anime::ID_UNKNOWN);
      EndWatching(*anime_item, CurrentEpisode);
      if (Settings.GetBool(taiga::kSync_Update_WaitPlayer)) {
        CurrentEpisode.anime_id = anime_item->GetId();
        CurrentEpisode.processed = processed;
        UpdateList(*anime_item, CurrentEpisode);
        CurrentEpisode.anime_id = anime::ID_UNKNOWN;
      }
      taiga::timers.timer(taiga::kTimerMedia)->Reset();

    // Media player was running, but the media was not recognized
    } else if (MediaPlayers.player_running()) {
      ui::ClearStatusText();
      CurrentEpisode.Set(anime::ID_UNKNOWN);
      MediaPlayers.set_player_running(false);
      ui::DlgNowPlaying.SetCurrentId(anime::ID_UNKNOWN);
      taiga::timers.timer(taiga::kTimerMedia)->Reset();
    }
  }
}

void ProcessMediaPlayerTitle(const MediaPlayer& media_player) {
  auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);

  if (CurrentEpisode.anime_id == anime::ID_UNKNOWN) {
    if (!Settings.GetBool(taiga::kApp_Option_EnableRecognition))
      return;

    // Examine title and compare it with list items
    bool ignore_file = false;
    static track::recognition::ParseOptions parse_options;
    parse_options.parse_path = true;
    parse_options.streaming_media = media_player.type == anisthesia::PlayerType::WebBrowser;
    if (Meow.Parse(MediaPlayers.current_title(), parse_options, CurrentEpisode)) {
      bool is_inside_library_folders = true;
      if (Settings.GetBool(taiga::kSync_Update_OutOfRoot))
        if (!CurrentEpisode.folder.empty() && !Settings.library_folders.empty())
          is_inside_library_folders = anime::IsInsideLibraryFolders(CurrentEpisode.folder);
      if (is_inside_library_folders) {
        static track::recognition::MatchOptions match_options;
        match_options.allow_sequels = true;
        match_options.check_airing_date = true;
        match_options.check_anime_type = true;
        match_options.check_episode_number = true;
        auto anime_id = Meow.Identify(CurrentEpisode, true, match_options);
        if (anime::IsValidId(anime_id)) {
          // Recognized
          anime_item = AnimeDatabase.FindItem(anime_id);
          MediaPlayers.set_title_changed(false);
          CurrentEpisode.Set(anime_item->GetId());
          StartWatching(*anime_item, CurrentEpisode);
          return;
        } else if (!Meow.IsValidAnimeType(CurrentEpisode)) {
          ignore_file = true;
        } else if (!CurrentEpisode.file_extension().empty() &&
                   !Meow.IsValidFileExtension(CurrentEpisode.file_extension())) {
          ignore_file = true;
        }
      } else {
        ignore_file = true;
      }
    }
    // Not recognized
    CurrentEpisode.Set(anime::ID_NOTINLIST);
    if (!ignore_file)
      ui::OnRecognitionFail();

  } else {
    if (MediaPlayers.title_changed()) {
      // Caption changed
      MediaPlayers.set_title_changed(false);
      ui::ClearStatusText();
      bool processed = CurrentEpisode.processed;  // TODO: not a good solution...
      CurrentEpisode.Set(anime::ID_UNKNOWN);
      if (anime_item) {
        EndWatching(*anime_item, CurrentEpisode);
        CurrentEpisode.anime_id = anime_item->GetId();
        CurrentEpisode.processed = processed;
        UpdateList(*anime_item, CurrentEpisode);
        CurrentEpisode.anime_id = anime::ID_UNKNOWN;
      } else {
        ui::DlgNowPlaying.SetCurrentId(anime::ID_UNKNOWN);
      }
      taiga::timers.timer(taiga::kTimerMedia)->Reset();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

bool MediaPlayers::GetTitleFromProcessHandle(HWND hwnd, ULONG process_id,
                                             std::wstring& title) {
  if (hwnd != nullptr && process_id == 0)
    GetWindowThreadProcessId(hwnd, &process_id);

  std::set<DWORD> process_ids = {process_id};

  auto enum_files_proc = [&](const anisthesia::win::OpenFile& open_file) -> bool {
    if (Meow.IsValidFileExtension(GetFileExtension(open_file.path))) {
      if (Meow.IsValidAnimeType(open_file.path)) {
        title = open_file.path;
        TrimLeft(title, L"\\?");
      }
    }
    return true;
  };

  if (!anisthesia::win::EnumerateFiles(process_ids, enum_files_proc))
    return false;

  return true;
}
