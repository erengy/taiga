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

#include "base/common.h"
#include "base/foreach.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "library/resource.h"
#include "taiga/announce.h"
#include "taiga/settings.h"
#include "taiga/stats.h"
#include "taiga/timer.h"
#include "track/feed.h"
#include "track/media.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_stats.h"
#include "ui/dlg/dlg_torrent.h"

namespace taiga {

Timer timer_history(kTimerHistory, 5 * 60);     //  5 minutes
Timer timer_library(kTimerLibrary, 30 * 60);    // 30 minutes
Timer timer_media(kTimerMedia, 2 * 60, false);  //  2 minutes
Timer timer_memory(kTimerMemory, 10 * 60);      // 10 minutes
Timer timer_stats(kTimerStats, 10);             // 10 seconds
Timer timer_torrents(kTimerTorrents, 60 * 60);  // 60 minutes

TimerManager timers;

////////////////////////////////////////////////////////////////////////////////

Timer::Timer(unsigned int id, int interval, bool repeat)
    : base::Timer(id, interval, repeat) {
}

void Timer::OnTimeout() {
  switch (id()) {
    case kTimerHistory:
      if (!History.queue.updating)
        History.queue.Check(true);
      break;

    case kTimerLibrary:
      ScanAvailableEpisodes(anime::ID_UNKNOWN, false, true);
      break;

    case kTimerMedia:
      ::Announcer.Do(taiga::kAnnounceToHttp | taiga::kAnnounceToMessenger |
                     taiga::kAnnounceToMirc | taiga::kAnnounceToSkype);
      if (!Settings.GetBool(taiga::kSync_Update_WaitPlayer)) {
        auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);
        if (anime_item)
          anime::UpdateList(*anime_item, CurrentEpisode);
      }
      break;

    case kTimerMemory:
      ImageDatabase.FreeMemory();
      break;

    case kTimerStats:
      Stats.CalculateAll();
      break;

    case kTimerTorrents:
      Aggregator.feeds.at(0).Check(
          Settings[taiga::kTorrent_Discovery_Source], true);
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
  InsertTimer(&timer_history);
  InsertTimer(&timer_library);
  InsertTimer(&timer_media);
  InsertTimer(&timer_memory);
  InsertTimer(&timer_stats);
  InsertTimer(&timer_torrents);
}

void TimerManager::UpdateIntervalsFromSettings() {
  timer_media.set_interval(
      Settings.GetInt(taiga::kSync_Update_Delay));

  timer_torrents.set_interval(
      Settings.GetInt(taiga::kTorrent_Discovery_AutoCheckInterval) * 60);
}

void TimerManager::OnTick() {
  // Library
  timer_library.set_enabled(!Settings.GetBool(taiga::kLibrary_WatchFolders));

  // Media
  auto media_player = MediaPlayers.CheckRunningPlayers();
  bool media_player_is_running = media_player != nullptr;
  bool media_player_is_active = media_player && media_player->IsActive();
  timer_media.set_enabled(media_player_is_running && media_player_is_active);
  ProcessMediaPlayerStatus(media_player);
  ui::DlgMain.UpdateStatusTimer();

  // Statistics
  Stats.uptime++;
  if (ui::DlgStats.IsVisible())
    ui::DlgStats.Refresh();
  timer_stats.set_enabled(ui::DlgStats.IsVisible() != FALSE);

  // Torrents
  timer_torrents.set_enabled(
      Settings.GetBool(taiga::kTorrent_Discovery_AutoCheckEnabled));
  ui::DlgTorrent.SetTimer(timer_torrents.ticks());

  // Tick
  foreach_(it, timers_)
    it->second->Tick();
}

void CALLBACK TimerManager::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent,
                                      DWORD dwTime) {
  timers.OnTick();
}

}  // namespace taiga