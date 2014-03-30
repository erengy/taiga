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

#include "base/file.h"
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
  } else if (action == L"SearchAnime") {
    if (body.empty())
      return;
    auto service = taiga::GetCurrentService();
    if (service->RequestNeedsAuthentication(sync::kSearchTitle)) {
      if (taiga::GetCurrentUsername().empty() ||
          taiga::GetCurrentPassword().empty()) {
        ui::OnSettingsAccountEmpty();
        return;
      }
    }
    ui::DlgMain.navigation.SetCurrentPage(ui::kSidebarItemSearch);
    ui::DlgMain.edit.SetText(body);
    ui::DlgSearch.Search(body);

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
      EraseChars(title, L"_!?.,:;~+");
      Erase(title, L" -");
      Replace(body, L"%title%", title);
    }
    ExecuteLink(body);

  //////////////////////////////////////////////////////////////////////////////
  // UI

  // About()
  //   Shows about window.
  } else if (action == L"About") {
    ui::ShowDialog(ui::kDialogAbout);

  // Info()
  //   Shows anime information window.
  //   lParam is an anime ID.
  } else if (action == L"Info") {
    int anime_id = static_cast<int>(lParam);
    ui::ShowDlgAnimeInfo(anime_id);

  // MainDialog()
  } else if (action == L"MainDialog") {
    ui::ShowDialog(ui::kDialogMain);

  // RecognitionTest()
  //   Shows recognition test window.
  } else if (action == L"RecognitionTest") {
    ui::ShowDialog(ui::kDialogTestRecognition);

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
    ui::DlgTorrent.Search(body, anime_id);

  // ShowSidebar()
  } else if (action == L"ShowSidebar") {
    bool hide_sidebar = !Settings.GetBool(taiga::kApp_Option_HideSidebar);
    Settings.Set(taiga::kApp_Option_HideSidebar, hide_sidebar);
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
  
  // AddToListAs(status)
  //   Adds new anime to list with given status.
  //   lParam is an anime ID.
  } else if (action == L"AddToListAs") {
    int status = ToInt(body);
    int anime_id = static_cast<int>(lParam);
    AnimeDatabase.AddToList(anime_id, status);

  //////////////////////////////////////////////////////////////////////////////
  // Tracker

  // AddFolder()
  //   Opens up a dialog to add new root folder.
  } else if (action == L"AddFolder") {
    std::wstring path;
    if (BrowseForFolder(ui::GetWindowHandle(ui::kDialogMain), L"Please select a folder:", L"", path)) {
      Settings.root_folders.push_back(path);
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
    bool enable_recognition = !Settings.GetBool(taiga::kApp_Option_EnableRecognition);
    Settings.Set(taiga::kApp_Option_EnableRecognition, enable_recognition);
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
    bool enable_sharing = !Settings.GetBool(taiga::kApp_Option_EnableSharing);
    Settings.Set(taiga::kApp_Option_EnableSharing, enable_sharing);
    ui::Menus.UpdateTools();
    if (enable_sharing) {
      ui::ChangeStatusText(L"Automatic sharing is now enabled.");
    } else {
      ui::ChangeStatusText(L"Automatic sharing is now disabled.");
    }

  // ToggleSynchronization()
  //   Enables or disables automatic list synchronization.
  } else if (action == L"ToggleSynchronization") {
    bool enable_sync = !Settings.GetBool(taiga::kApp_Option_EnableSync);
    Settings.Set(taiga::kApp_Option_EnableSync, enable_sync);
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
  
  // AnnounceToMessenger(force)
  //   Changes MSN Messenger status text.
  } else if (action == L"AnnounceToMessenger") {
    Announcer.Do(taiga::kAnnounceToMessenger, nullptr, body == L"true");
  
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

  // EditDelete()
  //   Removes an anime from list.
  //   lParam is an anime ID.
  } else if (action == L"EditDelete") {
    int anime_id = static_cast<int>(lParam);
    if (ui::OnLibraryEntryEditDelete(anime_id)) {
      HistoryItem history_item;
      history_item.anime_id = anime_id;
      history_item.mode = taiga::kHttpServiceDeleteLibraryEntry;
      History.queue.Add(history_item);
    }

  // EditEpisode(value)
  //   Changes watched episode value of an anime.
  //   Value is optional.
  //   lParam is an anime ID.
  } else if (action == L"EditEpisode") {
    int anime_id = static_cast<int>(lParam);
    int value = body.empty() ?
        ui::OnLibraryEntryEditEpisode(anime_id) : ToInt(body);
    anime::ChangeEpisode(anime_id, value);
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
  //   lParam is an anime ID.
  } else if (action == L"EditScore") {
    int anime_id = static_cast<int>(lParam);
    HistoryItem history_item;
    history_item.anime_id = anime_id;
    history_item.score = ToInt(body);
    history_item.mode = taiga::kHttpServiceUpdateLibraryEntry;
    History.queue.Add(history_item);

  // EditStatus(value)
  //   Changes anime status of user.
  //   Value must be 1, 2, 3, 4 or 6, and different from current status.
  //   lParam is an anime ID.
  } else if (action == L"EditStatus") {
    HistoryItem history_item;
    history_item.status = ToInt(body);
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
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

  // EditTags(tags)
  //   Changes anime tags.
  //   Tags must be separated by a comma.
  //   lParam is an anime ID.
  } else if (action == L"EditTags") {
    int anime_id = static_cast<int>(lParam);
    std::wstring tags;
    if (ui::OnLibraryEntryEditTags(anime_id, tags)) {
      HistoryItem history_item;
      history_item.anime_id = anime_id;
      history_item.tags = tags;
      history_item.mode = taiga::kHttpServiceUpdateLibraryEntry;
      History.queue.Add(history_item);
    }

  // EditTitles(titles)
  //   Changes alternative titles of an anime.
  //   Titles must be separated by "; ".
  //   lParam is an anime ID.
  } else if (action == L"EditTitles") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    std::wstring titles;
    if (ui::OnLibraryEntryEditTitles(anime_id, titles)) {
      anime_item->SetUserSynonyms(titles);
      Meow.UpdateCleanTitles(anime_id);
      Settings.Save();
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
    if (anime_item->GetFolder().empty()) {
      ScanAvailableEpisodes(false, anime_item->GetId(), 0);
    }
    if (anime_item->GetFolder().empty()) {
      if (ui::OnAnimeFolderNotFound()) {
        std::wstring default_path, path;
        if (!Settings.root_folders.empty())
          default_path = Settings.root_folders.front();
        if (BrowseForFolder(ui::GetWindowHandle(ui::kDialogMain),
                            L"Choose an anime folder", default_path, path)) {
          anime_item->SetFolder(path);
          Settings.Save();
        }
      }
    }
    ui::ClearStatusText();
    if (!anime_item->GetFolder().empty()) {
      Execute(anime_item->GetFolder());
    }

  // SetFolder()
  //   Lets user set an anime folder.
  //   lParam is an anime ID.
  } else if (action == L"SetFolder") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    std::wstring path, title = L"Anime title: " + anime_item->GetTitle();
    if (BrowseForFolder(ui::GetWindowHandle(ui::kDialogMain), title.c_str(), L"", path)) {
      anime_item->SetFolder(path);
      Settings.Save();
      ScanAvailableEpisodesQuick(anime_item->GetId());
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
    anime::PlayNextEpisode(anime_id);

  // PlayRandom()
  //   Searches for a random episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (action == L"PlayRandom") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item)
      anime::PlayRandomEpisode(*anime_item);

  // PlayRandomAnime()
  //   Searches for a random episode of a random anime and plays it.
  } else if (action == L"PlayRandomAnime") {
    anime::PlayRandomAnime();

  //////////////////////////////////////////////////////////////////////////////

  // Season_Load(file)
  //   Loads season data.
  } else if (action == L"Season_Load") {
    if (SeasonDatabase.Load(body)) {
      SeasonDatabase.Review();
      ui::DlgSeason.RefreshList();
      ui::DlgSeason.RefreshStatus();
      ui::DlgSeason.RefreshToolbar();
      if (SeasonDatabase.IsRefreshRequired())
        if (ui::OnSeasonRefreshRequired())
          ui::DlgSeason.RefreshData();
    }

  // Season_GroupBy(group)
  //   Groups season data.
  } else if (action == L"Season_GroupBy") {
    ui::DlgSeason.group_by = ToInt(body);
    ui::DlgSeason.RefreshList();
    ui::DlgSeason.RefreshToolbar();

  // Season_SortBy(sort)
  //   Sorts season data.
  } else if (action == L"Season_SortBy") {
    ui::DlgSeason.sort_by = ToInt(body);
    ui::DlgSeason.RefreshList();
    ui::DlgSeason.RefreshToolbar();

  // Season_RefreshItemData()
  //   Refreshes an individual season item data.
  } else if (action == L"Season_RefreshItemData") {
    ui::DlgSeason.RefreshData(static_cast<int>(lParam));

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