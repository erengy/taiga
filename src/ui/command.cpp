/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include <windows/win/common_dialogs.h>

#include "ui/command.h"

#include "base/file.h"
#include "base/format.h"
#include "base/log.h"
#include "base/process.h"
#include "base/string.h"
#include "base/url.h"
#include "media/anime_db.h"
#include "media/anime_season_db.h"
#include "media/anime_util.h"
#include "media/library/export.h"
#include "media/library/history.h"
#include "media/library/list_util.h"
#include "media/library/queue.h"
#include "sync/anilist_util.h"
#include "sync/kitsu_util.h"
#include "sync/myanimelist_util.h"
#include "sync/sync.h"
#include "taiga/announce.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "track/monitor.h"
#include "track/play.h"
#include "track/scanner.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_search.h"
#include "ui/dlg/dlg_season.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_torrent.h"
#include "ui/dlg/dlg_feed_filter.h"
#include "ui/dialog.h"
#include "ui/menu.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace ui {

static std::pair<std::wstring, std::wstring> ParseCommand(std::wstring command) {
  std::wstring body;

  const size_t pos = command.find('(');
  if (pos != command.npos) {
    body = command.substr(pos + 1, command.find_last_of(')') - (pos + 1));
    command.resize(pos);
  }

  Trim(command);
  Trim(body);

  return {command, body};
}

