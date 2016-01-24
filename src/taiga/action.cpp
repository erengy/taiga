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

#include "base/log.h"
#include "base/process.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/discover.h"
#include "library/history.h"
#include "sync/hummingbird_util.h"
#include "sync/myanimelist_util.h"
#include "sync/sync.h"
#include "taiga/announce.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "track/monitor.h"
#include "track/recognition.h"
#include "track/search.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_search.h"
#include "ui/dlg/dlg_season.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_torrent.h"
#include "ui/dlg/dlg_feed_filter.h"
#include "ui/dialog.h"
#include "ui/menu.h"
#include "ui/ui.h"
#include "win/win_commondialog.h"

void ExecuteAction(std::wstring action, WPARAM wParam, LPARAM lParam) {
  LOG(LevelDebug, action);

  std::wstring body;
  size_t pos = action.find('(');
  if (pos != action.npos) {
    body = action.substr(pos + 1, action.find_last_of(')') - (pos + 1));
    action.resize(pos);
  }
  Trim(body);
  Trim(action);
  if (action.empty())
    return;

  //////////////////////////////////////////////////////////////////////////////
  // Taiga

  // CheckUpdates()
  //   Checks for a new version of the program.
  if (action == L"CheckUpdates") {
    ui::ShowDialog(ui::kDialogUpdate);

  // Exit(), Quit()
  //   Exits from Taiga.
  } else if (action == L"Exit" || action == L"Quit") {
    ui::DlgMain.Destroy();

  //////////////////////////////////////////////////////////////////////////////
  // Services

  // Synchronize()
  //   Synchronizes local and remote lists.
  } else if (action == L"Synchronize") {
    sync::Synchronize();

  // SearchAnime()
  //   wParam is a BOOL value that defines local search.
  } else if (action == L"SearchAnime") {
    if (body.empty())
      return;
    bool local_search = wParam != FALSE;
    if (!local_search) {
      auto service = taiga::GetCurrentService();
      if (service->RequestNeedsAuthentication(sync::kSearchTitle)) {
        if (taiga::GetCurrentUsername().empty() ||
            taiga::GetCurrentPassword().empty()) {
          ui::OnSettingsAccountEmpty();
          return;
        }
      }
    }
    ui::DlgMain.navigation.SetCurrentPage(ui::kSidebarItemSearch);
    ui::DlgMain.edit.SetText(body);
    ui::DlgSearch.Search(body, local_search);

  // ViewAnimePage
  //   Opens up anime page on the active service.
  //   lParam is an anime ID.
  } else if (action == L"ViewAnimePage") {
    int anime_id = static_cast<int>(lParam);
    switch (taiga::GetCurrentServiceId()) {
      case sync::kMyAnimeList:
        sync::myanimelist::ViewAnimePage(anime_id);
        break;
      case sync::kHummingbird:
        sync::hummingbird::ViewAnimePage(anime_id);
        break;
    }

  // ViewUpcomingAnime
  //   Opens up upcoming anime page on MAL.
  } else if (action == L"ViewUpcomingAnime") {
    switch (taiga::GetCurrentServiceId()) {
      case sync::kMyAnimeList:
        sync::myanimelist::ViewUpcomingAnime();
        break;
      case sync::kHummingbird:
        sync::hummingbird::ViewUpcomingAnime();
        break;
    }

  // MalViewPanel()
  // MalViewProfile()
  // MalViewHistory()
  //   Opens up MyAnimeList user pages.
  } else if (action == L"MalViewPanel") {
    sync::myanimelist::ViewPanel();
  } else if (action == L"MalViewProfile") {
    sync::myanimelist::ViewProfile();
  } else if (action == L"MalViewHistory") {
    sync::myanimelist::ViewHistory();

  // HummingbirdViewProfile()
  // HummingbirdViewDashboard()
  // HummingbirdViewRecommendations()
  //   Opens up Hummingbird user pages.
  } else if (action == L"HummingbirdViewProfile") {
    sync::hummingbird::ViewProfile();
  } else if (action == L"HummingbirdViewDashboard") {
    sync::hummingbird::ViewDashboard();
  } else if (action == L"HummingbirdViewRecommendations") {
    sync::hummingbird::ViewRecommendations();

  //////////////////////////////////////////////////////////////////////////////

  // Execute(path)
  //   Executes a file or folder.
  } else if (action == L"Execute") {
    Execute(body);

  // URL(address)
  //   Opens a web page.
  //   lParam is an anime ID.
  } else if (action == L"URL") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item) {
      std::wstring title = anime_item->GetTitle();
      ReplaceString(body, L"%title%", EncodeUrl(title));
    }
    ExecuteLink(body);

  //////////////////////////////////////////////////////////////////////////////
  // UI

  // About()
  //   Shows about window.
  } else if (action == L"About") {
    ui::ShowDialog(ui::kDialogAbout);

  // Info([anime_id])
  //   Shows anime information window.
  //   lParam is an anime ID.
  } else if (action == L"Info") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    ui::ShowDlgAnimeInfo(anime_id);

  // MainDialog()
  } else if (action == L"MainDialog") {
    ui::ShowDialog(ui::kDialogMain);

  // Settings()
  //   Shows settings window.
  //   wParam is the initial section.
  //   lParam is the initial page.
  } else if (action == L"Settings") {
    ui::ShowDlgSettings(wParam, lParam);

  // SearchTorrents(source)
  //   Searches torrents from specified source URL.
  //   lParam is an anime ID.
  } else if (action == L"SearchTorrents") {
    int anime_id = static_cast<int>(lParam);
    if (body.empty())
      body = Settings[taiga::kTorrent_Discovery_SearchUrl];
    ui::DlgTorrent.Search(body, anime_id);

  // ToggleSidebar()
  } else if (action == L"ToggleSidebar") {
    bool hide_sidebar = Settings.Toggle(taiga::kApp_Option_HideSidebar);
    ui::DlgMain.treeview.Show(!hide_sidebar);
    ui::DlgMain.UpdateControlPositions();
    ui::Menus.UpdateView();

  // TorrentAddFilter()
  //   Shows add new filter window.
  //   wParam is a BOOL value that represents modal status.
  //   lParam is the handle of the parent window.
  } else if (action == L"TorrentAddFilter") {
    if (!ui::DlgFeedFilter.IsWindow()) {
      ui::DlgFeedFilter.Create(IDD_FEED_FILTER,
          reinterpret_cast<HWND>(lParam), wParam != FALSE);
    } else {
      ActivateWindow(ui::DlgFeedFilter.GetWindowHandle());
    }

  // ViewContent(page)
  //   Selects a page from sidebar.
  } else if (action == L"ViewContent") {
    int page = ToInt(body);
    ui::DlgMain.navigation.SetCurrentPage(page);

  //////////////////////////////////////////////////////////////////////////////
  // Library

  // AddToList(status)
  //   Adds an anime to list with given status.
  //   wParam is a BOOL value that defines lParam.
  //   lParam is an anime ID, or a pointer to a vector of anime IDs.
  } else if (action == L"AddToList") {
    int status = ToInt(body);
    if (!wParam) {
      int anime_id = static_cast<int>(lParam);
      AnimeDatabase.AddToList(anime_id, status);
    } else {
      const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
      for (const auto& anime_id : anime_ids) {
        AnimeDatabase.AddToList(anime_id, status);
      }
    }

  // ClearHistory()
  //   Deletes all history items.
  } else if (action == L"ClearHistory") {
    if (ui::OnHistoryClear())
      History.Clear();

  //////////////////////////////////////////////////////////////////////////////
  // Tracker

  // AddFolder()
  //   Opens up a dialog to add new library folder.
  } else if (action == L"AddFolder") {
    std::wstring path;
    if (win::BrowseForFolder(ui::GetWindowHandle(ui::kDialogMain),
                             L"Add a Library Folder", L"", path)) {
      Settings.library_folders.push_back(path);
      if (Settings.GetBool(taiga::kLibrary_WatchFolders))
        FolderMonitor.Enable();
      ui::ShowDlgSettings(ui::kSettingsSectionLibrary, ui::kSettingsPageLibraryFolders);
    }

  // ScanEpisodes(), ScanEpisodesAll()
  //   Checks episode availability.
  } else if (action == L"ScanEpisodes") {
    int anime_id = static_cast<int>(lParam);
    ScanAvailableEpisodes(false, anime_id, 0);
  } else if (action == L"ScanEpisodesAll") {
    ScanAvailableEpisodes(false);

  //////////////////////////////////////////////////////////////////////////////
  // Settings

  // ToggleRecognition()
  //   Enables or disables anime recognition.
  } else if (action == L"ToggleRecognition") {
    bool enable_recognition = Settings.Toggle(taiga::kApp_Option_EnableRecognition);
    if (enable_recognition) {
      ui::ChangeStatusText(L"Automatic anime recognition is now enabled.");
      CurrentEpisode.Set(anime::ID_UNKNOWN);
    } else {
      ui::ChangeStatusText(L"Automatic anime recognition is now disabled.");
      auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);
      CurrentEpisode.Set(anime::ID_NOTINLIST);
      if (anime_item)
        EndWatching(*anime_item, CurrentEpisode);
    }

  // ToggleSharing()
  //   Enables or disables automatic sharing.
  } else if (action == L"ToggleSharing") {
    bool enable_sharing = Settings.Toggle(taiga::kApp_Option_EnableSharing);
    ui::Menus.UpdateTools();
    if (enable_sharing) {
      ui::ChangeStatusText(L"Automatic sharing is now enabled.");
    } else {
      ui::ChangeStatusText(L"Automatic sharing is now disabled.");
    }

  // ToggleSynchronization()
  //   Enables or disables automatic list synchronization.
  } else if (action == L"ToggleSynchronization") {
    bool enable_sync = Settings.Toggle(taiga::kApp_Option_EnableSync);
    ui::Menus.UpdateTools();
    if (enable_sync) {
      ui::ChangeStatusText(L"Automatic synchronization is now enabled.");
    } else {
      ui::ChangeStatusText(L"Automatic synchronization is now disabled.");
    }

  //////////////////////////////////////////////////////////////////////////////
  // Sharing

  // AnnounceToHTTP(force)
  //   Sends an HTTP request.
  } else if (action == L"AnnounceToHTTP") {
    Announcer.Do(taiga::kAnnounceToHttp, nullptr, body == L"true");

  // AnnounceToMIRC(force)
  //   Sends message to specified channels in mIRC.
  } else if (action == L"AnnounceToMIRC") {
    Announcer.Do(taiga::kAnnounceToMirc, nullptr, body == L"true");

  // AnnounceToSkype(force)
  //   Changes Skype mood text.
  //   Requires authorization.
  } else if (action == L"AnnounceToSkype") {
    Announcer.Do(taiga::kAnnounceToSkype, nullptr, body == L"true");

  // AnnounceToTwitter(force)
  //   Changes Twitter status.
  } else if (action == L"AnnounceToTwitter") {
    Announcer.Do(taiga::kAnnounceToTwitter, nullptr, body == L"true");

  //////////////////////////////////////////////////////////////////////////////

  // EditAll([anime_id])
  //   Shows a dialog to edit details of an anime.
  //   lParam is an anime ID.
  } else if (action == L"EditAll") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    ui::ShowDlgAnimeEdit(anime_id);

  // EditDateClear(value)
  //   Clears date started (0), date completed (1), or both (2).
  //   lParam is a pointer to a vector of anime IDs.
  } else if (action == L"EditDateClear") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    for (const auto& anime_id : anime_ids) {
      HistoryItem history_item;
      history_item.anime_id = anime_id;
      int value = ToInt(body);
      if (value == 0 || value == 2)
        history_item.date_start = Date();
      if (value == 1 || value == 2)
        history_item.date_finish = Date();
      history_item.mode = taiga::kHttpServiceUpdateLibraryEntry;
      History.queue.Add(history_item);
    }

  // EditDelete()
  //   Removes an anime from list.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (action == L"EditDelete") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    if (ui::OnLibraryEntriesEditDelete(anime_ids)) {
      for (const auto& anime_id : anime_ids) {
        HistoryItem history_item;
        history_item.anime_id = anime_id;
        history_item.mode = taiga::kHttpServiceDeleteLibraryEntry;
        History.queue.Add(history_item);
      }
    }

  // EditEpisode()
  //   Changes watched episode value of an anime.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (action == L"EditEpisode") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    int value = ui::OnLibraryEntriesEditEpisode(anime_ids);
    if (value > -1) {
      for (const auto& anime_id : anime_ids) {
        anime::ChangeEpisode(anime_id, value);
      }
    }

  // DecrementEpisode()
  //   lParam is an anime ID.
  } else if (action == L"DecrementEpisode") {
    int anime_id = static_cast<int>(lParam);
    anime::DecrementEpisode(anime_id);
  // IncrementEpisode()
  //   lParam is an anime ID.
  } else if (action == L"IncrementEpisode") {
    int anime_id = static_cast<int>(lParam);
    anime::IncrementEpisode(anime_id);

  // EditScore(value)
  //   Changes anime score.
  //   Value must be between 0-10 and different from current score.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (action == L"EditScore") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    for (const auto& anime_id : anime_ids) {
      HistoryItem history_item;
      history_item.anime_id = anime_id;
      history_item.score = ToInt(body);
      history_item.mode = taiga::kHttpServiceUpdateLibraryEntry;
      History.queue.Add(history_item);
    }

  // EditStatus(value)
  //   Changes anime status of user.
  //   Value must be 1, 2, 3, 4 or 5, and different from current status.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (action == L"EditStatus") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    for (const auto& anime_id : anime_ids) {
      HistoryItem history_item;
      history_item.status = ToInt(body);
      auto anime_item = AnimeDatabase.FindItem(anime_id);
      if (!anime_item)
        continue;
      switch (*history_item.status) {
        case anime::kCompleted:
          history_item.episode = anime_item->GetEpisodeCount();
          if (*history_item.episode == 0)
            history_item.episode.Reset();
          if (!anime::IsValidDate(anime_item->GetMyDateStart()) &&
              anime_item->GetEpisodeCount() == 1)
              history_item.date_start = GetDate();
          if (!anime::IsValidDate(anime_item->GetMyDateEnd()))
            history_item.date_finish = GetDate();
          break;
      }
      history_item.anime_id = anime_id;
      history_item.mode = taiga::kHttpServiceUpdateLibraryEntry;
      History.queue.Add(history_item);
    }

  // EditTags(tags)
  //   Changes anime tags.
  //   Tags must be separated by a comma.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (action == L"EditTags") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    std::wstring tags;
    if (ui::OnLibraryEntriesEditTags(anime_ids, tags)) {
      for (const auto& anime_id : anime_ids) {
        HistoryItem history_item;
        history_item.anime_id = anime_id;
        history_item.tags = tags;
        history_item.mode = taiga::kHttpServiceUpdateLibraryEntry;
        History.queue.Add(history_item);
      }
    }

  //////////////////////////////////////////////////////////////////////////////

  // OpenFolder()
  //   Searches for anime folder and opens it.
  //   lParam is an anime ID.
  } else if (action == L"OpenFolder") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (!anime_item || !anime_item->IsInList())
      return;
    if (!anime::ValidateFolder(*anime_item))
      ScanAvailableEpisodes(false, anime_item->GetId(), 0);
    if (anime_item->GetFolder().empty()) {
      if (ui::OnAnimeFolderNotFound()) {
        std::wstring default_path, path;
        if (!Settings.library_folders.empty())
          default_path = Settings.library_folders.front();
        if (win::BrowseForFolder(ui::GetWindowHandle(ui::kDialogMain),
                                 L"Select Anime Folder",
                                 default_path, path)) {
          anime_item->SetFolder(path);
          Settings.Save();
        }
      }
    }
    ui::ClearStatusText();
    if (!anime_item->GetFolder().empty()) {
      Execute(anime_item->GetFolder());
    }

  //////////////////////////////////////////////////////////////////////////////

  // PlayEpisode(value)
  //   Searches for an episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (action == L"PlayEpisode") {
    int number = ToInt(body);
    int anime_id = static_cast<int>(lParam);
    anime::PlayEpisode(anime_id, number);

  // PlayLast()
  //   Searches for the last watched episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (action == L"PlayLast") {
    int anime_id = static_cast<int>(lParam);
    anime::PlayLastEpisode(anime_id);

  // PlayNext([anime_id])
  //   Searches for the next episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (action == L"PlayNext") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    if (anime::IsValidId(anime_id)) {
      anime::PlayNextEpisode(anime_id);
    } else {
      anime::PlayNextEpisodeOfLastWatchedAnime();
    }

  // PlayRandom()
  //   Searches for a random episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (action == L"PlayRandom") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    anime::PlayRandomEpisode(anime_id);

  // PlayRandomAnime()
  //   Searches for a random episode of a random anime and plays it.
  } else if (action == L"PlayRandomAnime") {
    anime::PlayRandomAnime();

  //////////////////////////////////////////////////////////////////////////////

  // Season_Load(file)
  //   Loads season data.
  } else if (action == L"Season_Load") {
    if (SeasonDatabase.LoadSeason(body)) {
      Settings.Set(taiga::kApp_Seasons_LastSeason,
                   SeasonDatabase.current_season.GetString());
      SeasonDatabase.Review();
      ui::OnSeasonLoad(SeasonDatabase.IsRefreshRequired());
    }

  // Season_GroupBy(group)
  //   Groups season data.
  } else if (action == L"Season_GroupBy") {
    Settings.Set(taiga::kApp_Seasons_GroupBy, ToInt(body));
    ui::DlgSeason.RefreshList();
    ui::DlgSeason.RefreshToolbar();

  // Season_SortBy(sort)
  //   Sorts season data.
  } else if (action == L"Season_SortBy") {
    Settings.Set(taiga::kApp_Seasons_SortBy, ToInt(body));
    ui::DlgSeason.RefreshList();
    ui::DlgSeason.RefreshToolbar();

  // Season_RefreshItemData()
  //   Refreshes an individual season item data.
  //   lParam is a pointer to a vector of anime IDs.
  } else if (action == L"Season_RefreshItemData") {
    const auto& anime_ids = *reinterpret_cast<std::vector<int>*>(lParam);
    for (const auto& anime_id : anime_ids) {
      ui::DlgSeason.RefreshData(anime_id);
    }

  // Season_ViewAs(mode)
  //   Changes view mode.
  } else if (action == L"Season_ViewAs") {
    ui::DlgSeason.SetViewMode(ToInt(body));
    ui::DlgSeason.RefreshList();
    ui::DlgSeason.RefreshToolbar();

  // Unknown
  } else {
    LOG(LevelWarning, L"Unknown action: " + action);
  }
}