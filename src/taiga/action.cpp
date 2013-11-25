/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "base/std.h"

#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_filter.h"
#include "sync/announce.h"
#include "base/common.h"
#include "track/feed.h"
#include "base/foreach.h"
#include "library/history.h"
#include "http.h"
#include "base/logger.h"
#include "track/monitor.h"
#include "sync/myanimelist.h"
#include "base/process.h"
#include "track/recognition.h"
#include "resource.h"
#include "settings.h"
#include "stats.h"
#include "base/string.h"
#include "taiga.h"
#include "ui/menu.h"
#include "ui/theme.h"

#include "ui/dlg/dlg_about.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_anime_info_page.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_history.h"
#include "ui/dlg/dlg_input.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_search.h"
#include "ui/dlg/dlg_season.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_test_recognition.h"
#include "ui/dlg/dlg_torrent.h"
#include "ui/dlg/dlg_feed_filter.h"
#include "ui/dlg/dlg_update.h"

#include "win/win_taskbar.h"
#include "win/win_taskdialog.h"

// =============================================================================

void ExecuteAction(wstring action, WPARAM wParam, LPARAM lParam) {
  LOG(LevelDebug, action);
  
  wstring body;
  size_t pos = action.find('(');
  if (pos != action.npos) {
    body = action.substr(pos + 1, action.find_last_of(')') - (pos + 1));
    action.resize(pos);
  }
  Trim(body);
  Trim(action);
  if (action.empty()) return;

  // ===========================================================================

  // Synchronize()
  //   Synchronizes local and remote lists.
  if (action == L"Synchronize") {
#ifdef _DEBUG
    // Retrieve list
    MainDialog.ChangeStatus(L"Downloading anime list...");
    bool result = mal::GetList();
    MainDialog.EnableInput(!result);
    if (!result) MainDialog.ChangeStatus();
#else
    if (!Taiga.logged_in) {
      // Log in
      MainDialog.ChangeStatus(L"Logging in...");
      bool result = mal::Login();
      MainDialog.EnableInput(!result);
      if (!result) MainDialog.ChangeStatus();
    } else {
      if (History.queue.GetItemCount() > 0) {
        // Update items in queue
        History.queue.Check(false);
      } else {
        // Retrieve list
        MainDialog.ChangeStatus(L"Synchronizing anime list...");
        bool result = mal::GetList();
        MainDialog.EnableInput(!result);
        if (!result) MainDialog.ChangeStatus();
      }
    }
#endif

  // ===========================================================================

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
      wstring title = anime_item->GetTitle();
      EraseChars(title, L"_!?.,:;~+");
      Erase(title, L" -");
      Replace(body, L"%title%", title);
    }
    ExecuteLink(body);

  // ===========================================================================

  // About()
  //   Shows about window.
  } else if (action == L"About") {
    if (!AboutDialog.IsWindow()) {
      AboutDialog.Create(IDD_ABOUT, g_hMain, true);
    } else {
      ActivateWindow(AboutDialog.GetWindowHandle());
    }

  // CheckUpdates()
  //   Checks for a new version of the program.
  } else if (action == L"CheckUpdates") {
    if (!UpdateDialog.IsWindow()) {
      UpdateDialog.Create(IDD_UPDATE, g_hMain, true);
    } else {
      ActivateWindow(UpdateDialog.GetWindowHandle());
    }

  // Exit(), Quit()
  //   Exits from Taiga.
  } else if (action == L"Exit" || action == L"Quit") {
    MainDialog.Destroy();

  // Info()
  //   Shows anime information window.
  //   lParam is an anime ID.
  } else if (action == L"Info") {
    int anime_id = static_cast<int>(lParam);
    AnimeDialog.SetCurrentId(anime_id);
    AnimeDialog.SetCurrentPage(INFOPAGE_SERIESINFO);
    if (!AnimeDialog.IsWindow()) {
      AnimeDialog.Create(IDD_ANIME_INFO, g_hMain, false);
    } else {
      ActivateWindow(AnimeDialog.GetWindowHandle());
    }

  // MainDialog()
  } else if (action == L"MainDialog") {
    if (!MainDialog.IsWindow()) {
      MainDialog.Create(IDD_MAIN, NULL, false);
    } else {
      ActivateWindow(MainDialog.GetWindowHandle());
    }

  // RecognitionTest()
  //   Shows recognition test window.
  } else if (action == L"RecognitionTest") {
    if (!RecognitionTest.IsWindow()) {
      RecognitionTest.Create(IDD_TEST_RECOGNITION, NULL, false);
    } else {
      ActivateWindow(RecognitionTest.GetWindowHandle());
    }

  // Settings()
  //   Shows settings window.
  //   wParam is the initial section.
  //   lParam is the initial page.
  } else if (action == L"Settings") {
    if (wParam > 0)
      SettingsDialog.SetCurrentSection(wParam);
    if (lParam > 0)
      SettingsDialog.SetCurrentPage(lParam);
    if (!SettingsDialog.IsWindow()) {
      SettingsDialog.Create(IDD_SETTINGS, g_hMain, true);
    } else {
      ActivateWindow(SettingsDialog.GetWindowHandle());
    }
  
  // SearchAnime()
  } else if (action == L"SearchAnime") {
    if (body.empty()) return;
    if (Settings.Account.MAL.user.empty() || Settings.Account.MAL.password.empty()) {
      win::TaskDialog dlg(APP_TITLE, TD_ICON_INFORMATION);
      dlg.SetMainInstruction(L"Would you like to set your account information first?");
      dlg.SetContent(L"Anime search requires authentication, which means, "
        L"you need to enter a valid username and password to search MyAnimeList.");
      dlg.AddButton(L"Yes", IDYES);
      dlg.AddButton(L"No", IDNO);
      dlg.Show(g_hMain);
      if (dlg.GetSelectedButtonID() == IDYES)
        ExecuteAction(L"Settings", SECTION_SERVICES, PAGE_SERVICES_MAL);
      return;
    }
    MainDialog.navigation.SetCurrentPage(SIDEBAR_ITEM_SEARCH);
    MainDialog.edit.SetText(body);
    SearchDialog.Search(body);

  // SearchTorrents(source)
  //   Searches torrents from specified source URL.
  //   lParam is an anime ID.
  } else if (action == L"SearchTorrents") {
    int anime_id = static_cast<int>(lParam);
    TorrentDialog.Search(body, anime_id);

  // ShowSidebar()
  } else if (action == L"ShowSidebar") {
    Settings.Program.General.hide_sidebar = !Settings.Program.General.hide_sidebar;
    MainDialog.treeview.Show(!Settings.Program.General.hide_sidebar);
    MainDialog.UpdateControlPositions();
    UpdateViewMenu();

  // TorrentAddFilter()
  //   Shows add new filter window.
  //   wParam is a BOOL value that represents modal status.
  //   lParam is the handle of the parent window.
  } else if (action == L"TorrentAddFilter") {
    if (!FeedFilterDialog.IsWindow()) {
      FeedFilterDialog.Create(IDD_FEED_FILTER, 
        reinterpret_cast<HWND>(lParam), wParam != FALSE);
    } else {
      ActivateWindow(FeedFilterDialog.GetWindowHandle());
    }

  // ViewContent(page)
  //   Selects a page from sidebar.
  } else if (action == L"ViewContent") {
    int page = ToInt(body);
    MainDialog.navigation.SetCurrentPage(page);

  // ===========================================================================
  
  // AddToListAs(status)
  //   Adds new anime to list with given status.
  //   lParam is an anime ID.
  } else if (action == L"AddToListAs") {
    int status = ToInt(body);
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    // Add item to list
    anime_item->AddtoUserList();
    AnimeDatabase.user.IncreaseItemCount(status, true);
    AnimeDatabase.SaveList(anime_id, L"", L"", anime::ADD_ANIME);
    // Add item to event queue
    EventItem event_item;
    event_item.anime_id = anime_id;
    event_item.status = status;
    if (status == mal::MYSTATUS_COMPLETED) {
      event_item.episode = anime_item->GetEpisodeCount();
      event_item.date_finish = mal::TranslateDateForApi(GetDate());
    }
    event_item.mode = HTTP_MAL_AnimeAdd;
    History.queue.Add(event_item);
    // Refresh
    AnimeListDialog.RefreshList(status);
    AnimeListDialog.RefreshTabs(status);
    SearchDialog.RefreshList();
    if (AnimeDialog.GetCurrentId() == anime_id)
      AnimeDialog.Refresh();
    if (NowPlayingDialog.GetCurrentId() == anime_id)
      NowPlayingDialog.Refresh();

  // ViewAnimePage
  //   Opens up anime page on MAL.
  //   lParam is an anime ID.
  } else if (action == L"ViewAnimePage") {
    int anime_id = static_cast<int>(lParam);
    mal::ViewAnimePage(anime_id);

  // ViewPanel(), ViewProfile(), ViewHistory()
  //   Opens up MyAnimeList user pages.
  } else if (action == L"ViewPanel") {
    mal::ViewPanel();
  } else if (action == L"ViewProfile") {
    mal::ViewProfile();
  } else if (action == L"ViewHistory") {
    mal::ViewHistory();

  // ViewUpcomingAnime
  //   Opens up upcoming anime page on MAL.
  } else if (action == L"ViewUpcomingAnime") {
    mal::ViewUpcomingAnime();

  // ===========================================================================

  // AddFolder()
  //   Opens up a dialog to add new root folder.
  } else if (action == L"AddFolder") {
    wstring path;
    if (BrowseForFolder(g_hMain, L"Please select a folder:", L"", path)) {
      Settings.Folders.root.push_back(path);
      if (Settings.Folders.watch_enabled) FolderMonitor.Enable();
      ExecuteAction(L"Settings", SECTION_LIBRARY, PAGE_LIBRARY_FOLDERS);
    }

  // ScanEpisodes(), ScanEpisodesAll()
  //   Checks episode availability.
  } else if (action == L"ScanEpisodes") {
    int anime_id = static_cast<int>(lParam);
    ScanAvailableEpisodes(anime_id, true, false);
  } else if (action == L"ScanEpisodesAll") {
    ScanAvailableEpisodes(anime::ID_UNKNOWN, true, false);

  // ToggleRecognition()
  //   Enables or disables anime recognition.
  } else if (action == L"ToggleRecognition") {
    Settings.Program.General.enable_recognition = !Settings.Program.General.enable_recognition;
    if (Settings.Program.General.enable_recognition) {
      MainDialog.ChangeStatus(L"Automatic anime recognition is now enabled.");
      CurrentEpisode.Set(anime::ID_UNKNOWN);
    } else {
      MainDialog.ChangeStatus(L"Automatic anime recognition is now disabled.");
      auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);
      CurrentEpisode.Set(anime::ID_NOTINLIST);
      if (anime_item) anime_item->EndWatching(CurrentEpisode);
    }

  // ToggleSharing()
  //   Enables or disables automatic sharing.
  } else if (action == L"ToggleSharing") {
    Settings.Program.General.enable_sharing = !Settings.Program.General.enable_sharing;
    UpdateToolsMenu();
    if (Settings.Program.General.enable_sharing) {
      MainDialog.ChangeStatus(L"Automatic sharing is now enabled.");
    } else {
      MainDialog.ChangeStatus(L"Automatic sharing is now disabled.");
    }

  // ToggleSynchronization()
  //   Enables or disables automatic list synchronization.
  } else if (action == L"ToggleSynchronization") {
    Settings.Program.General.enable_sync = !Settings.Program.General.enable_sync;
    UpdateToolsMenu();
    if (Settings.Program.General.enable_sync) {
      MainDialog.ChangeStatus(L"Automatic synchronization is now enabled.");
    } else {
      MainDialog.ChangeStatus(L"Automatic synchronization is now disabled.");
    }

  // ===========================================================================

  // AnnounceToHTTP(force)
  //   Sends an HTTP request.
  } else if (action == L"AnnounceToHTTP") {
    Announcer.Do(ANNOUNCE_TO_HTTP, nullptr, body == L"true");
  
  // AnnounceToMessenger(force)
  //   Changes MSN Messenger status text.
  } else if (action == L"AnnounceToMessenger") {
    Announcer.Do(ANNOUNCE_TO_MESSENGER, nullptr, body == L"true");
  
  // AnnounceToMIRC(force)
  //   Sends message to specified channels in mIRC.
  } else if (action == L"AnnounceToMIRC") {
    Announcer.Do(ANNOUNCE_TO_MIRC, nullptr, body == L"true");
  
  // AnnounceToSkype(force)
  //   Changes Skype mood text.
  //   Requires authorization.
  } else if (action == L"AnnounceToSkype") {
    Announcer.Do(ANNOUNCE_TO_SKYPE, nullptr, body == L"true");

  // AnnounceToTwitter(force)
  //   Changes Twitter status.
  } else if (action == L"AnnounceToTwitter") {
    Announcer.Do(ANNOUNCE_TO_TWITTER, nullptr, body == L"true");
  
  // ===========================================================================

  // EditAll([anime_id])
  //   Shows a dialog to edit details of an anime.
  //   lParam is an anime ID.
  } else if (action == L"EditAll") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (!anime_item || !anime_item->IsInList()) return;
    AnimeDialog.SetCurrentId(anime_id);
    AnimeDialog.SetCurrentPage(INFOPAGE_MYINFO);
    if (!AnimeDialog.IsWindow()) {
      AnimeDialog.Create(IDD_ANIME_INFO, g_hMain, false);
    } else {
      ActivateWindow(AnimeDialog.GetWindowHandle());
    }

  // EditDelete()
  //   Removes an anime from list.
  //   lParam is an anime ID.
  } else if (action == L"EditDelete") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    win::TaskDialog dlg;
    dlg.SetWindowTitle(anime_item->GetTitle().c_str());
    dlg.SetMainIcon(TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Are you sure you want to delete this title from your list?");
    dlg.AddButton(L"Yes", IDYES);
    dlg.AddButton(L"No", IDNO);
    dlg.Show(g_hMain);
    if (dlg.GetSelectedButtonID() == IDYES) {
      EventItem item;
      item.anime_id = anime_id;
      item.mode = HTTP_MAL_AnimeDelete;
      History.queue.Add(item);
    }

  // EditEpisode(value)
  //   Changes watched episode value of an anime.
  //   Value is optional.
  //   lParam is an anime ID.
  } else if (action == L"EditEpisode") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    int value = -1;
    if (body.empty()) {
      InputDialog dlg;
      dlg.SetNumbers(true, 0, anime_item->GetEpisodeCount(), anime_item->GetMyLastWatchedEpisode());
      dlg.title = anime_item->GetTitle();
      dlg.info = L"Please enter episode number for this title:";
      dlg.text = ToWstr(anime_item->GetMyLastWatchedEpisode());
      dlg.Show(g_hMain);
      if (dlg.result == IDOK) {
        value = ToInt(dlg.text);
      }
    } else {
      value = ToInt(body);
    }
    if (mal::IsValidEpisode(value, -1, anime_item->GetEpisodeCount())) {
      anime::Episode episode;
      episode.number = ToWstr(value);
      anime_item->AddToQueue(episode, true);
    }
  // DecrementEpisode()
  //   lParam is an anime ID.
  } else if (action == L"DecrementEpisode") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    int watched = anime_item->GetMyLastWatchedEpisode();
    auto event_item = History.queue.FindItem(anime_item->GetId(), EVENT_SEARCH_EPISODE);
    if (event_item && *event_item->episode == watched &&
        watched > anime_item->GetMyLastWatchedEpisode(false)) {
      event_item->enabled = false;
      History.queue.RemoveDisabled();
    } else {
      if (mal::IsValidEpisode(watched - 1, -1, anime_item->GetEpisodeCount())) {
        anime::Episode episode;
        episode.number = ToWstr(watched - 1);
        anime_item->AddToQueue(episode, true);
      }
    }
  // IncrementEpisode()
  //   lParam is an anime ID.
  } else if (action == L"IncrementEpisode") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    int watched = anime_item->GetMyLastWatchedEpisode();
    if (mal::IsValidEpisode(watched + 1, watched, anime_item->GetEpisodeCount())) {
      anime::Episode episode;
      episode.number = ToWstr(watched + 1);
      anime_item->AddToQueue(episode, true);
    }

  // EditScore(value)
  //   Changes anime score.
  //   Value must be between 0-10 and different from current score.
  //   lParam is an anime ID.
  } else if (action == L"EditScore") {
    int anime_id = static_cast<int>(lParam);
    EventItem item;
    item.anime_id = anime_id;
    item.score = ToInt(body);
    item.mode = HTTP_MAL_AnimeUpdate;
    History.queue.Add(item);

  // EditStatus(value)
  //   Changes anime status of user.
  //   Value must be 1, 2, 3, 4 or 6, and different from current status.
  //   lParam is an anime ID.
  } else if (action == L"EditStatus") {
    EventItem event_item;
    event_item.status = ToInt(body);
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    switch (anime_item->GetAiringStatus()) {
      case mal::STATUS_AIRING:
        if (*event_item.status == mal::MYSTATUS_COMPLETED) {
          MessageBox(g_hMain, 
            L"This anime is still airing, you cannot set it as completed.", 
            anime_item->GetTitle().c_str(), MB_ICONERROR);
          return;
        }
        break;
      case mal::STATUS_FINISHED:
        break;
      case mal::STATUS_NOTYETAIRED:
        if (*event_item.status != mal::MYSTATUS_PLANTOWATCH) {
          MessageBox(g_hMain, 
            L"This anime has not aired yet, you cannot set it as anything but Plan to Watch.", 
            anime_item->GetTitle().c_str(), MB_ICONERROR);
          return;
        }
        break;
      default:
        return;
    }
    switch (*event_item.status) {
      case mal::MYSTATUS_COMPLETED:
        event_item.episode = anime_item->GetEpisodeCount();
        if (*event_item.episode == 0) event_item.episode.Reset();
        if (!mal::IsValidDate(anime_item->GetMyDate(anime::DATE_END)))
          event_item.date_finish = mal::TranslateDateForApi(GetDate());
        break;
    }
    event_item.anime_id = anime_id;
    event_item.mode = HTTP_MAL_AnimeUpdate;
    History.queue.Add(event_item);

  // EditTags(tags)
  //   Changes anime tags.
  //   Tags must be separated by a comma.
  //   lParam is an anime ID.
  } else if (action == L"EditTags") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    InputDialog dlg;
    dlg.title = anime_item->GetTitle();
    dlg.info = L"Please enter tags for this title, separated by a comma:";
    dlg.text = anime_item->GetMyTags();
    dlg.Show(g_hMain);
    if (dlg.result == IDOK) {
      EventItem item;
      item.anime_id = anime_id;
      item.tags = dlg.text;
      item.mode = HTTP_MAL_AnimeUpdate;
      History.queue.Add(item);
    }

  // EditTitles(titles)
  //   Changes alternative titles of an anime.
  //   Titles must be separated by "; ".
  //   lParam is an anime ID.
  } else if (action == L"EditTitles") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    InputDialog dlg;
    dlg.title = anime_item->GetTitle();
    dlg.info = L"Please enter alternative titles, separated by a semicolon:";
    dlg.text = Join(anime_item->GetUserSynonyms(), L"; ");
    dlg.Show(g_hMain);
    if (dlg.result == IDOK) {
      anime_item->SetUserSynonyms(dlg.text);
      Meow.UpdateCleanTitles(anime_id);
      Settings.Save();
    }
  
  // ===========================================================================
  
  // OpenFolder()
  //   Searches for anime folder and opens it.
  //   lParam is an anime ID.
  } else if (action == L"OpenFolder") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (!anime_item || !anime_item->IsInList()) return;
    MainDialog.ChangeStatus(L"Searching for folder...");
    if (!anime_item->CheckFolder()) {
      win::TaskDialog dlg;
      dlg.SetWindowTitle(L"Folder Not Found");
      dlg.SetMainIcon(TD_ICON_INFORMATION);
      dlg.SetMainInstruction(L"Taiga couldn't find the folder of this anime. "
                             L"Would you like to set it manually?");
      dlg.AddButton(L"Yes", IDYES);
      dlg.AddButton(L"No", IDNO);
      dlg.Show(g_hMain);
      if (dlg.GetSelectedButtonID() == IDYES) {
        wstring default_path, path;
        if (!Settings.Folders.root.empty())
          default_path = Settings.Folders.root.front();
        if (BrowseForFolder(g_hMain, L"Choose an anime folder", default_path, path)) {
          anime_item->SetFolder(path);
          Settings.Save();
        }
      }
    }
    MainDialog.ChangeStatus();
    if (!anime_item->GetFolder().empty()) {
      Execute(anime_item->GetFolder());
    }

  // SetFolder()
  //   Lets user set an anime folder.
  //   lParam is an anime ID.
  } else if (action == L"SetFolder") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    wstring path, title = L"Anime title: " + anime_item->GetTitle();
    if (BrowseForFolder(MainDialog.GetWindowHandle(), title.c_str(), L"", path)) {
      anime_item->SetFolder(path);
      Settings.Save();
      anime_item->CheckEpisodes();
    }

  // ===========================================================================

  // PlayEpisode(value)
  //   Searches for an episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (action == L"PlayEpisode") {
    int number = ToInt(body);
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    anime_item->PlayEpisode(number);
  
  // PlayLast()
  //   Searches for the last watched episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (action == L"PlayLast") {
    int anime_id = static_cast<int>(lParam);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    int number = anime_item->GetMyLastWatchedEpisode();
    anime_item->PlayEpisode(number);
  
  // PlayNext([anime_id])
  //   Searches for the next episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (action == L"PlayNext") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item->GetEpisodeCount() != 1) {
      anime_item->PlayEpisode(anime_item->GetMyLastWatchedEpisode() + 1);
    } else {
      anime_item->PlayEpisode(1);
    }
  
  // PlayRandom()
  //   Searches for a random episode of an anime and plays it.
  //   lParam is an anime ID.
  } else if (action == L"PlayRandom") {
    int anime_id = body.empty() ? static_cast<int>(lParam) : ToInt(body);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item && anime_item->CheckFolder()) {
      int total = anime_item->GetEpisodeCount();
      if (total == 0)
        total = anime_item->GetMyLastWatchedEpisode() + 1;
      wstring path;
      srand(static_cast<unsigned int>(GetTickCount()));
      for (int i = 0; i < total; i++) {
        int episode_number = rand() % total + 1;
        path = SearchFileFolder(*anime_item, anime_item->GetFolder(), episode_number, false);
        if (!path.empty()) {
          Execute(path);
          return;
        }
      }
    }
    win::TaskDialog dlg;
    dlg.SetWindowTitle(L"Play Random Episode");
    dlg.SetMainIcon(TD_ICON_ERROR);
    dlg.SetMainInstruction(L"Could not find any episode to play.");
    dlg.Show(g_hMain);
  
  // PlayRandomAnime()
  //   Searches for a random episode of a random anime and plays it.
  } else if (action == L"PlayRandomAnime") {
    static time_t time_last_checked = 0;
    time_t time_now = time(nullptr);
    if (time_now > time_last_checked + (60 * 2)) { // 2 minutes
      ScanAvailableEpisodes(anime::ID_UNKNOWN, false, false);
      time_last_checked = time_now;
    }
    std::vector<int> valid_ids;
    foreach_(it, AnimeDatabase.items) {
      anime::Item& anime_item = it->second;
      if (!anime_item.IsInList())
        continue;
      if (!anime_item.IsNewEpisodeAvailable())
        continue;
      switch (anime_item.GetMyStatus()) {
        case mal::MYSTATUS_NOTINLIST:
        case mal::MYSTATUS_COMPLETED:
        case mal::MYSTATUS_DROPPED:
          continue;
      }
      valid_ids.push_back(anime_item.GetId());
    }
    foreach_ (id, valid_ids) {
      srand(static_cast<unsigned int>(GetTickCount()));
      size_t max_value = valid_ids.size();
      size_t index = rand() % max_value + 1;
      int anime_id = valid_ids.at(index);
      auto anime_item = AnimeDatabase.FindItem(anime_id);
      if (anime_item->PlayEpisode(anime_item->GetMyLastWatchedEpisode() + 1))
        return;
    }
    win::TaskDialog dlg;
    dlg.SetWindowTitle(L"Play Random Anime");
    dlg.SetMainIcon(TD_ICON_ERROR);
    dlg.SetMainInstruction(L"Could not find any episode to play.");
    dlg.Show(g_hMain);

  // ===========================================================================

  // Season_Load(file)
  //   Loads season data.
  } else if (action == L"Season_Load") {
    if (SeasonDatabase.Load(body)) {
      SeasonDatabase.Review();
      SeasonDialog.RefreshList();
      SeasonDialog.RefreshStatus();
      SeasonDialog.RefreshToolbar();
      if (SeasonDatabase.IsRefreshRequired()) {
        win::TaskDialog dlg;
        wstring title = L"Season - " + SeasonDatabase.name;
        dlg.SetWindowTitle(title.c_str());
        dlg.SetMainIcon(TD_ICON_INFORMATION);
        dlg.SetMainInstruction(L"Would you like to refresh this season's data?");
        dlg.SetContent(L"It seems that we don't know much about some anime titles in this season. "
                       L"Taiga will connect to MyAnimeList to retrieve missing information and images.");
        dlg.AddButton(L"Yes", IDYES);
        dlg.AddButton(L"No", IDNO);
        dlg.Show(g_hMain);
        if (dlg.GetSelectedButtonID() == IDYES)
          SeasonDialog.RefreshData();
      }
    }

  // Season_GroupBy(group)
  //   Groups season data.
  } else if (action == L"Season_GroupBy") {
    SeasonDialog.group_by = ToInt(body);
    SeasonDialog.RefreshList();
    SeasonDialog.RefreshToolbar();

  // Season_SortBy(sort)
  //   Sorts season data.
  } else if (action == L"Season_SortBy") {
    SeasonDialog.sort_by = ToInt(body);
    SeasonDialog.RefreshList();
    SeasonDialog.RefreshToolbar();

  // Season_RefreshItemData()
  //   Refreshes an individual season item data.
  } else if (action == L"Season_RefreshItemData") {
    SeasonDialog.RefreshData(static_cast<int>(lParam));

  // Season_ViewAs(mode)
  //   Changes view mode.
  } else if (action == L"Season_ViewAs") {
    SeasonDialog.SetViewMode(ToInt(body));
    SeasonDialog.RefreshList();
    SeasonDialog.RefreshToolbar();
  
  // Unknown
  } else {
    LOG(LevelWarning, L"Unknown action: " + action);
  }
}