void ExecuteCommand(const std::wstring& str, WPARAM wParam, LPARAM lParam) {
  LOGD(str);

  auto [command, body] = ParseCommand(str);

  if (command.empty())
    return;

  //////////////////////////////////////////////////////////////////////////////
  // Taiga

  // CheckUpdates()
  //   Checks for a new version of the program.
  if (command == L"CheckUpdates") {
    ui::ShowDialog(ui::Dialog::Update);

  // Exit(), Quit()
  //   Exits from Taiga.
  } else if (command == L"Exit" || command == L"Quit") {
    ui::DlgMain.Destroy();

  //////////////////////////////////////////////////////////////////////////////
  // Export

  // ExportAsMalXml()
  //   Exports library in MAL XML format.
  } else if (command == L"ExportAsMalXml") {
    std::wstring path;
    if (win::BrowseForFolder(ui::GetWindowHandle(ui::Dialog::Main),
                             L"Select Export Location", L"", path)) {
      AddTrailingSlash(path);
      path += L"animelist_{}.xml"_format(std::time(nullptr));
      if (library::ExportAsMalXml(path)) {
        ui::ChangeStatusText(L"Exported list to: " + path);
      } else {
        ui::ChangeStatusText(L"Could not export list to: " + path);
      }
    }

  // ExportAsMarkdown()
  //   Exports library in Markdown format.
  } else if (command == L"ExportAsMarkdown") {
    std::wstring path;
    if (win::BrowseForFolder(ui::GetWindowHandle(ui::Dialog::Main),
                             L"Select Export Location", L"", path)) {
      AddTrailingSlash(path);
      path += L"animelist_{}.md"_format(std::time(nullptr));
      if (library::ExportAsMarkdown(path)) {
        ui::ChangeStatusText(L"Exported list to: " + path);
      } else {
        ui::ChangeStatusText(L"Could not export list to: " + path);
      }
    }

  //////////////////////////////////////////////////////////////////////////////
  // Services

  // Synchronize()
  //   Synchronizes local and remote lists.
  } else if (command == L"Synchronize") {
    sync::Synchronize();

  // SearchAnime()
  //   wParam is a BOOL value that defines local search.
  } else if (command == L"SearchAnime") {
    if (body.empty())
      return;
    const bool local_search = wParam != FALSE;
    ui::DlgMain.navigation.SetCurrentPage(ui::kSidebarItemSearch);
    ui::DlgMain.edit.SetText(body);
    ui::DlgSearch.Search(body, local_search);

  // ViewAnimePage
  //   Opens up anime page on the active service.
  //   wParam is a BOOL value that defines lParam.
  //   lParam is an anime ID, or a pointer to a vector of anime IDs.
  } else if (command == L"ViewAnimePage") {
    const auto view_anime_page = [](const int anime_id) {
      switch (sync::GetCurrentServiceId()) {
        case sync::ServiceId::MyAnimeList:
          sync::myanimelist::ViewAnimePage(anime_id);
          break;
        case sync::ServiceId::Kitsu:
          sync::kitsu::ViewAnimePage(anime_id);
          break;
        case sync::ServiceId::AniList:
          sync::anilist::ViewAnimePage(anime_id);
          break;
      }
    };
    if (!wParam) {
      const int anime_id = static_cast<int>(lParam);
      view_anime_page(anime_id);
    } else {
      const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
      for (const auto anime_id : anime_ids) {
        view_anime_page(anime_id);
      }
    }

  // WatchTrailer
  //   Opens up YouTube page for trailer video.
  //   wParam is a BOOL value that defines lParam.
  //   lParam is an anime ID, or a pointer to a vector of anime IDs.
  } else if (command == L"WatchTrailer") {
    const auto view_trailer = [](const int anime_id) {
      if (auto anime_item = anime::db.Find(anime_id)) {
        if (!anime_item->GetTrailerId().empty()) {
          ExecuteLink(L"https://youtu.be/" + anime_item->GetTrailerId());
        }
      }
    };
    if (!wParam) {
      const int anime_id = static_cast<int>(lParam);
      view_trailer(anime_id);
    } else {
      const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
      for (const auto anime_id : anime_ids) {
        view_trailer(anime_id);
      }
    }

  // ViewUpcomingAnime
  //   Opens up upcoming anime page on MAL.
  } else if (command == L"ViewUpcomingAnime") {
    switch (sync::GetCurrentServiceId()) {
      case sync::ServiceId::MyAnimeList:
        sync::myanimelist::ViewUpcomingAnime();
        break;
    }

  // MalViewPanel()
  // MalViewProfile()
  // MalViewHistory()
  //   Opens up MyAnimeList user pages.
  } else if (command == L"MalViewPanel") {
    sync::myanimelist::ViewPanel();
  } else if (command == L"MalViewProfile") {
    sync::myanimelist::ViewProfile();
  } else if (command == L"MalViewHistory") {
    sync::myanimelist::ViewHistory();

  // KitsuViewFeed()
  // KitsuViewLibrary()
  // KitsuViewProfile()
  //   Opens up Kitsu user pages.
  } else if (command == L"KitsuViewFeed") {
    sync::kitsu::ViewFeed();
  } else if (command == L"KitsuViewLibrary") {
    sync::kitsu::ViewLibrary();
  } else if (command == L"KitsuViewProfile") {
    sync::kitsu::ViewProfile();

  // AniListViewProfile()
  // AniListViewStats()
  //   Opens up AniList user pages.
  } else if (command == L"AniListViewProfile") {
    sync::anilist::ViewProfile();
  } else if (command == L"AniListViewStats") {
    sync::anilist::ViewStats();

  //////////////////////////////////////////////////////////////////////////////

  // Execute(path)
  //   Executes a file or folder.
  } else if (command == L"Execute") {
    Execute(body);

  // URL(address)
  //   Opens a web page.
  //   lParam is an anime ID.
  } else if (command == L"URL") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = anime::db.Find(anime_id);
    if (anime_item) {
      std::wstring title = anime_item->GetTitle();
      ReplaceString(body, L"%title%", Url::Encode(title));
    }
    ExecuteLink(body);

  //////////////////////////////////////////////////////////////////////////////
  // UI

  // About()
  //   Shows about window.
  } else if (command == L"About") {
    ui::ShowDialog(ui::Dialog::About);

  // Info([anime_id])
  //   Shows anime information window.
  //   lParam is an anime ID.
  } else if (command == L"Info") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    ui::ShowDlgAnimeInfo(anime_id);

  // MainDialog()
  } else if (command == L"MainDialog") {
    ui::ShowDialog(ui::Dialog::Main);

  // Settings()
  //   Shows settings window.
  //   wParam is the initial section.
  //   lParam is the initial page.
  } else if (command == L"Settings") {
    ui::ShowDlgSettings(static_cast<int>(wParam), static_cast<int>(lParam));

  // SearchTorrents(source)
  //   Searches torrents from specified source URL.
  //   lParam is an anime ID.
  } else if (command == L"SearchTorrents") {
    int anime_id = static_cast<int>(lParam);
    if (body.empty())
      body = taiga::settings.GetTorrentDiscoverySearchUrl();
    ui::DlgTorrent.Search(body, anime_id);

  // ToggleSidebar()
  } else if (command == L"ToggleSidebar") {
    const bool hide_sidebar = !taiga::settings.GetAppOptionHideSidebar();
    taiga::settings.SetAppOptionHideSidebar(hide_sidebar);
    ui::DlgMain.treeview.Show(!hide_sidebar);
    ui::DlgMain.UpdateControlPositions();
    ui::Menus.UpdateView();

  // TorrentAddFilter()
  //   Shows add new filter window.
  //   wParam is a BOOL value that represents modal status.
  //   lParam is the handle of the parent window.
  } else if (command == L"TorrentAddFilter") {
    if (!ui::DlgFeedFilter.IsWindow()) {
      ui::DlgFeedFilter.Create(IDD_FEED_FILTER,
          reinterpret_cast<HWND>(lParam), wParam != FALSE);
    } else {
      ActivateWindow(ui::DlgFeedFilter.GetWindowHandle());
    }

  // ViewContent(page)
  //   Selects a page from sidebar.
  } else if (command == L"ViewContent") {
    int page = ToInt(body);
    ui::DlgMain.navigation.SetCurrentPage(page);

  //////////////////////////////////////////////////////////////////////////////
  // Library

  // AddToList(status)
  //   Adds an anime to list with given status.
  //   wParam is a BOOL value that defines lParam.
  //   lParam is an anime ID, or a pointer to a vector of anime IDs.
  } else if (command == L"AddToList") {
    const auto status = static_cast<anime::MyStatus>(ToInt(body));
    if (!wParam) {
      int anime_id = static_cast<int>(lParam);
      anime::db.AddToList(anime_id, status);
    } else {
      const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
      for (const auto& anime_id : anime_ids) {
        anime::db.AddToList(anime_id, status);
      }
    }

  // ClearHistory()
  //   Deletes all history items.
  } else if (command == L"ClearHistory") {
    if (ui::OnHistoryClear())
      library::history.Clear();

  // ClearQueue()
  //   Deletes or merges all queued updates.
  } else if (command == L"ClearQueue") {
    switch (ui::OnHistoryQueueClear()) {
      case IDYES:  // Delete
        library::queue.Clear();
        break;
      case IDNO:   // Merge
        library::queue.Merge();
        break;
    }

  //////////////////////////////////////////////////////////////////////////////
  // Tracker

  // AddFolder()
  //   Opens up a dialog to add new library folder.
  } else if (command == L"AddFolder") {
    std::wstring path;
    if (win::BrowseForFolder(ui::GetWindowHandle(ui::Dialog::Main),
                             L"Add a Library Folder", L"", path)) {
      auto library_folders = taiga::settings.GetLibraryFolders();
      library_folders.push_back(path);
      taiga::settings.SetLibraryFolders(library_folders);
      if (taiga::settings.GetLibraryWatchFolders())
        track::monitor.Enable();
      ui::ShowDlgSettings(ui::kSettingsSectionLibrary, ui::kSettingsPageLibraryFolders);
    }

  // ScanEpisodes(), ScanEpisodesAll()
  //   Checks episode availability.
  } else if (command == L"ScanEpisodes") {
    int anime_id = static_cast<int>(lParam);
    ScanAvailableEpisodes(false, anime_id, 0);
  } else if (command == L"ScanEpisodesAll") {
    ScanAvailableEpisodes(false);

  //////////////////////////////////////////////////////////////////////////////
  // Settings

  // ToggleRecognition()
  //   Enables or disables anime recognition.
  } else if (command == L"ToggleRecognition") {
    const bool enable_recognition = !taiga::settings.GetAppOptionEnableRecognition();
    taiga::settings.SetAppOptionEnableRecognition(enable_recognition);
    if (enable_recognition) {
      ui::ChangeStatusText(L"Automatic anime recognition is now enabled.");
      CurrentEpisode.Set(anime::ID_UNKNOWN);
    } else {
      ui::ChangeStatusText(L"Automatic anime recognition is now disabled.");
      auto anime_item = anime::db.Find(CurrentEpisode.anime_id);
      CurrentEpisode.Set(anime::ID_NOTINLIST);
      if (anime_item)
        EndWatching(*anime_item, CurrentEpisode);
    }

  // ToggleSharing()
  //   Enables or disables automatic sharing.
  } else if (command == L"ToggleSharing") {
    const bool enable_sharing = !taiga::settings.GetAppOptionEnableSharing();
    taiga::settings.SetAppOptionEnableSharing(enable_sharing);
    ui::Menus.UpdateTools();
    if (enable_sharing) {
      ui::ChangeStatusText(L"Automatic sharing is now enabled.");
    } else {
      ui::ChangeStatusText(L"Automatic sharing is now disabled.");
    }

  // ToggleSynchronization()
  //   Enables or disables automatic list synchronization.
  } else if (command == L"ToggleSynchronization") {
    const bool enable_sync = !taiga::settings.GetAppOptionEnableSync();
    taiga::settings.SetAppOptionEnableSync(enable_sync);
    ui::Menus.UpdateTools();
    if (enable_sync) {
      ui::ChangeStatusText(L"Automatic synchronization is now enabled.");
    } else {
      ui::ChangeStatusText(L"Automatic synchronization is now disabled.");
    }

  //////////////////////////////////////////////////////////////////////////////
  // Sharing

  // AnnounceToDiscord(force)
  //   Updates rich presence.
  } else if (command == L"AnnounceToDiscord") {
    taiga::announcer.Do(taiga::kAnnounceToDiscord, nullptr, ToBool(body));

  // AnnounceToHTTP(force)
  //   Sends an HTTP request.
  } else if (command == L"AnnounceToHTTP") {
    taiga::announcer.Do(taiga::kAnnounceToHttp, nullptr, ToBool(body));

  // AnnounceToMIRC(force)
  //   Sends message to specified channels in mIRC.
  } else if (command == L"AnnounceToMIRC") {
    taiga::announcer.Do(taiga::kAnnounceToMirc, nullptr, ToBool(body));

  //////////////////////////////////////////////////////////////////////////////

  // EditAll([anime_id])
  //   Shows a dialog to edit details of an anime.
  //   lParam is an anime ID.
  } else if (command == L"EditAll") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    ui::ShowDlgAnimeEdit(anime_id);

  // EditDateClear(value)
  //   Clears date started (0), date completed (1), or both (2).
  //   lParam is a pointer to a vector of anime IDs.
  } else if (command == L"EditDateClear") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    for (const auto& anime_id : anime_ids) {
      library::QueueItem queue_item;
      queue_item.anime_id = anime_id;
      int value = ToInt(body);
      if (value == 0 || value == 2)
        queue_item.date_start = Date();
      if (value == 1 || value == 2)
        queue_item.date_finish = Date();
      queue_item.mode = library::QueueItemMode::Update;
      library::queue.Add(queue_item);
    }

  // EditDateToStartedAiring
  //   Sets date started to date started airing.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (command == L"EditDateToStartedAiring") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    for (const auto& anime_id : anime_ids) {
      auto anime_item = anime::db.Find(anime_id);
      if (!anime_item || anime_item->GetMyDateStart() || !anime_item->GetDateStart())
        continue;
      library::QueueItem queue_item;
      queue_item.anime_id = anime_id;
      queue_item.date_start = anime_item->GetDateStart();
      queue_item.mode = library::QueueItemMode::Update;
      library::queue.Add(queue_item);
    }

  // EditDateToFinishedAiring
  //   Sets date completed to date finished airing.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (command == L"EditDateToFinishedAiring") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    for (const auto& anime_id : anime_ids) {
      auto anime_item = anime::db.Find(anime_id);
      if (!anime_item || anime_item->GetMyDateEnd() || !anime_item->GetDateEnd())
        continue;
      library::QueueItem queue_item;
      queue_item.anime_id = anime_id;
      queue_item.date_finish = anime_item->GetDateEnd();
      queue_item.mode = library::QueueItemMode::Update;
      library::queue.Add(queue_item);
    }

  // EditDateToLastUpdated
  //   Sets date completed to last updated.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (command == L"EditDateToLastUpdated") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    for (const auto& anime_id : anime_ids) {
      auto anime_item = anime::db.Find(anime_id);
      if (!anime_item || anime_item->GetMyDateEnd())
        continue;
      const auto last_updated = ToTime(anime_item->GetMyLastUpdated());
      if (!last_updated)
        continue;
      library::QueueItem queue_item;
      queue_item.anime_id = anime_id;
      queue_item.date_finish = GetDate(last_updated);
      queue_item.mode = library::QueueItemMode::Update;
      library::queue.Add(queue_item);
    }

  // EditDelete()
  //   Removes an anime from list.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (command == L"EditDelete") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    if (ui::OnLibraryEntriesEditDelete(anime_ids)) {
      for (const auto& anime_id : anime_ids) {
        library::QueueItem queue_item;
        queue_item.anime_id = anime_id;
        queue_item.mode = library::QueueItemMode::Delete;
        library::queue.Add(queue_item);
      }
    }

  // EditEpisode()
  //   Changes watched episode value of an anime.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (command == L"EditEpisode") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    int value = ui::OnLibraryEntriesEditEpisode(anime_ids);
    if (value > -1) {
      for (const auto& anime_id : anime_ids) {
        anime::ChangeEpisode(anime_id, value);
      }
    }

  // DecrementEpisode()
  //   lParam is an anime ID.
  } else if (command == L"DecrementEpisode") {
    int anime_id = static_cast<int>(lParam);
    anime::DecrementEpisode(anime_id);
  // IncrementEpisode()
  //   lParam is an anime ID.
  } else if (command == L"IncrementEpisode") {
    int anime_id = static_cast<int>(lParam);
    anime::IncrementEpisode(anime_id);

  // EditScore(value)
  //   Changes anime score.
  //   Value must be between 0-100 and different from current score.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (command == L"EditScore") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    for (const auto& anime_id : anime_ids) {
      library::QueueItem queue_item;
      queue_item.anime_id = anime_id;
      queue_item.score = ToInt(body);
      queue_item.mode = library::QueueItemMode::Update;
      library::queue.Add(queue_item);
    }

  // EditStatus(value)
  //   Changes anime status of user.
  //   Value must be 1, 2, 3, 4 or 5, and different from current status.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (command == L"EditStatus") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    for (const auto& anime_id : anime_ids) {
      library::QueueItem queue_item;
      queue_item.status = static_cast<anime::MyStatus>(ToInt(body));
      auto anime_item = anime::db.Find(anime_id);
      if (!anime_item)
        continue;
      switch (*queue_item.status) {
        case anime::MyStatus::Completed:
          queue_item.episode = anime_item->GetEpisodeCount();
          if (*queue_item.episode == 0)
            queue_item.episode.reset();
          if (!anime::IsValidDate(anime_item->GetMyDateStart()) &&
              anime_item->GetEpisodeCount() == 1)
              queue_item.date_start = GetDate();
          if (!anime::IsValidDate(anime_item->GetMyDateEnd()))
            queue_item.date_finish = GetDate();
          break;
      }
      queue_item.anime_id = anime_id;
      queue_item.mode = library::QueueItemMode::Update;
      library::queue.Add(queue_item);
    }

  // EditNotes(notes)
  //   Changes anime notes.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (command == L"EditNotes") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    std::wstring notes;
    if (ui::OnLibraryEntriesEditNotes(anime_ids, notes)) {
      for (const auto& anime_id : anime_ids) {
        library::QueueItem queue_item;
        queue_item.anime_id = anime_id;
        queue_item.notes = notes;
        queue_item.mode = library::QueueItemMode::Update;
        library::queue.Add(queue_item);
      }
    }

  //////////////////////////////////////////////////////////////////////////////

  // OpenFolder()
  //   Searches for anime folder and opens it.
  //   lParam is an anime ID.
  } else if (command == L"OpenFolder") {
    const int anime_id = static_cast<int>(lParam);
    const auto anime_item = anime::db.Find(anime_id);
    if (!anime_item || !anime_item->IsInList())
      return;
    if (!anime::ValidateFolder(*anime_item))
      ScanAvailableEpisodes(false, anime_item->GetId(), 0);
    const auto next_episode_path = anime_item->GetNextEpisodePath();
    if (!next_episode_path.empty()) {
      const auto anime_folder = anime_item->GetFolder();
      if (anime_folder.empty() || StartsWith(next_episode_path, anime_folder))
        if (OpenFolderAndSelectFile(next_episode_path))
          return;
    }
    if (anime_item->GetFolder().empty()) {
      if (ui::OnAnimeFolderNotFound()) {
        std::wstring default_path, path;
        const auto library_folders = taiga::settings.GetLibraryFolders();
        if (!library_folders.empty())
          default_path = library_folders.front();
        if (win::BrowseForFolder(ui::GetWindowHandle(ui::Dialog::Main),
                                 L"Select Anime Folder",
                                 default_path, path)) {
          anime_item->SetFolder(path);
          taiga::settings.Save();
        }
      }
    }
    if (!anime_item->GetFolder().empty()) {
      Execute(anime_item->GetFolder());
    }

  //////////////////////////////////////////////////////////////////////////////

  // PlayEpisode(value)
  //   Searches for an episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (command == L"PlayEpisode") {
    int number = ToInt(body);
    int anime_id = static_cast<int>(lParam);
    track::PlayEpisode(anime_id, number);

  // PlayLast()
  //   Searches for the last watched episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (command == L"PlayLast") {
    int anime_id = static_cast<int>(lParam);
    track::PlayLastEpisode(anime_id);

  // PlayNext([anime_id])
  //   Searches for the next episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (command == L"PlayNext") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    if (anime::IsValidId(anime_id)) {
      track::PlayNextEpisode(anime_id);
    } else {
      track::PlayNextEpisodeOfLastWatchedAnime();
    }

  // PlayRandom()
  //   Searches for a random episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (command == L"PlayRandom") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    track::PlayRandomEpisode(anime_id);

  // PlayRandomAnime()
  //   Searches for a random episode of a random anime and plays it.
  } else if (command == L"PlayRandomAnime") {
    track::PlayRandomAnime();

  // StartNewRewatch()
  //   Sets status to Currently Watching and plays first episode.
  } else if (command == L"StartNewRewatch") {
    int anime_id = static_cast<int>(lParam);
    anime::StartNewRewatch(anime_id);

  //////////////////////////////////////////////////////////////////////////////

  // Season_Load(file)
  //   Loads season data.
  } else if (command == L"Season_Load") {
    anime::season_db.Set(anime::Season(WstrToStr(body)));
    taiga::settings.SetAppSeasonsLastSeason(
        ui::TranslateSeason(anime::season_db.current_season));
    if (anime::season_db.items.empty()) {
      sync::GetSeason(anime::season_db.current_season);
    } else {
      ui::OnLibraryGetSeason();
    }

  // Season_GroupBy(group)
  //   Groups season data.
  } else if (command == L"Season_GroupBy") {
    taiga::settings.SetAppSeasonsGroupBy(ToInt(body));
    ui::DlgSeason.RefreshList();
    ui::DlgSeason.RefreshToolbar();

  // Season_SortBy(sort)
  //   Sorts season data.
  } else if (command == L"Season_SortBy") {
    taiga::settings.SetAppSeasonsSortBy(ToInt(body));
    ui::DlgSeason.RefreshList();
    ui::DlgSeason.RefreshToolbar();

  // Season_RefreshItemData()
  //   Refreshes an individual season item data.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (command == L"Season_RefreshItemData") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    ui::DlgSeason.RefreshData(anime_ids);

  // Season_ViewAs(mode)
  //   Changes view mode.
  } else if (command == L"Season_ViewAs") {
    ui::DlgSeason.SetViewMode(ToInt(body));
    ui::DlgSeason.RefreshList();
    ui::DlgSeason.RefreshToolbar();

  // Unknown
  } else {
    LOGW(L"Unknown command: {}", command);
  }
}

}  // namespace ui
