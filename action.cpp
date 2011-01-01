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
#include "dlg/dlg_settings.h"
#include "dlg/dlg_test_recognition.h"
#include "dlg/dlg_torrent.h"
#include "dlg/dlg_torrent_filter.h"
#include "event.h"
#include "http.h"
#include "myanimelist.h"
#include "process.h"
#include "resource.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "theme.h"
#include "win32/win_taskbar.h"
#include "win32/win_taskdialog.h"

// =============================================================================

void ExecuteAction(wstring action, WPARAM wParam, LPARAM lParam) {
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
      MainWindow.m_Toolbar.EnableButton(0, !result);
      MainWindow.m_Toolbar.EnableButton(1, !result);
      if (!result) MainWindow.ChangeStatus();
    } else {
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
    MainWindow.ChangeStatus(L"Refreshing list...");
    bool result = MAL.GetList(wParam == TRUE);
    MainWindow.m_Toolbar.EnableButton(0, !result);
    MainWindow.m_Toolbar.EnableButton(1, !result);
    if (!result) MainWindow.ChangeStatus();

  // RefreshLogin()
  //   Retrieves anime list and logs in afterwards.
  } else if (action == L"RefreshLogin") {
    ExecuteAction(L"RefreshList", TRUE);

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
    wstring title = AnimeList.Item[AnimeList.Index].Series_Title;
    ReplaceChars(title, L"_!?.,:;~+", L"");
    Replace(title, L" -", L"");
    Replace(body, L"%title%", title);
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
    Taiga.CheckNewVersion(false);

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
      reinterpret_cast<CAnime*>(lParam) : &AnimeList.Item[AnimeList.Index];
    AnimeWindow.Refresh(anime_item);
    AnimeWindow.SetCurrentPage(TAB_SERIESINFO);
    if (!AnimeWindow.IsWindow()) {
      AnimeWindow.Create(IDD_ANIME_INFO, g_hMain, false);
    } else {
      ActivateWindow(AnimeWindow.GetWindowHandle());
    }

  // RecognitionTest()
  //   Shows recognition test window.
  } else if (action == L"RecognitionTest") {
    if (!RecognitionTestWindow.IsWindow()) {
      RecognitionTestWindow.Create(IDD_TEST_RECOGNITION, NULL, false);
    } else {
      ActivateWindow(RecognitionTestWindow.GetWindowHandle());
    }

  // SetSearchMode()
  //   Changes search bar mode.
  } else if (action == L"SetSearchMode") {
    if (body == L"List") {
      MainWindow.SetSearchMode(SEARCH_MODE_LIST);
    } else if (body == L"MAL") {
      MainWindow.SetSearchMode(SEARCH_MODE_MAL);
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
  
  // SearchAnime()
  } else if (action == L"SearchAnime") {
    if (body.empty()) return;
    if (Settings.Account.MAL.API == MAL_API_OFFICIAL) {
      if (Settings.Account.MAL.User.empty() || Settings.Account.MAL.Password.empty()) {
        CTaskDialog dlg;
        dlg.SetWindowTitle(APP_TITLE);
        dlg.SetMainIcon(TD_INFORMATION_ICON);
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
    CEpisode episode;
    episode.Index = AnimeList.Index;
    Torrents.Check(ReplaceVariables(body, episode));
    ExecuteAction(L"Torrents");

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
    if (!TorrentFilterWindow.IsWindow()) {
      TorrentFilterWindow.Create(IDD_ADDFILTER, 
        reinterpret_cast<HWND>(lParam), wParam != FALSE);
    } else {
      ActivateWindow(TorrentFilterWindow.GetWindowHandle());
    }

  // ===========================================================================
  
  // AddToListAs(status)
  //   Adds new anime to list with given status.
  //   lParam is a pointer to a CAnime.
  } else if (action == L"AddToListAs") {
    int status = ToINT(body);
    CAnime* pAnimeItem = reinterpret_cast<CAnime*>(lParam);
    if (pAnimeItem) {
      pAnimeItem->My_Status = status;
      if (status == MAL_COMPLETED) {
        pAnimeItem->My_WatchedEpisodes = pAnimeItem->Series_Episodes;
        pAnimeItem->SetFinishDate(L"", true);
      }
      AnimeList.AddItem(*pAnimeItem);
      AnimeList.User.IncreaseItemCount(status, 1);
      AnimeList.Write(AnimeList.Count, L"", L"", ANIMELIST_ADDANIME);
      if (CurrentEpisode.Index == -1) CurrentEpisode.Index = 0;
      MainWindow.RefreshList(pAnimeItem->My_Status);
      MainWindow.RefreshTabs(pAnimeItem->My_Status);
      SearchWindow.RefreshList();
      EventQueue.Add(L"", AnimeList.Count, pAnimeItem->Series_ID, 
        pAnimeItem->My_WatchedEpisodes ? pAnimeItem->My_WatchedEpisodes : -1, 
        -1, status, EMPTY_STR, L"", HTTP_MAL_AnimeAdd);
    }

  // ===========================================================================

  // AddFolder()
  //   Opens up a dialog to add new root folder.
  } else if (action == L"AddFolder") {
    wstring path;
    if (BrowseForFolder(g_hMain, L"Please select a folder:", 
      BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON, path)) {
        Settings.Folders.Root.push_back(path);
        ExecuteAction(L"Settings", 0, PAGE_FOLDERS_ROOT);
    }

  // CheckEventBuffer()
  //   Checks for queued events and shows related window.
  } else if (action == L"CheckEventBuffer") {
    EventQueue.Show();

  // CheckNewEpisodes()
  //   Checks new episodes.
  //   wParam is a BOOL value that activates silent operation mode.
  } else if (action == L"CheckNewEpisodes") {
    bool check_folder = true;
    bool silent = wParam == TRUE;
    for (int i = 1; i <= AnimeList.Count; i++) {
      if (!AnimeList.Item[i].Folder.empty()) {
        check_folder = false;
        break;
      }
    }
    if (check_folder && !silent && !Settings.Folders.Root.empty()) {
      CTaskDialog dlg;
      dlg.SetWindowTitle(APP_TITLE);
      dlg.SetMainIcon(TD_ICON_INFORMATION);
      dlg.SetMainInstruction(L"Would you like to search for anime folders first?");
      dlg.SetContent(L"This feature only checks specific anime folders for new episodes. "
        L"As you have none set at the moment, searching for folders is highly recommended.");
      dlg.AddButton(L"Yes", IDYES);
      dlg.AddButton(L"No", IDNO);
      dlg.Show(g_hMain);
      check_folder = dlg.GetSelectedButtonID() == IDYES ? true : false;
    }
    if (!silent) TaskbarList.SetProgressState(TBPF_NORMAL);
    for (int i = 1; i <= AnimeList.Count; i++) {
      if (!silent) TaskbarList.SetProgressValue(i, AnimeList.Count);
      switch (AnimeList.Item[i].GetStatus()) {
        case MAL_WATCHING:
        case MAL_ONHOLD:
        case MAL_PLANTOWATCH:
          if (!silent) {
            MainWindow.ChangeStatus(L"Searching... (" + 
              AnimeList.Item[i].Series_Title + L")");
          }
          AnimeList.Item[i].CheckNewEpisode(check_folder);
      }
    }
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
        if (pEpisode->Index > 0) {
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
      if (pEpisode->Index > 0) {
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
        if (pEpisode->Index > 0) {
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
      if (pEpisode->Index > 0) {
        AnnounceToTwitter(ReplaceVariables(Settings.Announce.Twitter.Format, *pEpisode));
      }
    }
  
  // ===========================================================================

  // EditAll()
  //   Shows a dialog to edit details of an anime.
  //   lParam is a pointer to an anime list item.
  } else if (action == L"EditAll") {
    CAnime* anime_item = lParam ? 
      reinterpret_cast<CAnime*>(lParam) : &AnimeList.Item[AnimeList.Index];
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
    dlg.SetWindowTitle(AnimeList.Item[AnimeList.Index].Series_Title.c_str());
    dlg.SetMainIcon(TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Are you sure you want to delete this title from your list?");
    dlg.AddButton(L"Yes", IDYES);
    dlg.AddButton(L"No", IDNO);
    dlg.Show(g_hMain);
    if (dlg.GetSelectedButtonID() == IDYES) {
      EventQueue.Add(L"", AnimeList.Index, AnimeList.Item[AnimeList.Index].Series_ID, 
        -1, -1, -1, EMPTY_STR, L"", HTTP_MAL_AnimeDelete);
    }

  // EditEpisode()
  //   Changes watched episode value of an anime.
  } else if (action == L"EditEpisode") {
    CInputDialog dlg;
    dlg.SetNumbers(true, 0, AnimeList.Item[AnimeList.Index].Series_Episodes, 
      AnimeList.Item[AnimeList.Index].GetLastWatchedEpisode());
    dlg.Title = AnimeList.Item[AnimeList.Index].Series_Title;
    dlg.Info = L"Please enter episode number for this title:";
    dlg.Text = ToWSTR(AnimeList.Item[AnimeList.Index].GetLastWatchedEpisode());
    dlg.Show(g_hMain);
    if (dlg.Result == IDOK && MAL.IsValidEpisode(ToINT(dlg.Text), 0, AnimeList.Item[AnimeList.Index].Series_Episodes)) {
      CEpisode episode;
      episode.Number = dlg.Text;
      AnimeList.Item[AnimeList.Index].Update(episode, true);
    }

  // EditScore(value)
  //   Changes anime score.
  //   Value must be between 0-10 and different from current score.
  } else if (action == L"EditScore") {
    int score = ToINT(body);
    EventQueue.Add(L"", AnimeList.Index, AnimeList.Item[AnimeList.Index].Series_ID, 
      -1, score, -1, EMPTY_STR, L"", HTTP_MAL_ScoreUpdate);

  // EditStatus(value)
  //   Changes anime status of user.
  //   Value must be 1, 2, 3, 4 or 6, and different from current status.
  } else if (action == L"EditStatus") {
    int episode = -1, status = ToINT(body);
    switch (AnimeList.Item[AnimeList.Index].Series_Status) {
      case MAL_AIRING:
        if (status == MAL_COMPLETED) {
          MessageBox(g_hMain, 
            L"This anime is still airing, you cannot set it as completed.", 
            AnimeList.Item[AnimeList.Index].Series_Title.c_str(), MB_ICONERROR);
          return;
        }
        break;
      case MAL_FINISHED:
        break;
      case MAL_NOTYETAIRED:
        if (status != MAL_PLANTOWATCH) {
          MessageBox(g_hMain, 
            L"This anime has not aired yet, you cannot set it as anything but Plan to Watch.", 
            AnimeList.Item[AnimeList.Index].Series_Title.c_str(), MB_ICONERROR);
          return;
        }
        break;
      default:
        return;
    }
    switch (status) {
      case MAL_COMPLETED:
        AnimeList.Item[AnimeList.Index].SetFinishDate(L"", false);
        episode = AnimeList.Item[AnimeList.Index].Series_Episodes;
        if (episode == 0) episode = -1;
        break;
    }
    EventQueue.Add(L"", AnimeList.Index, AnimeList.Item[AnimeList.Index].Series_ID, 
      episode, -1, status, EMPTY_STR, L"", episode == -1 ? HTTP_MAL_StatusUpdate : HTTP_MAL_AnimeEdit);

  // EditTags(tags)
  //   Changes anime tags.
  //   Tags must be seperated by a comma.
  } else if (action == L"EditTags") {
    CInputDialog dlg;
    dlg.Title = AnimeList.Item[AnimeList.Index].Series_Title;
    dlg.Info = L"Please enter tags for this title, seperated by a comma:";
    dlg.Text = AnimeList.Item[AnimeList.Index].GetTags();
    dlg.Show(g_hMain);
    if (dlg.Result == IDOK) {
      EventQueue.Add(L"", AnimeList.Index, AnimeList.Item[AnimeList.Index].Series_ID, 
        -1, -1, -1, dlg.Text, L"", HTTP_MAL_TagUpdate);
    }

  // EditTitles(titles)
  //   Changes alternative titles of an anime.
  //   Titles must be seperated by "; ".
  } else if (action == L"EditTitles") {
    CInputDialog dlg;
    dlg.Title = AnimeList.Item[AnimeList.Index].Series_Title;
    dlg.Info  = L"Please enter alternative titles, seperated by a semicolon:";
    dlg.Text  = AnimeList.Item[AnimeList.Index].Synonyms;
    dlg.Show(g_hMain);
    if (dlg.Result == IDOK) {
      AnimeList.Item[AnimeList.Index].Synonyms = dlg.Text;
      Settings.Anime.SetItem(AnimeList.Item[AnimeList.Index].Series_ID, EMPTY_STR, dlg.Text);
      if (CurrentEpisode.Index == -1) CurrentEpisode.Index = 0;
    }
  
  // ===========================================================================
  
  // OpenFolder()
  //   Searches for anime folder and opens it.
  } else if (action == L"OpenFolder") {
    if (AnimeList.Item[AnimeList.Index].Folder.empty()) {
      MainWindow.ChangeStatus(L"Searching for folder...");
      AnimeList.Item[AnimeList.Index].CheckFolder();
      if (AnimeList.Item[AnimeList.Index].Folder.empty()) {
        MainWindow.ChangeStatus(L"Folder not found.");
        return;
      } else {
        MainWindow.ChangeStatus(L"Folder found.");
      }
    }
    Execute(AnimeList.Item[AnimeList.Index].Folder);

  // PlayEpisode(value)
  //   Searches for an episode of an anime and plays it.
  } else if (action == L"PlayEpisode") {
    int number = ToINT(body);
    wstring file = SearchFileFolder(AnimeList.Index, AnimeList.Item[AnimeList.Index].Folder, number, false);
    if (file.empty()) {
      MainWindow.ChangeStatus(L"Could not find episode #" + ToWSTR(number) + L".");
    } else {
      Execute(file);
    }
  
  // PlayLast()
  //   Searches for the last watched episode of an anime and plays it.
  } else if (action == L"PlayLast") {
    int number = AnimeList.Item[AnimeList.Index].GetLastWatchedEpisode();
    wstring file = SearchFileFolder(AnimeList.Index, AnimeList.Item[AnimeList.Index].Folder, number, false);
    if (file.empty()) {
      if (number == 0) number = 1;
      MainWindow.ChangeStatus(L"Could not find episode #" + ToWSTR(number) + L".");
    } else {
      Execute(file);
    }
  
  // PlayNext()
  //   Searches for the next episode of an anime and plays it.
  } else if (action == L"PlayNext") {
    int number = 0;
    if (AnimeList.Item[AnimeList.Index].Series_Episodes != 1) {
      number = AnimeList.Item[AnimeList.Index].GetLastWatchedEpisode() + 1;
    }
    wstring file = SearchFileFolder(AnimeList.Index, AnimeList.Item[AnimeList.Index].Folder, number, false);
    if (file.empty()) {
      if (number == 0) number = 1;
      MainWindow.ChangeStatus(L"Could not find episode #" + ToWSTR(number) + L".");
    } else {
      Execute(file);
    }
  
  // PlayRandom()
  //   Searches for a random episode of an anime and plays it.
  } else if (action == L"PlayRandom") {
    int anime_index = ToINT(body);
    if (anime_index == 0) anime_index = AnimeList.Index;
    if (anime_index > 0) {
      if (anime_index <= AnimeList.Count) {
        int total = AnimeList.Item[anime_index].GetTotalEpisodes();
        if (total == 0) total = AnimeList.Item[anime_index].GetLastWatchedEpisode() + 1;
        for (int i = 0; i < total; i++) {
          srand(static_cast<unsigned int>(GetTickCount()));
          int episode = rand() % total + 1;
          AnimeList.Item[anime_index].CheckFolder();
          wstring file = SearchFileFolder(anime_index, AnimeList.Item[anime_index].Folder, episode, false);
          if (!file.empty()) {
            Execute(file);
            break;
          }
        }
      }
    }
  
  // PlayRandomAnime()
  //   Searches for a random episode of a random anime and plays it.
  } else if (action == L"PlayRandomAnime") {
    for (int i = 0; i < AnimeList.Count; i++) {
      int anime_index = rand() % AnimeList.Count + 1;
      int total = AnimeList.Item[anime_index].GetTotalEpisodes();
      if (total == 0) total = AnimeList.Item[anime_index].GetLastWatchedEpisode() + 1;
      srand(static_cast<unsigned int>(GetTickCount()));
      int episode = rand() % total + 1;
      AnimeList.Item[anime_index].CheckFolder();
      wstring file = SearchFileFolder(anime_index, AnimeList.Item[anime_index].Folder, episode, false);
      if (!file.empty()) {
        Execute(file);
        break;
      }
    }
  }
}