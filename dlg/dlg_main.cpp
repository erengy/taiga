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

#include "../std.h"
#include <algorithm>

#include "dlg_main.h"

#include "dlg_anime_info.h"
#include "dlg_anime_list.h"
#include "dlg_history.h"
#include "dlg_search.h"
#include "dlg_season.h"
#include "dlg_settings.h"
#include "dlg_stats.h"
#include "dlg_test_recognition.h"
#include "dlg_torrent.h"

#include "../anime_db.h"
#include "../anime_filter.h"
#include "../announce.h"
#include "../common.h"
#include "../debug.h"
#include "../gfx.h"
#include "../history.h"
#include "../http.h"
#include "../media.h"
#include "../monitor.h"
#include "../myanimelist.h"
#include "../process.h"
#include "../recognition.h"
#include "../resource.h"
#include "../settings.h"
#include "../stats.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"

#include "../win32/win_gdi.h"
#include "../win32/win_taskbar.h"
#include "../win32/win_taskdialog.h"

class MainDialog MainDialog;

// =============================================================================

MainDialog::MainDialog() {
  navigation.parent = this;
  search_bar.parent = this;
  
  RegisterDlgClass(L"TaigaMainW");
}

BOOL MainDialog::OnInitDialog() {
  // Set global variables
  g_hMain = GetWindowHandle();
  
  // Initialize window properties
  InitWindowPosition();
  SetIconLarge(IDI_MAIN);
  SetIconSmall(IDI_MAIN);

  // Create default fonts
  UI.CreateFonts(GetDC());
  
  // Create controls
  CreateDialogControls();

  // Select default content page
  navigation.SetCurrentPage(SIDEBAR_ITEM_ANIMELIST);

  // Start process timer
  SetTimer(g_hMain, TIMER_MAIN, 1000, nullptr);
  
  // Add icon to taskbar
  Taskbar.Create(g_hMain, nullptr, APP_TITLE);

  ChangeStatus();
  UpdateTip();
  UpdateTitle();
  
  // Refresh menus
  UpdateAllMenus();

  // Apply start-up settings
  if (Settings.Program.StartUp.check_new_episodes) {
    ExecuteAction(L"CheckEpisodes()", TRUE);
  }
  if (!Settings.Program.StartUp.minimize) {
    Show(Settings.Program.Exit.remember_pos_size && Settings.Program.Position.maximized ? 
      SW_MAXIMIZE : SW_SHOWNORMAL);
  }
  if (Settings.Account.MAL.user.empty()) {
    win32::TaskDialog dlg(APP_TITLE, TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Welcome to Taiga!");
    dlg.SetContent(L"User name is not set. Would you like to open settings window to set it now?");
    dlg.AddButton(L"Yes", IDYES);
    dlg.AddButton(L"No", IDNO);
    dlg.Show(g_hMain);
    if (dlg.GetSelectedButtonID() == IDYES) {
      ExecuteAction(L"Settings", 0, PAGE_ACCOUNT);
    }
  }
  if (Settings.Folders.watch_enabled) {
    FolderMonitor.SetWindowHandle(GetWindowHandle());
    FolderMonitor.Enable();
  }

  // Success
  return TRUE;
}

void MainDialog::CreateDialogControls() {
  // Create rebar
  rebar.Attach(GetDlgItem(IDC_REBAR_MAIN));
  // Create menu toolbar
  toolbar_menu.Attach(GetDlgItem(IDC_TOOLBAR_MENU));
  toolbar_menu.SetImageList(nullptr, 0, 0);
  // Create main toolbar
  toolbar_main.Attach(GetDlgItem(IDC_TOOLBAR_MAIN));
  toolbar_main.SetImageList(UI.ImgList24.GetHandle(), 24, 24);
  toolbar_main.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  // Create search toolbar
  toolbar_search.Attach(GetDlgItem(IDC_TOOLBAR_SEARCH));
  toolbar_search.SetImageList(UI.ImgList16.GetHandle(), 16, 16);
  toolbar_search.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  // Create search text
  edit.Attach(GetDlgItem(IDC_EDIT_SEARCH));
  edit.SetCueBannerText(L"Search list");
  edit.SetParent(toolbar_search.GetWindowHandle());
  edit.SetPosition(nullptr, 0, 1, 200, 20);
  edit.SetMargins(1, 16);
  win32::Rect rcEdit; edit.GetRect(&rcEdit);
  // Create cancel search button
  cancel_button.Attach(GetDlgItem(IDC_BUTTON_CANCELSEARCH));
  cancel_button.SetParent(edit.GetWindowHandle());
  cancel_button.SetPosition(nullptr, rcEdit.right + 1, 0, 16, 16);
  // Create treeview control
  treeview.Attach(GetDlgItem(IDC_TREE_MAIN));
  treeview.SendMessage(TVM_SETBKCOLOR, 0, ::GetSysColor(COLOR_3DFACE));
  treeview.SetImageList(UI.ImgList16.GetHandle());
  treeview.SetItemHeight(20);
  treeview.SetTheme();
  // Create status bar
  statusbar.Attach(GetDlgItem(IDC_STATUSBAR_MAIN));
  statusbar.SetImageList(UI.ImgList16.GetHandle());
  statusbar.InsertPart(-1, 0, 0, 900, nullptr, nullptr);
  statusbar.InsertPart(ICON16_CLOCK, 0, 0,  32, nullptr, nullptr);

  // Insert treeview items
  treeview.hti.push_back(treeview.InsertItem(L"Now Playing", ICON16_PLAY, SIDEBAR_ITEM_NOWPLAYING, nullptr));
  treeview.hti.push_back(treeview.InsertItem(nullptr, -1, -1, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Anime List", ICON16_DOCUMENT_A, SIDEBAR_ITEM_ANIMELIST, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"History", ICON16_CLOCK, SIDEBAR_ITEM_HISTORY, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Statistics", ICON16_CHART, SIDEBAR_ITEM_STATS, nullptr));
  treeview.hti.push_back(treeview.InsertItem(nullptr, -1, -1, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Search", ICON16_SEARCH, SIDEBAR_ITEM_SEARCH, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Seasons", ICON16_CALENDAR, SIDEBAR_ITEM_SEASONS, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Torrents", ICON16_FEED, SIDEBAR_ITEM_FEEDS, nullptr));
  if (History.queue.GetItemCount() > 0) {
    treeview.RefreshHistoryCounter();
  }

  // Insert menu toolbar buttons
  BYTE fsStyle0 = BTNS_AUTOSIZE | BTNS_DROPDOWN | BTNS_SHOWTEXT;
  toolbar_menu.InsertButton(0, I_IMAGENONE, 100, 1, fsStyle0, 0, L"  File", nullptr);
  toolbar_menu.InsertButton(1, I_IMAGENONE, 101, 1, fsStyle0, 0, L"  MyAnimeList", nullptr);
  toolbar_menu.InsertButton(2, I_IMAGENONE, 102, 1, fsStyle0, 0, L"  Tools", nullptr);
  toolbar_menu.InsertButton(3, I_IMAGENONE, 103, 1, fsStyle0, 0, L"  View", nullptr);
  toolbar_menu.InsertButton(4, I_IMAGENONE, 104, 1, fsStyle0, 0, L"  Help", nullptr);
  // Insert main toolbar buttons
  BYTE fsStyle1 = BTNS_AUTOSIZE;
  BYTE fsStyle2 = BTNS_AUTOSIZE | BTNS_WHOLEDROPDOWN;
  toolbar_main.InsertButton(0, ICON24_SYNC, TOOLBAR_BUTTON_SYNCHRONIZE, 
                            1, fsStyle1, 0, nullptr, L"Synchronize list");
  toolbar_main.InsertButton(1, ICON24_MAL, TOOLBAR_BUTTON_MAL, 
                            1, fsStyle1, 1, nullptr, L"View your panel at MyAnimeList");
  toolbar_main.InsertButton(2, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_main.InsertButton(3, ICON24_FOLDERS, TOOLBAR_BUTTON_FOLDERS, 
                            1, fsStyle2, 3, nullptr, L"Root folders");
  toolbar_main.InsertButton(4, ICON24_TOOLS, TOOLBAR_BUTTON_TOOLS, 
                            1, fsStyle2, 4, nullptr, L"External services");
  toolbar_main.InsertButton(5, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_main.InsertButton(6, ICON24_SETTINGS, TOOLBAR_BUTTON_SETTINGS, 
                            1, fsStyle1, 6, nullptr, L"Change program settings");
#ifdef _DEBUG
  toolbar_main.InsertButton(7, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_main.InsertButton(8, ICON24_ABOUT, TOOLBAR_BUTTON_ABOUT, 
                            1, fsStyle1, 8, nullptr, L"Debug");
#endif

  // Insert rebar bands
  UINT fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_HEADERSIZE | RBBIM_SIZE | RBBIM_STYLE;
  UINT fStyle = RBBS_NOGRIPPER;
  rebar.InsertBand(toolbar_menu.GetWindowHandle(), 
    GetSystemMetrics(SM_CXSCREEN), 
    0, 0, 0, 0, 0, 0,
    HIWORD(toolbar_menu.GetButtonSize()), 
    fMask, fStyle);
  rebar.InsertBand(toolbar_main.GetWindowHandle(), 
    GetSystemMetrics(SM_CXSCREEN), 
    WIN_CONTROL_MARGIN, 0, 0, 0, 0, 0, 
    HIWORD(toolbar_main.GetButtonSize()) + 2, 
    fMask, fStyle | RBBS_BREAK);
  rebar.InsertBand(toolbar_search.GetWindowHandle(), 
    0, WIN_CONTROL_MARGIN, 0, 208, 0, 0, 0, 
    HIWORD(toolbar_search.GetButtonSize()), 
    fMask, fStyle);
}

void MainDialog::InitWindowPosition() {
  UINT flags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER;
  const LONG min_w = ScaleX(786);
  const LONG min_h = ScaleX(568);
  
  win32::Rect rcParent, rcWindow;
  ::GetWindowRect(GetParent(), &rcParent);
  rcWindow.Set(
    Settings.Program.Position.x, 
    Settings.Program.Position.y, 
    Settings.Program.Position.x + Settings.Program.Position.w,
    Settings.Program.Position.y + Settings.Program.Position.h);

  if (rcWindow.left < 0 || rcWindow.left >= rcParent.right || 
      rcWindow.top < 0 || rcWindow.top >= rcParent.bottom) {
        flags |= SWP_NOMOVE;
  }
  if (rcWindow.Width() < min_w) {
    rcWindow.right = rcWindow.left + min_w;
  }
  if (rcWindow.Height() < min_h) {
    rcWindow.bottom = rcWindow.top + min_h;
  }
  if (rcWindow.Width() > rcParent.Width()) {
    rcWindow.right = rcParent.left + rcParent.Width();
  }
  if (rcWindow.Height() > rcParent.Height()) {
    rcWindow.bottom = rcParent.top + rcParent.Height();
  }
  if (rcWindow.Width() > 0 && rcWindow.Height() > 0 && 
    Settings.Program.Position.maximized == FALSE &&
    Settings.Program.Exit.remember_pos_size == TRUE) {
      SetPosition(nullptr, rcWindow, flags);
      if (flags & SWP_NOMOVE) {
        CenterOwner();
      }
  }

  SetSizeMin(min_w, min_h);
  SetSnapGap(10);
}

// =============================================================================

INT_PTR MainDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Log off / Shutdown
    case WM_ENDSESSION: {
      OnDestroy();
      return FALSE;
    }

    // Monitor anime folders
    case WM_MONITORCALLBACK: {
      FolderMonitor.OnChange(reinterpret_cast<FolderInfo*>(lParam));
      return TRUE;
    }

    // Show menu
    case WM_TAIGA_SHOWMENU: {
      toolbar_wm.ShowMenu();
      return TRUE;
    }

    // External programs
    case WM_COPYDATA: {
      PCOPYDATASTRUCT pCDS = (PCOPYDATASTRUCT)lParam;
      // Skype
      if (reinterpret_cast<HWND>(wParam) == Skype.api_window_handle) {
        return TRUE; // pCDS->lpData is the response

      // JetAudio
      } else if (pCDS->dwData == 0x3000 /* JRC_COPYDATA_ID_TRACK_FILENAME */) {
        MediaPlayers.new_title = ToUTF8(reinterpret_cast<LPCSTR>(pCDS->lpData));
        return TRUE;

      // Media Portal
      } else if (pCDS->dwData == 0x1337) {
        MediaPlayers.new_title = ToUTF8(reinterpret_cast<LPCSTR>(pCDS->lpData));
        return TRUE;
      }
      break;
    }
    default: {
      // Skype
      if (uMsg == Skype.control_api_attach) {
        switch (lParam) {
          case 0: // ATTACH_SUCCESS
#ifdef _DEBUG
            ChangeStatus(L"Skype attach succeeded.");
#endif
            Skype.api_window_handle = reinterpret_cast<HWND>(wParam);
            Skype.ChangeMood();
            return TRUE;
          case 1: // ATTACH_PENDING_AUTHORIZATION
#ifdef _DEBUG
            ChangeStatus(L"Skype is pending authorization...");
#endif
            return TRUE;
        }
      }
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL MainDialog::PreTranslateMessage(MSG* pMsg) {
  switch (pMsg->message) {
    case WM_KEYDOWN: {
      if (::GetFocus() == edit.GetWindowHandle()) {
        switch (pMsg->wParam) {
          // Clear search text
          case VK_ESCAPE: {
            edit.SetText(L"");
            return TRUE;
          }
          // Search
          case VK_RETURN: {
            wstring text;
            edit.GetText(text);
            if (text.empty()) break;
            switch (search_bar.mode) {
              case SEARCH_MODE_MAL: {
                switch (Settings.Account.MAL.api) {
                  case MAL_API_OFFICIAL: {
                    ExecuteAction(L"SearchAnime(" + text + L")");
                    return TRUE;
                  }
                  case MAL_API_NONE: {
                    mal::ViewAnimeSearch(text); // TEMP
                    return TRUE;
                  }
                }
                break;
              }
              case SEARCH_MODE_FEED: {
                TorrentDialog.Search(Settings.RSS.Torrent.search_url, text);
                return TRUE;
              }
            }
            break;
          }
        }
      }
      break;
    }

    // Forward mouse wheel messages to the active page
    case WM_MOUSEWHEEL: {
      // Ignoring the low-order word of wParam to avoid falling into an infinite
      // message-forwarding loop
      WPARAM wParam = MAKEWPARAM(0, HIWORD(pMsg->wParam));
      switch (navigation.GetCurrentPage()) {
        case SIDEBAR_ITEM_ANIMELIST:
          return AnimeListDialog.SendMessage(
            pMsg->message, wParam, pMsg->lParam);
        case SIDEBAR_ITEM_HISTORY:
          return HistoryDialog.SendMessage(
            pMsg->message, wParam, pMsg->lParam);
        case SIDEBAR_ITEM_STATS:
          return StatsDialog.SendMessage(
            pMsg->message, wParam, pMsg->lParam);
        case SIDEBAR_ITEM_SEARCH:
          return SearchDialog.SendMessage(
            pMsg->message, wParam, pMsg->lParam);
        case SIDEBAR_ITEM_SEASONS:
          return SeasonDialog.SendMessage(
            pMsg->message, wParam, pMsg->lParam);
        case SIDEBAR_ITEM_FEEDS:
          return TorrentDialog.SendMessage(
            pMsg->message, wParam, pMsg->lParam);
      }
      break;
    }

    // Back & forward buttons are used for navigation
    case WM_XBUTTONUP: {
      switch (HIWORD(pMsg->wParam)) {
        case XBUTTON1:
          navigation.GoBack();
          break;
        case XBUTTON2:
          navigation.GoForward();
          break;
      }
      return TRUE;
    }
  }

  return FALSE;
}

// =============================================================================

BOOL MainDialog::OnClose() {
  if (Settings.Program.General.close) {
    Hide();
    return TRUE;
  }
  
  if (Settings.Program.Exit.ask) {
    win32::TaskDialog dlg(APP_TITLE, TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Are you sure you want to exit?");
    dlg.AddButton(L"Yes", IDYES);
    dlg.AddButton(L"No", IDNO);
    dlg.Show(g_hMain);
    if (dlg.GetSelectedButtonID() != IDYES) return TRUE;
  }

  return FALSE;
}

BOOL MainDialog::OnDestroy() {
  // Announce
  if (Taiga.play_status == PLAYSTATUS_PLAYING) {
    Taiga.play_status = PLAYSTATUS_STOPPED;
    Announcer.Do(ANNOUNCE_TO_HTTP);
  }
  Announcer.Clear(ANNOUNCE_TO_MESSENGER | ANNOUNCE_TO_SKYPE);
  
  // Close other dialogs
  AnimeDialog.Destroy();
  RecognitionTest.Destroy();
  SearchDialog.Destroy();
  SeasonDialog.Destroy();
  TorrentDialog.Destroy();
  
  // Cleanup
  MainClient.Cleanup();
  Taskbar.Destroy();
  TaskbarList.Release();
  
  // Save settings
  if (Settings.Program.Exit.remember_pos_size) {
    Settings.Program.Position.maximized = (GetWindowLong() & WS_MAXIMIZE) ? TRUE : FALSE;
    if (Settings.Program.Position.maximized == FALSE) {
      bool invisible = !IsVisible();
      if (invisible) ActivateWindow(GetWindowHandle());
      win32::Rect rcWindow; GetWindowRect(&rcWindow);
      if (invisible) Hide();
      Settings.Program.Position.x = rcWindow.left;
      Settings.Program.Position.y = rcWindow.top;
      Settings.Program.Position.w = rcWindow.Width();
      Settings.Program.Position.h = rcWindow.Height();
    }
  }
  Settings.Save();

  // Save anime database
  AnimeDatabase.SaveDatabase();
  
  // Exit
  Taiga.PostQuitMessage();
  return TRUE;
}

void MainDialog::OnDropFiles(HDROP hDropInfo) {
#ifdef _DEBUG
  WCHAR buffer[MAX_PATH];
  if (DragQueryFile(hDropInfo, 0, buffer, MAX_PATH) > 0) {
    anime::Episode episode;
    Meow.ExamineTitle(buffer, episode); 
    MessageBox(ReplaceVariables(Settings.Program.Balloon.format, episode).c_str(), APP_TITLE, MB_OK);
  }
#endif
}

LRESULT MainDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // Toolbar controls
  if (idCtrl == IDC_TOOLBAR_MENU || idCtrl == IDC_TOOLBAR_MAIN || idCtrl == IDC_TOOLBAR_SEARCH) {
    return OnToolbarNotify(reinterpret_cast<LPARAM>(pnmh));

  // Tree control
  } else if (idCtrl == IDC_TREE_MAIN) {
    return OnTreeNotify(reinterpret_cast<LPARAM>(pnmh));

  // Button control
  } else if (idCtrl == IDC_BUTTON_CANCELSEARCH) {
    if (pnmh->code == NM_CUSTOMDRAW) {
      return cancel_button.OnCustomDraw(reinterpret_cast<LPARAM>(pnmh));
    }
  }
  
  return 0;
}

void MainDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  // Paint sidebar
  if (treeview.IsVisible()) {
    win32::Dc dc = hdc;
    win32::Rect rect;

    rect.Copy(rect_sidebar_);
    dc.FillRect(rect, ::GetSysColor(COLOR_3DFACE));

    rect.left = rect.right - 1;
    dc.FillRect(rect, ::GetSysColor(COLOR_ACTIVEBORDER));
  }
}

void MainDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_ENTERSIZEMOVE: {
      if (::IsAppThemed() && win32::GetWinVersion() >= win32::VERSION_VISTA) {
        SetTransparency(200);
      }
      break;
    }
    case WM_EXITSIZEMOVE: {
      if (::IsAppThemed() && win32::GetWinVersion() >= win32::VERSION_VISTA) {
        SetTransparency(255);
      }
      break;
    }
    case WM_SIZE: {
      if (IsIconic()) {
        if (Settings.Program.General.minimize) Hide();
        return;
      }
      UpdateControlPositions(&size);
      break;
    }
  }
}

// =============================================================================

/* Timer */

// This function is very delicate, even order of things are important.
// Please be careful with what you change.

void MainDialog::OnTimer(UINT_PTR nIDEvent) {
  // Measure stability
  Stats.uptime++;
  // Refresh statistics window
  if (StatsDialog.IsVisible()) {
    if (Stats.uptime % 10 == 0) { // Recalculate every 10 seconds
      Stats.CalculateAll();
    }
    StatsDialog.Refresh();
  }

  // ===========================================================================

  // Free memory
  Taiga.ticker_memory++;
  if (Taiga.ticker_memory >= 10 * 60) { // 10 minutes
    Taiga.ticker_memory = 0;
    ImageDatabase.FreeMemory();
  }

  // ===========================================================================

  // Check event queue
  Taiga.ticker_queue++;
  if (Taiga.ticker_queue >= 5 * 60) { // 5 minutes
    Taiga.ticker_queue = 0;
    if (History.queue.updating == false) {
      History.queue.Check();
    }
  }

  // ===========================================================================
  
  // Check new episodes (if folder monitor is disabled)
  if (!Settings.Folders.watch_enabled) {
    Taiga.ticker_new_episodes++;
    if (Taiga.ticker_new_episodes >= 30 * 60) { // 30 minutes
      Taiga.ticker_new_episodes = 0;
      ExecuteAction(L"CheckEpisodes()", TRUE);
    }
  }

  // ===========================================================================

  // Check feeds
  for (unsigned int i = 0; i < Aggregator.feeds.size(); i++) {
    switch (Aggregator.feeds[i].category) {
      case FEED_CATEGORY_LINK:
        if (Settings.RSS.Torrent.check_enabled) {
          Aggregator.feeds[i].ticker++;
        }
        if (Settings.RSS.Torrent.check_enabled && Settings.RSS.Torrent.check_interval) {
          if (TorrentDialog.IsWindow()) {
            TorrentDialog.SetTimerText(L"Check new torrents [" + 
              ToTimeString(Settings.RSS.Torrent.check_interval * 60 - Aggregator.feeds[i].ticker) + L"]");
          }
          if (Aggregator.feeds[i].ticker >= Settings.RSS.Torrent.check_interval * 60) {
            Aggregator.feeds[i].Check(Settings.RSS.Torrent.source);
          }
        } else {
          if (TorrentDialog.IsWindow()) {
            TorrentDialog.SetTimerText(L"Check new torrents");
          }
        }
        break;
    }
  }

  // ===========================================================================
  
  // Check process list for media players
  auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);
  int media_index = MediaPlayers.Check();

  // Media player is running
  if (media_index > -1) {
    // Started to watch?
    if (CurrentEpisode.anime_id == anime::ID_UNKNOWN) {
      // Recognized?
      if (Taiga.is_recognition_enabled) {
        if (Meow.ExamineTitle(MediaPlayers.current_title, CurrentEpisode)) {
          anime_item = Meow.MatchDatabase(CurrentEpisode, false, true);
          if (anime_item) {
            CurrentEpisode.Set(anime_item->GetId());
            anime_item->StartWatching(CurrentEpisode);
            return;
          }
        }
        // Not recognized
        CurrentEpisode.Set(anime::ID_NOTINLIST);
        if (CurrentEpisode.title.empty()) {
#ifdef _DEBUG
          ChangeStatus(MediaPlayers.items[MediaPlayers.index].name + L" is running.");
#endif
        } else {
#ifdef _DEBUG
          std::multimap<int, int> scores = Meow.GetScores();
          debug::Print(L"Not recognized: " + CurrentEpisode.title + L"\n");
          debug::Print(L"Could be:\n");
          for (auto it = scores.begin(); it != scores.end(); ++it) {
            debug::Print(L"* " + AnimeDatabase.items[it->second].GetTitle() + 
                         L" | Score: " + ToWstr(-it->first) + L"\n");
          }
#endif  
          ChangeStatus(L"Watching: " + CurrentEpisode.title + 
            PushString(L" #", CurrentEpisode.number) + L" (Not recognized)");
          if (Settings.Program.Balloon.enabled) {
            wstring tip_text = ReplaceVariables(Settings.Program.Balloon.format, CurrentEpisode);
            tip_text += L"\nClick here to search MyAnimeList for this anime.";
            Taiga.current_tip_type = TIPTYPE_SEARCH;
            Taskbar.Tip(L"", L"", 0);
            Taskbar.Tip(tip_text.c_str(), L"Media is not in your list", NIIF_WARNING);
          }
        }
      }

    // Already watching or not recognized before
    } else {
      // Tick and compare with delay time
      if (Taiga.ticker_media > -1 && Taiga.ticker_media <= Settings.Account.Update.delay) {
        if (Taiga.ticker_media == Settings.Account.Update.delay) {
          // Disable ticker
          Taiga.ticker_media = -1;
          // Announce current episode
          Announcer.Do(ANNOUNCE_TO_HTTP | ANNOUNCE_TO_MESSENGER | ANNOUNCE_TO_MIRC | ANNOUNCE_TO_SKYPE);
          // Update
          if (Settings.Account.Update.time == UPDATE_MODE_AFTERDELAY && anime_item)
            anime_item->UpdateList(CurrentEpisode);
          return;
        }
        if (Settings.Account.Update.check_player == FALSE || 
            MediaPlayers.items[media_index].window_handle == GetForegroundWindow())
          Taiga.ticker_media++;
      }
      // Caption changed?
      if (MediaPlayers.TitleChanged() == true) {
        CurrentEpisode.Set(anime::ID_UNKNOWN);
        if (anime_item) {
          anime_item->EndWatching(CurrentEpisode);
          anime_item->UpdateList(CurrentEpisode);
        }
        Taiga.ticker_media = 0;
      }
    }
  
  // Media player is NOT running
  } else {
    // Was running, but not watching
    if (!anime_item) {
      if (MediaPlayers.index_old > 0){
        ChangeStatus();
        CurrentEpisode.Set(anime::ID_UNKNOWN);
        MediaPlayers.index_old = 0;
      }
    
    // Was running and watching
    } else {
      CurrentEpisode.Set(anime::ID_UNKNOWN);
      anime_item->EndWatching(CurrentEpisode);
      if (Settings.Account.Update.time == UPDATE_MODE_WAITPLAYER)
        anime_item->UpdateList(CurrentEpisode);
      Taiga.ticker_media = 0;
    }
  }

  // Update status timer
  UpdateStatusTimer();
}

// =============================================================================

/* Taskbar */

void MainDialog::OnTaskbarCallback(UINT uMsg, LPARAM lParam) {
  // Taskbar creation notification
  if (uMsg == WM_TASKBARCREATED) {
    Taskbar.Create(m_hWindow, nullptr, APP_TITLE);
  
  // Windows 7 taskbar interface
  } else if (uMsg == WM_TASKBARBUTTONCREATED) {
    TaskbarList.Initialize(m_hWindow);

  // Taskbar callback
  } else if (uMsg == WM_TASKBARCALLBACK) {
    switch (lParam) {
      case NIN_BALLOONSHOW: {
        debug::Print(L"Tip type: " + ToWstr(Taiga.current_tip_type) + L"\n");
        break;
      }
      case NIN_BALLOONTIMEOUT: {
        Taiga.current_tip_type = TIPTYPE_NORMAL;
        break;
      }
      case NIN_BALLOONUSERCLICK: {
        switch (Taiga.current_tip_type) {
          case TIPTYPE_SEARCH:
            ExecuteAction(L"SearchAnime(" + CurrentEpisode.title + L")");
            break;
          case TIPTYPE_TORRENT:
            navigation.SetCurrentPage(SIDEBAR_ITEM_FEEDS);
            break;
          case TIPTYPE_UPDATEFAILED:
            History.queue.Check();
            break;
        }
        Taiga.current_tip_type = TIPTYPE_NORMAL;
        break;
      }
      case WM_LBUTTONDBLCLK: {
        ActivateWindow(m_hWindow);
        break;
      }
      case WM_RBUTTONDOWN: {
        UpdateAllMenus(AnimeDatabase.GetCurrentItem());
        SetForegroundWindow();
        ExecuteAction(UI.Menus.Show(m_hWindow, 0, 0, L"Tray"));
        UpdateAllMenus(AnimeDatabase.GetCurrentItem());
        break;
      }
    }
  }
}

// =============================================================================

void MainDialog::ChangeStatus(wstring str) {
  if (!str.empty()) str = L"  " + str;
  statusbar.SetText(str.c_str());
}

void MainDialog::EnableInput(bool enable) {
  // Toolbar buttons
  toolbar_main.EnableButton(TOOLBAR_BUTTON_SYNCHRONIZE, enable);
  // Content
  AnimeListDialog.Enable(enable);
  HistoryDialog.Enable(enable);
}

void MainDialog::UpdateControlPositions(const SIZE* size) {
  // Set client area
  win32::Rect rect_client;
  if (size == nullptr) {
    GetClientRect(&rect_client);
  } else {
    rect_client.Set(0, 0, size->cx, size->cy);
  }

  // Resize rebar
  rebar.SendMessage(WM_SIZE, 0, 0);
  rect_client.top += rebar.GetBarHeight();
  
  // Resize status bar
  win32::Rect rcStatus;
  statusbar.GetClientRect(&rcStatus);
  statusbar.SendMessage(WM_SIZE, 0, 0);
  UpdateStatusTimer();
  rect_client.bottom -= rcStatus.Height();
  
  // Set sidebar
  rect_sidebar_.Set(0, rect_client.top, 140, rect_client.bottom);
  // Resize treeview
  if (treeview.IsVisible()) {
    win32::Rect rect_tree(rect_sidebar_);
    rect_tree.Inflate(-ScaleX(WIN_CONTROL_MARGIN), -ScaleY(WIN_CONTROL_MARGIN));
    treeview.SetPosition(nullptr, rect_tree);
  }

  // Set content
  if (treeview.IsVisible()) {
    rect_content_.Subtract(rect_client, rect_sidebar_);
  } else {
    rect_content_ = rect_client;
  }
  
  // Resize content
  AnimeListDialog.SetPosition(nullptr, rect_content_);
  HistoryDialog.SetPosition(nullptr, rect_content_);
  NowPlayingDialog.SetPosition(nullptr, rect_content_);
  SearchDialog.SetPosition(nullptr, rect_content_);
  SeasonDialog.SetPosition(nullptr, rect_content_);
  StatsDialog.SetPosition(nullptr, rect_content_);
  TorrentDialog.SetPosition(nullptr, rect_content_);
}

void MainDialog::UpdateStatusTimer() {
  win32::Rect rect;
  GetClientRect(&rect);

  int seconds = Settings.Account.Update.delay - Taiga.ticker_media;

  if (CurrentEpisode.anime_id > anime::ID_UNKNOWN && 
      seconds > 0 && seconds < Settings.Account.Update.delay &&
      AnimeDatabase.FindItem(CurrentEpisode.anime_id)->IsUpdateAllowed(CurrentEpisode, true)) {
    wstring str = L"List update in " + ToTimeString(seconds);
    statusbar.SetPartText(1, str.c_str());
    statusbar.SetPartTipText(1, str.c_str());

    statusbar.SetPartWidth(0, rect.Width() - ScaleX(160));
    statusbar.SetPartWidth(1, ScaleX(160));

  } else {
    statusbar.SetPartWidth(0, rect.Width());
    statusbar.SetPartWidth(1, 0);
  }
}

void MainDialog::UpdateTip() {
  wstring tip = APP_TITLE;
  if (Taiga.logged_in) {
    tip += L"\n" + AnimeDatabase.user.GetName() + L" is logged in.";
  }
  if (CurrentEpisode.anime_id > anime::ID_UNKNOWN) {
    auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);
    tip += L"\nWatching: " + anime_item->GetTitle() + 
      (!CurrentEpisode.number.empty() ? L" #" + CurrentEpisode.number : L"");
  }
  Taskbar.Modify(tip.c_str());
}

void MainDialog::UpdateTitle() {
  wstring title = APP_TITLE;
  if (!Settings.Account.MAL.user.empty()) {
    title += L" \u2013 " + Settings.Account.MAL.user;
  }
  if (CurrentEpisode.anime_id > anime::ID_UNKNOWN) {
    auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);
    title += L" \u2013 " + anime_item->GetTitle() + PushString(L" #", CurrentEpisode.number);
    if (Settings.Account.Update.out_of_range && 
        GetEpisodeLow(CurrentEpisode.number) > anime_item->GetMyLastWatchedEpisode() + 1) {
      title += L" (out of range)";
    }
  }
  SetText(title);
}

// =============================================================================

int MainDialog::Navigation::GetCurrentPage() {
  return current_page_;
}

void MainDialog::Navigation::SetCurrentPage(int page, bool add_to_history) {
  if (page == current_page_)
    return;
  
  int previous_page = current_page_;
  current_page_ = page;

  wstring cue_text, search_text;
  switch (current_page_) {
    case SIDEBAR_ITEM_ANIMELIST:
    case SIDEBAR_ITEM_SEASONS:
      parent->search_bar.mode = SEARCH_MODE_NONE;
      cue_text = L"Filter list";
      break;
    case SIDEBAR_ITEM_NOWPLAYING:
    case SIDEBAR_ITEM_HISTORY:
    case SIDEBAR_ITEM_STATS:
    case SIDEBAR_ITEM_SEARCH:
      parent->search_bar.mode = SEARCH_MODE_MAL;
      cue_text = L"Search MyAnimeList";
      if (current_page_ == SIDEBAR_ITEM_SEARCH)
        search_text = SearchDialog.search_text;
      break;
    case SIDEBAR_ITEM_FEEDS:
      parent->search_bar.mode = SEARCH_MODE_FEED;
      cue_text = L"Search torrents";
      break;
  }
  if (!parent->search_bar.filters.text.empty()) {
    parent->search_bar.filters.text.clear();
    switch (previous_page) {
      case SIDEBAR_ITEM_ANIMELIST:
        AnimeListDialog.RefreshList();
        AnimeListDialog.RefreshTabs();
        break;
      case SIDEBAR_ITEM_SEASONS:
        SeasonDialog.RefreshList();
        break;
    }
  }
  parent->edit.SetCueBannerText(cue_text.c_str());
  parent->edit.SetText(search_text);
  
  AnimeListDialog.Hide();
  HistoryDialog.Hide();
  NowPlayingDialog.Hide();
  SearchDialog.Hide();
  SeasonDialog.Hide();
  StatsDialog.Hide();
  TorrentDialog.Hide();

  #define DISPLAY_PAGE(item, dialog, resource_id) \
    case item: \
      if (!dialog.IsWindow()) dialog.Create(resource_id, parent->GetWindowHandle(), false); \
      parent->UpdateControlPositions(); \
      dialog.Show(); \
      break;
  switch (current_page_) {
    DISPLAY_PAGE(SIDEBAR_ITEM_NOWPLAYING, NowPlayingDialog, IDD_ANIME_INFO);
    DISPLAY_PAGE(SIDEBAR_ITEM_ANIMELIST, AnimeListDialog, IDD_ANIME_LIST);
    DISPLAY_PAGE(SIDEBAR_ITEM_HISTORY, HistoryDialog, IDD_HISTORY);
    DISPLAY_PAGE(SIDEBAR_ITEM_STATS, StatsDialog, IDD_STATS);
    DISPLAY_PAGE(SIDEBAR_ITEM_SEARCH, SearchDialog, IDD_SEARCH);
    DISPLAY_PAGE(SIDEBAR_ITEM_SEASONS, SeasonDialog, IDD_SEASON);
    DISPLAY_PAGE(SIDEBAR_ITEM_FEEDS, TorrentDialog, IDD_TORRENT);
  }
  #undef DISPLAY_PAGE

  parent->treeview.SelectItem(parent->treeview.hti.at(current_page_));

  UpdateViewMenu();
  Refresh(add_to_history);
}

void MainDialog::Navigation::GoBack() {
  if (index_ > 0) {
    index_--;
    SetCurrentPage(items_.at(index_), false);
  }
}

void MainDialog::Navigation::GoForward() {
  if (index_ < items_.size() - 1) {
    index_++;
    SetCurrentPage(items_.at(index_), false);
  }
}

void MainDialog::Navigation::Refresh(bool add_to_history) {
  if (add_to_history) {
    auto it = std::find(items_.begin(), items_.end(), current_page_);
    if (it != items_.end())
      items_.erase(it);

    items_.push_back(current_page_);
    index_ = items_.size() - 1;
  }
}