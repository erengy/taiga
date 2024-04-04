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

#include "taiga/timer.h"

#include "base/string.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "media/library/queue.h"
#include "taiga/announce.h"
#include "taiga/settings.h"
#include "taiga/stats.h"
#include "track/feed_aggregator.h"
#include "track/media.h"
#include "track/scanner.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_stats.h"
#include "ui/dlg/dlg_torrent.h"
#include "ui/resource.h"

namespace taiga {

static Timer timer_anime_list(kTimerAnimeList, 60);    //  1 minute
static Timer timer_detection(kTimerDetection, 3);      //  3 seconds
static Timer timer_history(kTimerHistory, 5 * 60);     //  5 minutes
static Timer timer_library(kTimerLibrary, 30 * 60);    // 30 minutes
static Timer timer_media(kTimerMedia, 2 * 60, false);  //  2 minutes
static Timer timer_memory(kTimerMemory, 10 * 60);      // 10 minutes
static Timer timer_stats(kTimerStats, 10);             // 10 seconds
static Timer timer_torrents(kTimerTorrents, 60 * 60);  // 60 minutes

////////////////////////////////////////////////////////////////////////////////

Timer::Timer(unsigned int id, int interval, bool repeat)
    : base::Timer(id, interval, repeat) {
}

void Timer::OnTimeout() {
  switch (id()) {
    case kTimerAnimeList:
      ui::DlgAnimeList.listview.RefreshLastUpdateColumn();
      break;

    case kTimerDetection:
      track::media_players.CheckRunningPlayers();
      break;

    case kTimerHistory:
      library::queue.Check(true);
      break;

    case kTimerLibrary:
      ScanAvailableEpisodesQuick();
      break;

    case kTimerMedia:
      announcer.Do(taiga::kAnnounceToDiscord |
                   taiga::kAnnounceToHttp |
                   taiga::kAnnounceToMirc);
      if (!settings.GetSyncUpdateWaitPlayer()) {
        auto anime_item = anime::db.Find(CurrentEpisode.anime_id);
        if (anime_item)
          anime::UpdateList(*anime_item, CurrentEpisode);
      }
      break;

    case kTimerMemory:
      ui::image_db.FreeMemory();
      break;

    case kTimerStats:
      taiga::stats.CalculateAll();
      break;

    case kTimerTorrents:
      track::aggregator.CheckFeed(settings.GetTorrentDiscoverySource(), true);
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////

void TimerManager::Initialize() {
  // Set intervals based on user settings
  UpdateIntervalsFromSettings();

  // Initialize manager
  base::TimerManager::Initialize(nullptr, TimerProc);

  // Attach timers to the manager
  InsertTimer(&timer_anime_list);
  InsertTimer(&timer_detection);
  InsertTimer(&timer_history);
  InsertTimer(&timer_library);
  InsertTimer(&timer_media);
  InsertTimer(&timer_memory);
  InsertTimer(&timer_stats);
  InsertTimer(&timer_torrents);
}

void TimerManager::UpdateEnabledState() {
  // Library
  timer_library.set_enabled(!settings.GetLibraryWatchFolders());

  // Media
  bool media_player_is_running =
      track::media_players.GetRunningPlayer() != nullptr;
  bool media_player_is_active = track::media_players.IsPlayerActive();
  bool episode_processed = CurrentEpisode.processed || timer_media.ticks() == 0;
  timer_media.set_enabled(media_player_is_running && media_player_is_active &&
                          !episode_processed);

  // Statistics
  timer_stats.set_enabled(ui::DlgStats.IsVisible() != FALSE);

  // Torrents
  timer_torrents.set_enabled(
      settings.GetTorrentDiscoveryAutoCheckEnabled());
}

void TimerManager::UpdateIntervalsFromSettings() {
  timer_detection.set_interval(
      std::max(1, settings.GetRecognitionDetectionInterval()));

  timer_media.set_interval(
      settings.GetSyncUpdateDelay());

  timer_torrents.set_interval(
      settings.GetTorrentDiscoveryAutoCheckInterval() * 60);
}

void TimerManager::UpdateUi() {
  // Media
  ui::DlgMain.UpdateStatusTimer();
  track::ProcessMediaPlayerStatus(track::media_players.GetRunningPlayer());

  // Statistics
  if (ui::DlgStats.IsVisible())
    ui::DlgStats.Refresh();

  // Torrents
  ui::DlgTorrent.SetTimer(timer_torrents.ticks());
}

void TimerManager::OnTick() {
  ++taiga::stats.uptime;

  UpdateEnabledState();

  for (const auto& [id, timer] : timers_) {
    timer->Tick();
  }

  UpdateUi();
}

void CALLBACK TimerManager::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent,
                                      DWORD dwTime) {
  timers.OnTick();
}

}  // namespace taiga
