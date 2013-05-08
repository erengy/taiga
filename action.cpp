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

#include "std.h"

#include "anime.h"
#include "anime_db.h"
#include "anime_filter.h"
#include "announce.h"
#include "common.h"
#include "debug.h"
#include "feed.h"
#include "history.h"
#include "http.h"
#include "monitor.h"
#include "myanimelist.h"
#include "process.h"
#include "resource.h"
#include "settings.h"
#include "stats.h"
#include "string.h"
#include "taiga.h"
#include "theme.h"

#include "dlg/dlg_about.h"
#include "dlg/dlg_anime_info.h"
#include "dlg/dlg_anime_info_page.h"
#include "dlg/dlg_anime_list.h"
#include "dlg/dlg_history.h"
#include "dlg/dlg_input.h"
#include "dlg/dlg_main.h"
#include "dlg/dlg_search.h"
#include "dlg/dlg_season.h"
#include "dlg/dlg_settings.h"
#include "dlg/dlg_test_recognition.h"
#include "dlg/dlg_torrent.h"
#include "dlg/dlg_feed_filter.h"
#include "dlg/dlg_update.h"

#include "win32/win_taskbar.h"
#include "win32/win_taskdialog.h"

// =============================================================================

void ExecuteAction(wstring action, WPARAM wParam, LPARAM lParam) {
  debug::Print(L"Action :: " + action + L"\n");
  
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
        History.queue.Check();
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
  } else if (action == L"URL") {
    if (AnimeDatabase.GetCurrentId() > anime::ID_UNKNOWN) {
      wstring title = AnimeDatabase.GetCurrentItem()->GetTitle();
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
    int anime_id = lParam ? static_cast<int>(lParam) : AnimeDatabase.GetCurrentId();
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
      //Taiga.Updater.RunActions();
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
  //   lParam is the initial page number.
  } else if (action == L"Settings") {
    SettingsDialog.SetCurrentPage(lParam);
    if (!SettingsDialog.IsWindow()) {
      SettingsDialog.Create(IDD_SETTINGS, g_hMain, true);
    } else {
      ActivateWindow(SettingsDialog.GetWindowHandle());
    }
  
  // SearchAnime()
  } else if (action == L"SearchAnime") {
    if (body.empty()) return;
    if (Settings.Account.MAL.api == MAL_API_OFFICIAL) {
      if (Settings.Account.MAL.user.empty() || Settings.Account.MAL.password.empty()) {
        win32::TaskDialog dlg(APP_TITLE, TD_ICON_INFORMATION);
        dlg.SetMainInstruction(L"Would you like to set your account information first?");
        dlg.SetContent(L"Anime search requires authentication, which means, "
          L"you need to enter a valid user name and password to search MyAnimeList.");
        dlg.AddButton(L"Yes", IDYES);
        dlg.AddButton(L"No", IDNO);
        dlg.Show(g_hMain);
        if (dlg.GetSelectedButtonID() == IDYES) {
          ExecuteAction(L"Settings", 0, PAGE_ACCOUNT);
        }
        return;
      }
    }
    MainDialog.navigation.SetCurrentPage(SIDEBAR_ITEM_SEARCH);
    SearchDialog.Search(body);

  // SearchTorrents()
  } else if (action == L"SearchTorrents") {
    Feed* feed = Aggregator.Get(FEED_CATEGORY_LINK);
    if (feed) {
      anime::Episode episode;
      episode.anime_id = AnimeDatabase.GetCurrentId();
      MainDialog.navigation.SetCurrentPage(SIDEBAR_ITEM_FEEDS);
      MainDialog.edit.SetText(AnimeDatabase.GetCurrentItem()->GetTitle());
      MainDialog.ChangeStatus(L"Searching torrents for \"" + 
        AnimeDatabase.GetCurrentItem()->GetTitle() + L"\"...");
      feed->Check(ReplaceVariables(body, episode));
    }

  // ShowSidebar()
  } else if (action == L"ShowSidebar") {
    MainDialog.treeview.Show(!MainDialog.treeview.IsVisible());
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

  // ViewAnimePage
  //   Opens up anime page on MAL.
  //   lParam is an anime ID.
  } else if (action == L"ViewAnimePage") {
    int anime_id = lParam ? static_cast<int>(lParam) : AnimeDatabase.GetCurrentId();
    mal::ViewAnimePage(anime_id);

  // ViewPanel(), ViewProfile(), ViewHistory()
  //   Opens up MyAnimeList user pages.
  } else if (action == L"ViewPanel") {
    mal::ViewPanel();
  } else if (action == L"ViewProfile") {
    mal::ViewProfile();
  } else if (action == L"ViewHistory") {
    mal::ViewHistory();

  // ===========================================================================

  // AddFolder()
  //   Opens up a dialog to add new root folder.
  } else if (action == L"AddFolder") {
    wstring path;
    if (BrowseForFolder(g_hMain, L"Please select a folder:", 
      BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON, path)) {
        Settings.Folders.root.push_back(path);
        if (Settings.Folders.watch_enabled) FolderMonitor.Enable();
        ExecuteAction(L"Settings", 0, PAGE_FOLDERS_ROOT);
    }

  // CheckEpisodes()
  //   Checks new episodes or episode availability.
  //   wParam is a BOOL value that activates silent operation mode.
  //   If body text is empty, search is made for all list items.
  } else if (action == L"CheckEpisodes") {
    // Check silent operation mode
    bool silent = (wParam == TRUE);
    if (!silent) TaskbarList.SetProgressState(TBPF_NORMAL);
    // If there's no anime folder set, we'll check them first
    bool check_folder = true;
    for (auto it = AnimeDatabase.items.begin(); it != AnimeDatabase.items.end(); ++it) {
      if (it->second.IsInList() && !it->second.GetFolder().empty()) {
        check_folder = false;
        break;
      }
    }
    if (check_folder && !silent && !Settings.Folders.root.empty()) {
      win32::TaskDialog dlg(APP_TITLE, TD_ICON_INFORMATION);
      dlg.SetMainInstruction(L"Would you like to search for anime folders first?");
      dlg.SetContent(L"This feature only checks specific anime folders for new episodes. "
        L"As you have none set at the moment, searching for folders is highly recommended.");
      dlg.AddButton(L"Yes", IDYES);
      dlg.AddButton(L"No", IDNO);
      dlg.Show(g_hMain);
      check_folder = (dlg.GetSelectedButtonID() == IDYES);
    }
    // Search for all list items
    if (body.empty()) {
      size_t i = 0;
      for (auto it = AnimeDatabase.items.begin(); it != AnimeDatabase.items.end(); ++it) {
        if (!silent) TaskbarList.SetProgressValue(i++, AnimeDatabase.items.size());
        switch (it->second.GetMyStatus()) {
          case mal::MYSTATUS_WATCHING:
          case mal::MYSTATUS_ONHOLD:
          case mal::MYSTATUS_PLANTOWATCH:
            if (!silent) {
              MainDialog.ChangeStatus(L"Searching... (" + it->second.GetTitle() + L")");
            }
            it->second.CheckEpisodes(
              Settings.Program.List.progress_mode == LIST_PROGRESS_AVAILABLEEPS ? -1 : 0, 
              check_folder);
        }
      }
    // Search only for selected list item
    } else {
      AnimeDatabase.GetCurrentItem()->CheckEpisodes(
        Settings.Program.List.progress_mode == LIST_PROGRESS_AVAILABLEEPS ? -1 : 0,
        true);
    }
    // We're done
    if (!silent) {
      TaskbarList.SetProgressState(TBPF_NOPROGRESS);
      MainDialog.ChangeStatus(L"Search finished.");
    }

  // ToggleRecognition()
  //   Enables or disables anime recognition.
  } else if (action == L"ToggleRecognition") {
    Taiga.is_recognition_enabled = !Taiga.is_recognition_enabled;
    if (Taiga.is_recognition_enabled) {
      MainDialog.ChangeStatus(L"Automatic anime recognition is now enabled.");
      CurrentEpisode.Set(anime::ID_UNKNOWN);
    } else {
      MainDialog.ChangeStatus(L"Automatic anime recognition is now disabled.");
      auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);
      CurrentEpisode.Set(anime::ID_NOTINLIST);
      if (anime_item) anime_item->EndWatching(CurrentEpisode);
    }

  // ToggleSharing()
  //   Enables or disables announcements.
  } else if (action == L"ToggleSharing") {
    Taiga.is_sharing_enabled = !Taiga.is_sharing_enabled;
    UpdateToolsMenu();
    if (Taiga.is_sharing_enabled) {
      MainDialog.ChangeStatus(L"Automatic sharing is now enabled.");
    } else {
      MainDialog.ChangeStatus(L"Automatic sharing is now disabled.");
    }

  // ToggleListUpdate()
  //   Enables or disables list updates.
  } else if (action == L"ToggleListUpdate") {
    Taiga.is_update_enabled = !Taiga.is_update_enabled;
    UpdateToolsMenu();
    if (Taiga.is_update_enabled) {
      MainDialog.ChangeStatus(L"Automatic list update is now enabled.");
    } else {
      MainDialog.ChangeStatus(L"Automatic list update is now disabled.");
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
  } else if (action == L"EditAll") {
    int anime_id = body.empty() ? AnimeDatabase.GetCurrentId() : ToInt(body);
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
  } else if (action == L"EditDelete") {
    win32::TaskDialog dlg;
    dlg.SetWindowTitle(AnimeDatabase.GetCurrentItem()->GetTitle().c_str());
    dlg.SetMainIcon(TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Are you sure you want to delete this title from your list?");
    dlg.AddButton(L"Yes", IDYES);
    dlg.AddButton(L"No", IDNO);
    dlg.Show(g_hMain);
    if (dlg.GetSelectedButtonID() == IDYES) {
      EventItem item;
      item.anime_id = AnimeDatabase.GetCurrentId();
      item.mode = HTTP_MAL_AnimeDelete;
      History.queue.Add(item);
    }

  // EditEpisode(value)
  //   Changes watched episode value of an anime.
  //   Value is optional.
  } else if (action == L"EditEpisode") {
    auto anime_item = AnimeDatabase.GetCurrentItem();
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
    if (mal::IsValidEpisode(value, 0, anime_item->GetEpisodeCount())) {
      anime::Episode episode;
      episode.number = ToWstr(value);
      anime_item->AddToQueue(episode, true);
    }
  // DecrementEpisode()
  } else if (action == L"DecrementEpisode") {
    auto anime_item = AnimeDatabase.GetCurrentItem();
    int watched = anime_item->GetMyLastWatchedEpisode();
    auto event_item = History.queue.FindItem(anime_item->GetId(), EVENT_SEARCH_EPISODE);
    if (event_item && *event_item->episode == watched) {
      event_item->enabled = false;
      History.queue.RemoveDisabled();
    } else {
      if (mal::IsValidEpisode(watched - 1, 0, anime_item->GetEpisodeCount())) {
        anime::Episode episode;
        episode.number = ToWstr(watched - 1);
        anime_item->AddToQueue(episode, true);
      }
    }
  // IncrementEpisode()
  } else if (action == L"IncrementEpisode") {
    auto anime_item = AnimeDatabase.GetCurrentItem();
    int watched = anime_item->GetMyLastWatchedEpisode();
    if (mal::IsValidEpisode(watched + 1, watched, anime_item->GetEpisodeCount())) {
      anime::Episode episode;
      episode.number = ToWstr(watched + 1);
      anime_item->AddToQueue(episode, true);
    }

  // EditScore(value)
  //   Changes anime score.
  //   Value must be between 0-10 and different from current score.
  } else if (action == L"EditScore") {
    EventItem item;
    item.anime_id = AnimeDatabase.GetCurrentId();
    item.score = ToInt(body);
    item.mode = HTTP_MAL_ScoreUpdate;
    History.queue.Add(item);

  // EditStatus(value)
  //   Changes anime status of user.
  //   Value must be 1, 2, 3, 4 or 6, and different from current status.
  } else if (action == L"EditStatus") {
    EventItem event_item;
    event_item.status = ToInt(body);
    switch (AnimeDatabase.GetCurrentItem()->GetAiringStatus()) {
      case mal::STATUS_AIRING:
        if (*event_item.status == mal::MYSTATUS_COMPLETED) {
          MessageBox(g_hMain, 
            L"This anime is still airing, you cannot set it as completed.", 
            AnimeDatabase.GetCurrentItem()->GetTitle().c_str(), MB_ICONERROR);
          return;
        }
        break;
      case mal::STATUS_FINISHED:
        break;
      case mal::STATUS_NOTYETAIRED:
        if (*event_item.status != mal::MYSTATUS_PLANTOWATCH) {
          MessageBox(g_hMain, 
            L"This anime has not aired yet, you cannot set it as anything but Plan to Watch.", 
            AnimeDatabase.GetCurrentItem()->GetTitle().c_str(), MB_ICONERROR);
          return;
        }
        break;
      default:
        return;
    }
    switch (*event_item.status) {
      case mal::MYSTATUS_COMPLETED:
        event_item.episode = AnimeDatabase.GetCurrentItem()->GetEpisodeCount();
        if (*event_item.episode == 0) event_item.episode.Reset();
        if (!mal::IsValidDate(AnimeDatabase.GetCurrentItem()->GetMyDate(anime::DATE_END)))
          event_item.date_finish = mal::TranslateDateForApi(GetDate());
        break;
    }
    event_item.anime_id = AnimeDatabase.GetCurrentId();
    event_item.mode = event_item.episode ? HTTP_MAL_AnimeEdit : HTTP_MAL_StatusUpdate;
    History.queue.Add(event_item);

  // EditTags(tags)
  //   Changes anime tags.
  //   Tags must be separated by a comma.
  } else if (action == L"EditTags") {
    InputDialog dlg;
    dlg.title = AnimeDatabase.GetCurrentItem()->GetTitle();
    dlg.info = L"Please enter tags for this title, separated by a comma:";
    dlg.text = AnimeDatabase.GetCurrentItem()->GetMyTags();
    dlg.Show(g_hMain);
    if (dlg.result == IDOK) {
      EventItem item;
      item.anime_id = AnimeDatabase.GetCurrentId();
      item.tags = dlg.text;
      item.mode = HTTP_MAL_TagUpdate;
      History.queue.Add(item);
    }

  // EditTitles(titles)
  //   Changes alternative titles of an anime.
  //   Titles must be separated by "; ".
  } else if (action == L"EditTitles") {
    InputDialog dlg;
    dlg.title = AnimeDatabase.GetCurrentItem()->GetTitle();
    dlg.info = L"Please enter alternative titles, separated by a semicolon:";
    dlg.text = Join(AnimeDatabase.GetCurrentItem()->GetUserSynonyms(), L"; ");
    dlg.Show(g_hMain);
    if (dlg.result == IDOK) {
      AnimeDatabase.GetCurrentItem()->SetUserSynonyms(dlg.text, true);
    }
  
  // ===========================================================================
  
  // OpenFolder()
  //   Searches for anime folder and opens it.
  } else if (action == L"OpenFolder") {
    auto anime_item = AnimeDatabase.GetCurrentItem();
    if (!anime_item || !anime_item->IsInList()) return;
    if (anime_item->GetFolder().empty()) {
      MainDialog.ChangeStatus(L"Searching for folder...");
      if (anime_item->CheckFolder()) {
        MainDialog.ChangeStatus(L"Folder found.");
      } else {
        MainDialog.ChangeStatus(L"Folder not found.");
        return;
      }
    }
    Execute(anime_item->GetFolder());

  // SetFolder()
  //   Lets user set an anime folder.
  } else if (action == L"SetFolder") {
    wstring path, title = L"Anime title: " + AnimeDatabase.GetCurrentItem()->GetTitle();
    if (BrowseForFolder(MainDialog.GetWindowHandle(), title.c_str(), 
      BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON, path)) {
        AnimeDatabase.GetCurrentItem()->SetFolder(path, true);
        AnimeDatabase.GetCurrentItem()->CheckEpisodes();
    }

  // ===========================================================================

  // PlayEpisode(value)
  //   Searches for an episode of an anime and plays it.
  } else if (action == L"PlayEpisode") {
    int number = ToInt(body);
    AnimeDatabase.GetCurrentItem()->PlayEpisode(number);
  
  // PlayLast()
  //   Searches for the last watched episode of an anime and plays it.
  } else if (action == L"PlayLast") {
    int number = AnimeDatabase.GetCurrentItem()->GetMyLastWatchedEpisode();
    AnimeDatabase.GetCurrentItem()->PlayEpisode(number);
  
  // PlayNext([anime_id])
  //   Searches for the next episode of an anime and plays it.
  } else if (action == L"PlayNext") {
    int anime_id = body.empty() ? AnimeDatabase.GetCurrentId() : ToInt(body);
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item->GetEpisodeCount() != 1) {
      anime_item->PlayEpisode(anime_item->GetMyLastWatchedEpisode() + 1);
    } else {
      anime_item->PlayEpisode(1);
    }
  
  // PlayRandom()
  //   Searches for a random episode of an anime and plays it.
  } else if (action == L"PlayRandom") {
    int anime_id = ToInt(body);
    auto anime_item = anime_id ? AnimeDatabase.FindItem(anime_id) : AnimeDatabase.GetCurrentItem();
    if (anime_item) {
      int total = anime_item->GetEpisodeCount();
      if (total == 0) total = anime_item->GetMyLastWatchedEpisode() + 1;
      for (int i = 0; i < total; i++) {
        srand(static_cast<unsigned int>(GetTickCount()));
        int episode = rand() % total + 1;
        anime_item->CheckFolder();
        wstring file = SearchFileFolder(*anime_item, anime_item->GetFolder(), episode, false);
        if (!file.empty()) {
          Execute(file);
          break;
        }
      }
    }
  
  // PlayRandomAnime()
  //   Searches for a random episode of a random anime and plays it.
  } else if (action == L"PlayRandomAnime") {
    srand(static_cast<unsigned int>(GetTickCount()));
    for (size_t i = 0; i < AnimeDatabase.items.size(); i++) {
      int max_anime_id = AnimeDatabase.items.rbegin()->first;
      int anime_id = rand() % max_anime_id + 1;
      auto anime_item = AnimeDatabase.FindItem(anime_id);
      if (!anime_item || !anime_item->IsInList()) continue;
      int episode_count = anime_item->GetMyLastWatchedEpisode() + 1;
      int episode_number = rand() % episode_count  + 1;
      anime_item->CheckFolder();
      wstring file = SearchFileFolder(*anime_item, anime_item->GetFolder(), episode_number, false);
      if (!file.empty()) {
        Execute(file);
        break;
      }
    }

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
        win32::TaskDialog dlg;
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
  }
}