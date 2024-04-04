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

#include <algorithm>

#include <windows/win/string.h>

#include "track/media.h"

#include "base/file.h"
#include "base/log.h"
#include "base/process.h"
#include "base/string.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/timer.h"
#include "track/episode.h"
#include "track/media_stream.h"
#include "track/recognition.h"
#include "ui/dialog.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/ui.h"

namespace track {
namespace recognition {

bool MediaPlayers::Load() {
  items.clear();

  std::vector<anisthesia::Player> players;
  const auto path = taiga::GetPath(taiga::Path::Media);

  std::wstring resource;
  win::ReadStringFromResource(L"IDR_PLAYERS", L"DATA", resource);
  if (anisthesia::ParsePlayersData(WstrToStr(resource), players)) {
    for (const auto& player : players) {
      items.push_back(player);
    }
  }

  players.clear();
  if (anisthesia::ParsePlayersFile(WstrToStr(path), players)) {
    for (const auto& player : players) {
      auto it = std::find_if(items.begin(), items.end(),
          [&player](const MediaPlayer& item) {
            return item.name == player.name;
          });
      if (it != items.end()) {
        LOGD(L"Override: {}", StrToWstr(player.name));
        *it = player;
      } else {
        LOGD(L"Add: {}", StrToWstr(player.name));
        items.push_back(player);
      }
    }
  }

  if (items.empty()) {
    ui::DisplayErrorMessage(L"Could not read media players data.",
                            path.c_str());
    return false;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MediaPlayers::IsPlayerActive() const {
  if (!taiga::settings.GetSyncUpdateCheckPlayer())
    return true;

  return current_result_ &&
         current_result_->window.handle == GetForegroundWindow();
}

std::string MediaPlayers::current_player_name() const {
  if (current_result_)
    return current_result_->player.name;
  return std::string();
}

std::wstring MediaPlayers::current_title() const {
  return current_title_;
}

bool MediaPlayers::player_running() const {
  return player_running_;
}

void MediaPlayers::set_player_running(bool player_running) {
  player_running_ = player_running;
}

bool MediaPlayers::title_changed() const {
  return title_changed_;
}

void MediaPlayers::set_title_changed(bool title_changed) {
  title_changed_ = title_changed;
}

////////////////////////////////////////////////////////////////////////////////

std::vector<anisthesia::Player> GetEnabledPlayers(
    const std::vector<MediaPlayer>& players) {
  std::vector<anisthesia::Player> results;

  for (const auto& player : players) {
    if (!taiga::settings.GetMediaPlayerEnabled(StrToWstr(player.name)))
      continue;

    switch (player.type) {
      default:
      case anisthesia::PlayerType::Default:
        if (!taiga::settings.GetRecognitionDetectMediaPlayers())
          continue;
        break;
      case anisthesia::PlayerType::WebBrowser:
        if (!taiga::settings.GetRecognitionDetectStreamingMedia())
          continue;
        break;
    }

    results.push_back(player);
  }

  return results;
}

bool GetTitleFromDefaultPlayer(const std::vector<anisthesia::Media>& media,
                               std::wstring& title) {
  bool invalid_file = false;
  const bool check_library_folders =
      taiga::settings.GetSyncUpdateOutOfRoot() &&
      !taiga::settings.GetLibraryFolders().empty();

  for (const auto& item : media) {
    for (const auto& information : item.information) {
      auto value = StrToWstr(information.value);

      switch (information.type) {
        case anisthesia::MediaInfoType::File: {
          value = GetNormalizedPath(value);
          const auto file_extension = GetFileExtension(value);

          // Invalid path
          if (check_library_folders && !anime::IsInsideLibraryFolders(value)) {
            if (Meow.IsValidFileExtension(file_extension) ||
                Meow.IsAudioFileExtension(file_extension)) {
              invalid_file = true;
            }
            continue;
          }

          // Video file
          if (Meow.IsValidFileExtension(file_extension)) {
            // Invalid type
            if (!Meow.IsValidAnimeType(value)) {
              invalid_file = true;
              continue;
            }
            // Valid video file
            title = value;
            return true;
          }

          // Audio file
          if (Meow.IsAudioFileExtension(file_extension)) {
            invalid_file = true;
            continue;
          }

          // Unknown extension
          break;
        }

        default:
        case anisthesia::MediaInfoType::Title:
        case anisthesia::MediaInfoType::Unknown: {
          if (!invalid_file) {
            title = value;
            return true;
          }
          break;
        }
      }
    }
  }

  return false;
}

bool GetTitleFromWebBrowser(const std::vector<anisthesia::Media>& media,
                            const std::wstring& current_title,
                            std::wstring& current_page_title,
                            std::wstring& title) {
  std::wstring page_title;
  std::wstring url;
  std::vector<std::wstring> tabs;

  for (const auto& item : media) {
    for (const auto& information : item.information) {
      const auto value = StrToWstr(information.value);
      switch (information.type) {
        case anisthesia::MediaInfoType::Tab:
          tabs.push_back(value);
          break;
        case anisthesia::MediaInfoType::Title:
          if (page_title.empty())
            page_title = value;
          break;
        case anisthesia::MediaInfoType::Url:
          url = value;
          break;
        // We prefer the regular window title, because Chrome appends the audio
        // state to tab accessibility labels.
        case anisthesia::MediaInfoType::Unknown:
          page_title = value;
          break;
      }
    }
  }

  NormalizeWebBrowserTitle(url, page_title);
  for (auto& tab : tabs) {
    NormalizeWebBrowserTitle(url, tab);
  }

  if (anime::IsValidId(CurrentEpisode.anime_id)) {
    if (!page_title.empty() && page_title == current_page_title) {
      title = current_title;
      return true;
    }
    for (const auto& tab : tabs) {
      if (!tab.empty() && tab == current_page_title) {
        title = current_title;
        return true;
      }
    }
  }

  ParseOptions parse_options;
  parse_options.parse_path = false;
  parse_options.streaming_media = true;
  if (!Meow.IsValidAnimeType(page_title, parse_options)) {
    current_page_title.clear();
    return false;
  }

  title = page_title;

  if (GetTitleFromStreamingMediaProvider(url, title)) {
    current_page_title = page_title;
    return true;
  } else {
    current_page_title.clear();
    return false;
  }
}

bool GetTitleFromResult(const anisthesia::win::Result& result,
                        const std::wstring& current_title,
                        std::wstring& current_page_title,
                        std::wstring& title) {
  switch (result.player.type) {
    case anisthesia::PlayerType::Default:
      return GetTitleFromDefaultPlayer(result.media, title);
    case anisthesia::PlayerType::WebBrowser:
      return GetTitleFromWebBrowser(result.media, current_title,
                                    current_page_title, title);
  }

  return false;
}

bool MediaPlayers::CheckRunningPlayers() {
  const auto enabled_players = GetEnabledPlayers(items);
  std::vector<anisthesia::win::Result> results;

  const auto media_proc = [](const anisthesia::MediaInfo&) {
    return true;  // Accept all media
  };

  if (anisthesia::win::GetResults(enabled_players, media_proc, results)) {
    // Stick with the previously detected window if possible
    if (current_result_ && anime::IsValidId(CurrentEpisode.anime_id)) {
      auto it = std::find_if(results.begin(), results.end(),
          [this](const anisthesia::win::Result& result) {
            return result.window.handle == current_result_->window.handle;
          });
      if (it != results.end())
        std::rotate(results.begin(), it, it + 1);  // Move to front
    }

    std::wstring title;

    for (const auto& result : results) {
      if (GetTitleFromResult(result, current_title_,
                             current_page_title_, title)) {
        current_result_.reset(new anisthesia::win::Result(result));

        if (current_title_ != title) {
          current_title_ = title;
          set_title_changed(true);
        }
        player_running_ = true;

        return true;
      }
    }
  }

  current_result_.reset();
  return false;
}

MediaPlayer* MediaPlayers::GetRunningPlayer() {
  if (current_result_)
    for (auto& item : items)
      if (item.name == current_result_->player.name)
        return &item;

  return nullptr;
}

}  // namespace recognition

////////////////////////////////////////////////////////////////////////////////

void ProcessMediaPlayerStatus(const recognition::MediaPlayer* media_player) {
  // Media player is running
  if (media_player) {
    ProcessMediaPlayerTitle(*media_player);

  // Media player is not running
  } else {
    auto anime_item = anime::db.Find(CurrentEpisode.anime_id, false);

    // Media player was running, and the media was recognized
    if (anime_item) {
      bool processed = CurrentEpisode.processed;  // TODO: temporary solution...
      CurrentEpisode.Set(anime::ID_UNKNOWN);
      EndWatching(*anime_item, CurrentEpisode);
      if (taiga::settings.GetSyncUpdateWaitPlayer()) {
        CurrentEpisode.anime_id = anime_item->GetId();
        CurrentEpisode.processed = processed;
        UpdateList(*anime_item, CurrentEpisode);
        CurrentEpisode.anime_id = anime::ID_UNKNOWN;
      }
      taiga::timers.timer(taiga::kTimerMedia)->Reset();

    // Media player was running, but the media was not recognized
    } else if (media_players.player_running()) {
      ui::ClearStatusText();
      CurrentEpisode.Set(anime::ID_UNKNOWN);
      media_players.set_player_running(false);
      ui::DlgNowPlaying.SetCurrentId(anime::ID_UNKNOWN);
      taiga::timers.timer(taiga::kTimerMedia)->Reset();
    }
  }
}

void ProcessMediaPlayerTitle(const recognition::MediaPlayer& media_player) {
  auto anime_item = anime::db.Find(CurrentEpisode.anime_id);

  if (CurrentEpisode.anime_id == anime::ID_UNKNOWN) {
    if (!taiga::settings.GetAppOptionEnableRecognition())
      return;

    // Examine title and compare it with list items
    static track::recognition::ParseOptions parse_options;
    parse_options.parse_path = true;
    parse_options.streaming_media = media_player.type == anisthesia::PlayerType::WebBrowser;
    if (Meow.Parse(media_players.current_title(), parse_options, CurrentEpisode)) {
      static track::recognition::MatchOptions match_options;
      match_options.allow_sequels = true;
      match_options.check_airing_date = true;
      match_options.check_anime_type = true;
      match_options.check_episode_number = true;
      match_options.streaming_media = media_player.type == anisthesia::PlayerType::WebBrowser;
      auto anime_id = Meow.Identify(CurrentEpisode, true, match_options);
      if (anime::IsValidId(anime_id)) {
        // Recognized
        anime_item = anime::db.Find(anime_id);
        media_players.set_title_changed(false);
        CurrentEpisode.Set(anime_item->GetId());
        StartWatching(*anime_item, CurrentEpisode);
        return;
      }
    }
    // Not recognized
    CurrentEpisode.Set(anime::ID_NOTINLIST);
    ui::OnRecognitionFail();

  } else {
    if (media_players.title_changed()) {
      // Caption changed
      media_players.set_title_changed(false);
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

}  // namespace track
