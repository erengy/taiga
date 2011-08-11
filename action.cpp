/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include "animedb.h"
#include "animelist.h"
#include "announce.h"
#include "common.h"
#include "dlg/dlg_about.h"
#include "dlg/dlg_anime_info.h"
#include "dlg/dlg_anime_info_page.h"
#include "dlg/dlg_filter.h"
#include "dlg/dlg_input.h"
#include "dlg/dlg_main.h"
#include "dlg/dlg_search.h"
#include "dlg/dlg_season.h"
#include "dlg/dlg_settings.h"
#include "dlg/dlg_test_recognition.h"
#include "dlg/dlg_torrent.h"
#include "dlg/dlg_feed_filter.h"
#include "dlg/dlg_update.h"
#include "event.h"
#include "feed.h"
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
#include "win32/win_taskbar.h"
#include "win32/win_taskdialog.h"

// =============================================================================

void ExecuteAction(wstring action, WPARAM wParam, LPARAM lParam) {
  DEBUG_PRINT(L"Action :: " + action + L"\n");
  
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

  // Login()
  //   Logs in to MyAnimeList.
  if (action == L"Login") {
    if (AnimeList.Count > 0) {
      MainWindow.ChangeStatus(L"Logging in...");
      bool result = MAL.Login();
      MainWindow.EnableInput(!result);
      if (!result) MainWindow.ChangeStatus();
    } else {
      // Retrieve anime list and log in afterwards
      ExecuteAction(L"RefreshList", TRUE);
    }

  // Logout()
  //   Logs out of MyAnimeList.
  } else if (action == L"Logout") {
    if (Taiga.LoggedIn) {
      Taiga.LoggedIn = false;
      MainWindow.m_Toolbar.SetButtonImage(0, Icon24_Offline);
      MainWindow.m_Toolbar.SetButtonText(0, L"Log in");
      MainWindow.m_Toolbar.SetButtonTooltip(0, L"Log in");
      MainWindow.ChangeStatus((body.empty() ? Settings.Account.MAL.User : body) + L" is now logged out.");
      MainWindow.RefreshMenubar();
      MainWindow.UpdateTip();
      MainClient.ClearCookies();
    }

  // LoginLogout(), ToggleLogin()
  //   Logs in or out depending on status.
  } else if (action == L"LoginLogout" || action == L"ToggleLogin") {
    ExecuteAction(Taiga.LoggedIn ? L"Logout" : L"Login");

  // RefreshList()
  //   Retrieves anime list from MyAnimeList.
  //   wParam is a BOOL value
  } else if (action == L"RefreshList") {
    int user_index = EventQueue.GetUserIndex();
    if (user_index > -1) {
      for (auto it = EventQueue.List[user_index].Items.begin(); 
        it != EventQueue.List[user_index].Items.end(); ++it) {
          if (it->Mode == HTTP_MAL_AnimeAdd) {
            // Refreshing list would cause this item to disappear
            if (Taiga.LoggedIn) {
              EventQueue.Check();
            } else {
              ExecuteAction(L"Login");
            }
            return;
          }
      }
    }
    MainWindow.ChangeStatus(L"Refreshing list...");
    bool result = MAL.GetList(wParam == TRUE);
    MainWindow.EnableInput(!result);
    if (!result) MainWindow.ChangeStatus();

  // Synchronize()
  //   Synchronize local and remote lists.
  } else if (action == L"Synchronize") {
    if (Taiga.LoggedIn && EventQueue.GetItemCount() > 0) {
      EventQueue.Check();
    } else {
      ExecuteAction(L"RefreshList");
    }

  // ViewPanel(), ViewProfile(), ViewHistory()
  //   Opens up MyAnimeList user pages.
  } else if (action == L"ViewPanel") {
    MAL.ViewPanel();
  } else if (action == L"ViewProfile") {
    MAL.ViewProfile();
  } else if (action == L"ViewHistory") {
    MAL.ViewHistory();

  // ===========================================================================

  // Execute(path)
  //   Executes a file or folder.
  } else if (action == L"Execute") {
    Execute(body);

  // URL(address)
  //   Opens a web page.
  } else if (action == L"URL") {
    if (AnimeList.Index > 0) {
      wstring title = AnimeList.Items[AnimeList.Index].Series_Title;
      EraseChars(title, L"_!?.,:;~+");
      Erase(title, L" -");
      Replace(body, L"%title%", title);
    }
    ExecuteLink(body);

  // ===========================================================================

  // About()
  //   Shows about window.
  } else if (action == L"About") {
    if (!AboutWindow.IsWindow()) {
      AboutWindow.Create(IDD_ABOUT, g_hMain, true);
    } else {
      ActivateWindow(AboutWindow.GetWindowHandle());
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
    MainWindow.Destroy();

  // Filter()
  //   Shows filter window.
  } else if (action == L"Filter") {
    if (!FilterWindow.IsWindow()) {
      FilterWindow.Create(IDD_FILTER, g_hMain, false);
    } else {
      ActivateWindow(FilterWindow.GetWindowHandle());
    }

  // Info()
  //   Shows anime information window.
  //   lParam is a pointer to an anime list item.
  } else if (action == L"Info") {
    CAnime* anime_item = lParam ? 
      reinterpret_cast<CAnime*>(lParam) : &AnimeList.Items[AnimeList.Index];
    AnimeWindow.Refresh(anime_item);
    AnimeWindow.SetCurrentPage(TAB_SERIESINFO);
    if (!AnimeWindow.IsWindow()) {
      AnimeWindow.Create(IDD_ANIME_INFO, g_hMain, false);
    } else {
      ActivateWindow(AnimeWindow.GetWindowHandle());
    }

  // MainWindow()
  } else if (action == L"MainWindow") {
    if (!MainWindow.IsWindow()) {
      MainWindow.Create(IDD_MAIN, NULL, false);
      //Taiga.UpdateHelper.RunActions();
    } else {
      ActivateWindow(MainWindow.GetWindowHandle());
    }

  // RecognitionTest()
  //   Shows recognition test window.
  } else if (action == L"RecognitionTest") {
    if (!RecognitionTestWindow.IsWindow()) {
      RecognitionTestWindow.Create(IDD_TEST_RECOGNITION, NULL, false);
    } else {
      ActivateWindow(RecognitionTestWindow.GetWindowHandle());
    }

  // SeasonBrowser()
  //   Shows season browser window.
  } else if (action == L"SeasonBrowser") {
    if (!SeasonWindow.IsWindow()) {
      SeasonWindow.Create(IDD_SEASON, nullptr, false);
    } else {
      ActivateWindow(SeasonWindow.GetWindowHandle());
    }

  // SetSearchMode()
  //   Changes search bar mode.
  //   Body text has 4 parameters: Menu index, search mode, cue text, search URL
  } else if (action == L"SetSearchMode") {
    vector<wstring> body_vector;
    Split(body, L", ", body_vector);
    if (body_vector.size() > 2) {
      if (body_vector[1] == L"List") {
        MainWindow.m_SearchBar.SetMode(
          ToINT(body_vector[0]), SEARCH_MODE_LIST, body_vector[2]);
      } else if (body_vector[1] == L"MAL") {
        MainWindow.m_SearchBar.SetMode(
          ToINT(body_vector[0]), SEARCH_MODE_MAL, body_vector[2]);
      } else if (body_vector[1] == L"Torrent" && body_vector.size() > 3) {
        MainWindow.m_SearchBar.SetMode(
          ToINT(body_vector[0]), SEARCH_MODE_TORRENT, body_vector[2], body_vector[3]);
      } else if (body_vector[1] == L"Web" && body_vector.size() > 3) {
        MainWindow.m_SearchBar.SetMode(
          ToINT(body_vector[0]), SEARCH_MODE_WEB, body_vector[2], body_vector[3]);
      }
    }

  // Settings()
  //   Shows settings window.
  //   lParam is the initial page number.
  } else if (action == L"Settings") {
    SettingsWindow.SetCurrentPage(lParam);
    if (!SettingsWindow.IsWindow()) {
      SettingsWindow.Create(IDD_SETTINGS, g_hMain, true);
    } else {
      ActivateWindow(SettingsWindow.GetWindowHandle());
    }

  // SaveSettings()
  } else if (action == L"SaveSettings") {
    Settings.Write();
  
  // SearchAnime()
  } else if (action == L"SearchAnime") {
    if (body.empty()) return;
    if (Settings.Account.MAL.API == MAL_API_OFFICIAL) {
      if (Settings.Account.MAL.User.empty() || Settings.Account.MAL.Password.empty()) {
        CTaskDialog dlg(APP_TITLE, TD_ICON_INFORMATION);
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
    if (!SearchWindow.IsWindow()) {
      SearchWindow.Create(IDD_SEARCH, g_hMain, false);
    } else {
      ActivateWindow(SearchWindow.GetWindowHandle());
    }
    SearchWindow.Search(body);

  // SearchTorrents()
  } else if (action == L"SearchTorrents") {
    CFeed* pFeed = Aggregator.Get(FEED_CATEGORY_LINK);
    if (pFeed) {
      CEpisode episode;
      episode.AnimeId = AnimeList.Items[AnimeList.Index].Series_ID;
      pFeed->Check(ReplaceVariables(body, episode));
      ExecuteAction(L"Torrents");
    }

  // ShowListStats()
  } else if (action == L"ShowListStats") {
    Stats.CalculateAll();
    if (!AnimeList.User.Name.empty()) {
      wstring main_instruction, content;
      main_instruction = AnimeList.User.Name + L"'s anime list stats:";
      content += L"\u2022 Anime count: \t\t" + ToWSTR(Stats.anime_count_);
      content += L"\n\u2022 Episode count: \t\t" + ToWSTR(Stats.episode_count_);
      content += L"\n\u2022 Life spent watching: \t" + Stats.life_spent_;
      content += L"\n\u2022 Mean score: \t\t" + ToWSTR(Stats.score_mean_, 2);
      content += L"\n\u2022 Score deviation: \t\t" + ToWSTR(Stats.score_dev_, 2);
      CTaskDialog dlg(APP_TITLE, TD_ICON_INFORMATION);
      dlg.SetMainInstruction(main_instruction.c_str());
      dlg.SetContent(content.c_str());
      dlg.AddButton(L"OK", IDOK);
      dlg.Show(g_hMain);
    }

  // Torrents()
  //   Shows torrents window.
  } else if (action == L"Torrents") {
    if (!TorrentWindow.IsWindow()) {
      TorrentWindow.Create(IDD_TORRENT, NULL, false);
    } else {
      ActivateWindow(TorrentWindow.GetWindowHandle());
    }

  // TorrentAddFilter()
  //   Shows add new filter window.
  //   wParam is a BOOL value that represents modal status.
  //   lParam is the handle of the parent window.
  } else if (action == L"TorrentAddFilter") {
    if (!FeedFilterWindow.IsWindow()) {
      FeedFilterWindow.Create(IDD_FEED_FILTER, 
        reinterpret_cast<HWND>(lParam), wParam != FALSE);
    } else {
      ActivateWindow(FeedFilterWindow.GetWindowHandle());
    }

  // ===========================================================================
  
  // AddToListAs(status)
  //   Adds new anime to list with given status.
  //   lParam is a pointer to a CAnime.
  } else if (action == L"AddToListAs") {
    int status = ToINT(body);
    CAnime* pAnimeItem = reinterpret_cast<CAnime*>(lParam);
    if (pAnimeItem) {
      // Set item properties
      pAnimeItem->My_Status = status;
      if (status == MAL_COMPLETED) {
        pAnimeItem->My_WatchedEpisodes = pAnimeItem->Series_Episodes;
        pAnimeItem->SetFinishDate(L"", true);
      }
      // Add item to list
      AnimeList.AddItem(*pAnimeItem);
      AnimeList.Write(pAnimeItem->Series_ID, L"", L"", ANIMELIST_ADDANIME);
      // Refresh
      if (CurrentEpisode.AnimeId == ANIMEID_NOTINLIST) CurrentEpisode.AnimeId = ANIMEID_UNKNOWN;
      MainWindow.RefreshList(status);
      MainWindow.RefreshTabs(status);
      SearchWindow.RefreshList();
      // Add item to event queue
      CEventItem item;
      item.AnimeId = pAnimeItem->Series_ID;
      item.episode = pAnimeItem->My_WatchedEpisodes ? pAnimeItem->My_WatchedEpisodes : -1;
      item.status = status;
      item.Mode = HTTP_MAL_AnimeAdd;
      EventQueue.Add(item);
    }

  // ===========================================================================

  // AddFolder()
  //   Opens up a dialog to add new root folder.
  } else if (action == L"AddFolder") {
    wstring path;
    if (BrowseForFolder(g_hMain, L"Please select a folder:", 
      BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON, path)) {
        Settings.Folders.Root.push_back(path);
        if (Settings.Folders.WatchEnabled) FolderMonitor.Enable();
        ExecuteAction(L"Settings", 0, PAGE_FOLDERS_ROOT);
    }

  // CheckEventBuffer()
  //   Checks for queued events and shows related window.
  } else if (action == L"CheckEventBuffer") {
    EventQueue.Show();

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
    for (int i = 1; i <= AnimeList.Count; i++) {
      if (!AnimeList.Items[i].Folder.empty()) {
        check_folder = false;
        break;
      }
    }
    if (check_folder && !silent && !Settings.Folders.Root.empty()) {
      CTaskDialog dlg(APP_TITLE, TD_ICON_INFORMATION);
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
      for (int i = 1; i <= AnimeList.Count; i++) {
        if (!silent) TaskbarList.SetProgressValue(i, AnimeList.Count);
        switch (AnimeList.Items[i].GetStatus()) {
          case MAL_WATCHING:
          case MAL_ONHOLD:
          case MAL_PLANTOWATCH:
            if (!silent) {
              MainWindow.ChangeStatus(L"Searching... (" + AnimeList.Items[i].Series_Title + L")");
            }
            AnimeList.Items[i].CheckEpisodes(
              Settings.Program.List.ProgressMode == LIST_PROGRESS_AVAILABLEEPS ? -1 : 0, 
              check_folder);
        }
      }
    // Search only for selected list item
    } else {
      AnimeList.Items[AnimeList.Index].CheckEpisodes(
        Settings.Program.List.ProgressMode == LIST_PROGRESS_AVAILABLEEPS ? -1 : 0,
        true);
    }
    // We're done
    if (!silent) {
      TaskbarList.SetProgressState(TBPF_NOPROGRESS);
      MainWindow.ChangeStatus(L"Search finished.");
    }

  // ToggleUpdates()
  //   Enabled or disables list updates.
  } else if (action == L"ToggleUpdates") {
    Taiga.UpdatesEnabled = !Taiga.UpdatesEnabled;
    if (Taiga.UpdatesEnabled) {
      MainWindow.ChangeStatus(L"Updates are now enabled.");
    } else {
      MainWindow.ChangeStatus(L"Updates are now disabled.");
    }

  // ===========================================================================

  // FilterReset()
  //   Resets list filters to their default values.
  } else if (action == L"FilterReset") {
    AnimeList.Filter.Reset();
    FilterWindow.RefreshFilters();
    if (!MainWindow.m_EditSearch.SetText(L"")) {
      MainWindow.RefreshList();
    }

  // FilterStatus(value)
  //   Filters list by status.
  //   Value must be between 1-3.
  } else if (action == L"FilterStatus") {
    int index = ToINT(body) - 1;
    if (index > -1 && index < 3) {
      AnimeList.Filter.Status[index] = !AnimeList.Filter.Status[index];
      FilterWindow.RefreshFilters();
      MainWindow.RefreshList();
    }
  
  // FilterType(value)
  //   Filters list by type.
  //   Value must be between 1-6.
  } else if (action == L"FilterType") {
    int index = ToINT(body) - 1;
    if (index > -1 && index < 6) {
      AnimeList.Filter.Type[index] = !AnimeList.Filter.Type[index];
      FilterWindow.RefreshFilters();
      MainWindow.RefreshList();
    }

  // ===========================================================================

  // AnnounceToHTTP(force)
  //   Sends an HTTP request.
  //   If wParam is set to FALSE, this function sends an empty string.
  //   lParam points to a CEpisode class.
  } else if (action == L"AnnounceToHTTP") {
    if (Settings.Announce.HTTP.Enabled || body == L"true") {
      if (wParam) {
        CEpisode* pEpisode = lParam ? reinterpret_cast<CEpisode*>(lParam) : &CurrentEpisode;
        AnnounceToHTTP(Settings.Announce.HTTP.URL, ReplaceVariables(Settings.Announce.HTTP.Format, *pEpisode, true));
      } else {
        AnnounceToHTTP(Settings.Announce.HTTP.URL, L"");
      }
    }
  
  // AnnounceToMessenger(force)
  //   Changes MSN Messenger status text.
  //   If wParam is set to FALSE, this function sends an empty string.
  //   lParam points to a CEpisode class.
  } else if (action == L"AnnounceToMessenger") {
    if (Settings.Announce.MSN.Enabled || body == L"true") {
      if (wParam) {
        CEpisode* pEpisode = lParam ? reinterpret_cast<CEpisode*>(lParam) : &CurrentEpisode;
        if (pEpisode->AnimeId > 0) {
          AnnounceToMessenger(L"Taiga", L"MyAnimeList", ReplaceVariables(Settings.Announce.MSN.Format, *pEpisode), true);
        }
      } else {
        AnnounceToMessenger(L"", L"", L"", false);
      }
    }
  
  // AnnounceToMIRC(force)
  //   Sends message to specified channels in mIRC.
  //   lParam points to a CEpisode class.
  } else if (action == L"AnnounceToMIRC") {
    if (Settings.Announce.MIRC.Enabled || body == L"true") {
      CEpisode* pEpisode = lParam ? reinterpret_cast<CEpisode*>(lParam) : &CurrentEpisode;
      if (pEpisode->AnimeId > 0) {
        AnnounceToMIRC(Settings.Announce.MIRC.Service, Settings.Announce.MIRC.Channels, 
          ReplaceVariables(Settings.Announce.MIRC.Format, *pEpisode), 
          Settings.Announce.MIRC.Mode, Settings.Announce.MIRC.UseAction, Settings.Announce.MIRC.MultiServer);
      }
    }
  
  // AnnounceToSkype(force)
  //   Changes Skype mood text.
  //   Requires authorization.
  //   If wParam is set to FALSE, this function sends an empty string.
  //   lParam points to a CEpisode class.
  } else if (action == L"AnnounceToSkype") {
    if (Settings.Announce.Skype.Enabled || body == L"true") {
      if (wParam) {
        CEpisode* pEpisode = lParam ? reinterpret_cast<CEpisode*>(lParam) : &CurrentEpisode;
        if (pEpisode->AnimeId > 0) {
          AnnounceToSkype(ReplaceVariables(Settings.Announce.Skype.Format, *pEpisode));
        }
      } else {
        AnnounceToSkype(L"");
      }
    }

  // AnnounceToTwitter(force)
  //   Changes Twitter status.
  //   lParam points to a CEpisode class.
  } else if (action == L"AnnounceToTwitter") {
    if (Settings.Announce.Twitter.Enabled || body == L"true") {
      CEpisode* pEpisode = lParam ? reinterpret_cast<CEpisode*>(lParam) : &CurrentEpisode;
      if (pEpisode->AnimeId > 0) {
        AnnounceToTwitter(ReplaceVariables(Settings.Announce.Twitter.Format, *pEpisode));
      }
    }
  
  // ===========================================================================

  // EditAll()
  //   Shows a dialog to edit details of an anime.
  //   lParam is a pointer to an anime list item.
  } else if (action == L"EditAll") {
    CAnime* anime_item = lParam ? 
      reinterpret_cast<CAnime*>(lParam) : &AnimeList.Items[AnimeList.Index];
    AnimeWindow.Refresh(anime_item);
    AnimeWindow.SetCurrentPage(TAB_MYINFO);
    if (!AnimeWindow.IsWindow()) {
      AnimeWindow.Create(IDD_ANIME_INFO, g_hMain, false);
    } else {
       ActivateWindow(AnimeWindow.GetWindowHandle());
    }

  // EditDelete()
  //   Removes an anime from list.
  } else if (action == L"EditDelete") {
    CTaskDialog dlg;
    dlg.SetWindowTitle(AnimeList.Items[AnimeList.Index].Series_Title.c_str());
    dlg.SetMainIcon(TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Are you sure you want to delete this title from your list?");
    dlg.AddButton(L"Yes", IDYES);
    dlg.AddButton(L"No", IDNO);
    dlg.Show(g_hMain);
    if (dlg.GetSelectedButtonID() == IDYES) {
      CEventItem item;
      item.AnimeId = AnimeList.Items[AnimeList.Index].Series_ID;
      item.Mode = HTTP_MAL_AnimeDelete;
      EventQueue.Add(item);
    }

  // EditEpisode()
  //   Changes watched episode value of an anime.
  } else if (action == L"EditEpisode") {
    CInputDialog dlg;
    dlg.SetNumbers(true, 0, AnimeList.Items[AnimeList.Index].Series_Episodes, 
      AnimeList.Items[AnimeList.Index].GetLastWatchedEpisode());
    dlg.Title = AnimeList.Items[AnimeList.Index].Series_Title;
    dlg.Info = L"Please enter episode number for this title:";
    dlg.Text = ToWSTR(AnimeList.Items[AnimeList.Index].GetLastWatchedEpisode());
    dlg.Show(g_hMain);
    if (dlg.Result == IDOK && MAL.IsValidEpisode(ToINT(dlg.Text), 0, AnimeList.Items[AnimeList.Index].Series_Episodes)) {
      CEpisode episode;
      episode.Number = dlg.Text;
      AnimeList.Items[AnimeList.Index].Update(episode, true);
    }

  // EditScore(value)
  //   Changes anime score.
  //   Value must be between 0-10 and different from current score.
  } else if (action == L"EditScore") {
    CEventItem item;
    item.AnimeId = AnimeList.Items[AnimeList.Index].Series_ID;
    item.score = ToINT(body);
    item.Mode = HTTP_MAL_ScoreUpdate;
    EventQueue.Add(item);

  // EditStatus(value)
  //   Changes anime status of user.
  //   Value must be 1, 2, 3, 4 or 6, and different from current status.
  } else if (action == L"EditStatus") {
    int episode = -1, status = ToINT(body);
    switch (AnimeList.Items[AnimeList.Index].GetAiringStatus()) {
      case MAL_AIRING:
        if (status == MAL_COMPLETED) {
          MessageBox(g_hMain, 
            L"This anime is still airing, you cannot set it as completed.", 
            AnimeList.Items[AnimeList.Index].Series_Title.c_str(), MB_ICONERROR);
          return;
        }
        break;
      case MAL_FINISHED:
        break;
      case MAL_NOTYETAIRED:
        if (status != MAL_PLANTOWATCH) {
          MessageBox(g_hMain, 
            L"This anime has not aired yet, you cannot set it as anything but Plan to Watch.", 
            AnimeList.Items[AnimeList.Index].Series_Title.c_str(), MB_ICONERROR);
          return;
        }
        break;
      default:
        return;
    }
    switch (status) {
      case MAL_COMPLETED:
        AnimeList.Items[AnimeList.Index].SetFinishDate(L"", false);
        episode = AnimeList.Items[AnimeList.Index].Series_Episodes;
        if (episode == 0) episode = -1;
        break;
    }
    CEventItem item;
    item.AnimeId = AnimeList.Items[AnimeList.Index].Series_ID;
    item.episode = episode;
    item.status = status;
    item.Mode = episode == -1 ? HTTP_MAL_StatusUpdate : HTTP_MAL_AnimeEdit;
    EventQueue.Add(item);

  // EditTags(tags)
  //   Changes anime tags.
  //   Tags must be separated by a comma.
  } else if (action == L"EditTags") {
    CInputDialog dlg;
    dlg.Title = AnimeList.Items[AnimeList.Index].Series_Title;
    dlg.Info = L"Please enter tags for this title, separated by a comma:";
    dlg.Text = AnimeList.Items[AnimeList.Index].GetTags();
    dlg.Show(g_hMain);
    if (dlg.Result == IDOK) {
      CEventItem item;
      item.AnimeId = AnimeList.Items[AnimeList.Index].Series_ID;
      item.tags = dlg.Text;
      item.Mode = HTTP_MAL_TagUpdate;
      EventQueue.Add(item);
    }

  // EditTitles(titles)
  //   Changes alternative titles of an anime.
  //   Titles must be separated by "; ".
  } else if (action == L"EditTitles") {
    CInputDialog dlg;
    dlg.Title = AnimeList.Items[AnimeList.Index].Series_Title;
    dlg.Info = L"Please enter alternative titles, separated by a semicolon:";
    dlg.Text = AnimeList.Items[AnimeList.Index].Synonyms;
    dlg.Show(g_hMain);
    if (dlg.Result == IDOK) {
      AnimeList.Items[AnimeList.Index].SetLocalData(EMPTY_STR, dlg.Text);
    }
  
  // ===========================================================================
  
  // OpenFolder()
  //   Searches for anime folder and opens it.
  } else if (action == L"OpenFolder") {
    if (AnimeList.Items[AnimeList.Index].Folder.empty()) {
      MainWindow.ChangeStatus(L"Searching for folder...");
      AnimeList.Items[AnimeList.Index].CheckFolder();
      if (AnimeList.Items[AnimeList.Index].Folder.empty()) {
        MainWindow.ChangeStatus(L"Folder not found.");
        return;
      } else {
        MainWindow.ChangeStatus(L"Folder found.");
      }
    }
    Execute(AnimeList.Items[AnimeList.Index].Folder);

  // SetFolder()
  //   Lets user set an anime folder.
  } else if (action == L"SetFolder") {
    wstring path, title = L"Anime title: " + AnimeList.Items[AnimeList.Index].Series_Title;
    if (BrowseForFolder(MainWindow.GetWindowHandle(), title.c_str(), 
      BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON, path)) {
        AnimeList.Items[AnimeList.Index].SetFolder(path, true, true);
    }

  // ===========================================================================

  // PlayEpisode(value)
  //   Searches for an episode of an anime and plays it.
  } else if (action == L"PlayEpisode") {
    int number = ToINT(body);
    AnimeList.Items[AnimeList.Index].PlayEpisode(number);
  
  // PlayLast()
  //   Searches for the last watched episode of an anime and plays it.
  } else if (action == L"PlayLast") {
    int number = AnimeList.Items[AnimeList.Index].GetLastWatchedEpisode();
    AnimeList.Items[AnimeList.Index].PlayEpisode(number);
  
  // PlayNext()
  //   Searches for the next episode of an anime and plays it.
  } else if (action == L"PlayNext") {
    int number = 1;
    if (AnimeList.Items[AnimeList.Index].Series_Episodes != 1) {
      number = AnimeList.Items[AnimeList.Index].GetLastWatchedEpisode() + 1;
    }
    AnimeList.Items[AnimeList.Index].PlayEpisode(number);
  
  // PlayRandom()
  //   Searches for a random episode of an anime and plays it.
  } else if (action == L"PlayRandom") {
    int anime_id = ToINT(body);
    CAnime* anime = anime_id ? AnimeList.FindItem(anime_id) : &AnimeList.Items[AnimeList.Index];
    if (anime) {
      int total = anime->GetTotalEpisodes();
      if (total == 0) total = anime->GetLastWatchedEpisode() + 1;
      for (int i = 0; i < total; i++) {
        srand(static_cast<unsigned int>(GetTickCount()));
        int episode = rand() % total + 1;
        anime->CheckFolder();
        wstring file = SearchFileFolder(*anime, anime->Folder, episode, false);
        if (!file.empty()) {
          Execute(file);
          break;
        }
      }
    }
  
  // PlayRandomAnime()
  //   Searches for a random episode of a random anime and plays it.
  } else if (action == L"PlayRandomAnime") {
    for (int i = 0; i < AnimeList.Count; i++) {
      srand(static_cast<unsigned int>(GetTickCount()));
      int anime_index = rand() % AnimeList.Count + 1;
      CAnime& anime = AnimeList.Items[anime_index];
      int total = anime.GetTotalEpisodes();
      if (total == 0) total = anime.GetLastWatchedEpisode() + 1;
      int episode = rand() % total + 1;
      anime.CheckFolder();
      wstring file = SearchFileFolder(anime, anime.Folder, episode, false);
      if (!file.empty()) {
        Execute(file);
        break;
      }
    }

  // ===========================================================================

  // Season_Load(file)
  //   Loads season data.
  } else if (action == L"Season_Load") {
    if (SeasonDatabase.Read(body)) {
      SeasonWindow.RefreshData(false);
      SeasonWindow.RefreshList();
    }

  // Season_GroupBy(group)
  //   Groups season data.
  } else if (action == L"Season_GroupBy") {
    SeasonWindow.GroupBy = ToINT(body);
    SeasonWindow.RefreshList();
    SeasonWindow.RefreshToolbar();

  // Season_SortBy(sort)
  //   Sorts season data.
  } else if (action == L"Season_SortBy") {
    SeasonWindow.SortBy = ToINT(body);
    SeasonWindow.RefreshList();
    SeasonWindow.RefreshToolbar();
  }
}