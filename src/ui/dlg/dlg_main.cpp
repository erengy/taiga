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

#include "base/std.h"

#include "dlg_main.h"

#include "dlg_about.h"
#include "dlg_anime_info.h"
#include "dlg_anime_list.h"
#include "dlg_history.h"
#include "dlg_search.h"
#include "dlg_season.h"
#include "dlg_settings.h"
#include "dlg_stats.h"
#include "dlg_test_recognition.h"
#include "dlg_torrent.h"
#include "dlg_update.h"

#include "library/anime_db.h"
#include "library/anime_filter.h"
#include "library/anime_util.h"
#include "library/resource.h"
#include "taiga/announce.h"
#include "base/common.h"
#include "taiga/debug.h"
#include "base/gfx.h"
#include "library/history.h"
#include "taiga/http.h"
#include "track/media.h"
#include "track/monitor.h"
#include "base/process.h"
#include "track/recognition.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "taiga/script.h"
#include "taiga/stats.h"
#include "base/string.h"
#include "taiga/taiga.h"
#include "ui/dialog.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "sync/service.h"

#include "win/win_gdi.h"
#include "win/win_taskbar.h"
#include "win/win_taskdialog.h"

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
  ui::Theme.CreateFonts(GetDC());
  
  // Create controls
  CreateDialogControls();

  // Select default content page
  navigation.SetCurrentPage(SIDEBAR_ITEM_ANIMELIST);

  // Start process timer
  SetTimer(g_hMain, TIMER_MAIN, 1000, nullptr);
  
  // Add icon to taskbar
  Taskbar.Create(g_hMain, nullptr, TAIGA_APP_TITLE);

  ChangeStatus();
  UpdateTip();
  UpdateTitle();
  
  // Refresh menus
  ui::Menus.UpdateAll();

  // Apply start-up settings
  if (Settings.GetBool(taiga::kSync_AutoOnStart)) { 		
    ExecuteAction(L"Synchronize"); 		
  }
  if (Settings.GetBool(taiga::kApp_Behavior_ScanAvailableEpisodes)) {
    ScanAvailableEpisodes(anime::ID_UNKNOWN, false, true);
  }
  if (!Settings.GetBool(taiga::kApp_Behavior_StartMinimized)) {
    Show(Settings.GetBool(taiga::kApp_Position_Remember) && Settings.GetBool(taiga::kApp_Position_Maximized) ? 
      SW_MAXIMIZE : SW_SHOWNORMAL);
  }
  if (taiga::GetCurrentUsername().empty()) {
    win::TaskDialog dlg(TAIGA_APP_TITLE, TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Welcome to Taiga!");
    dlg.SetContent(L"Username is not set. Would you like to open settings window to set it now?");
    dlg.AddButton(L"Yes", IDYES);
    dlg.AddButton(L"No", IDNO);
    dlg.Show(g_hMain);
    if (dlg.GetSelectedButtonID() == IDYES)
      ExecuteAction(L"Settings", SECTION_SERVICES, PAGE_SERVICES_MAL);
  }
  if (Settings.GetBool(taiga::kLibrary_WatchFolders)) {
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
  toolbar_main.SetImageList(ui::Theme.GetImageList24().GetHandle(), 24, 24);
  toolbar_main.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  // Create search toolbar
  toolbar_search.Attach(GetDlgItem(IDC_TOOLBAR_SEARCH));
  toolbar_search.SetImageList(ui::Theme.GetImageList16().GetHandle(), 16, 16);
  toolbar_search.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  // Create search text
  edit.Attach(GetDlgItem(IDC_EDIT_SEARCH));
  edit.SetCueBannerText(L"Search list");
  edit.SetParent(toolbar_search.GetWindowHandle());
  edit.SetPosition(nullptr, 0, 1, 200, 20);
  edit.SetMargins(1, 16);
  win::Rect rcEdit; edit.GetRect(&rcEdit);
  // Create cancel search button
  cancel_button.Attach(GetDlgItem(IDC_BUTTON_CANCELSEARCH));
  cancel_button.SetParent(edit.GetWindowHandle());
  cancel_button.SetPosition(nullptr, rcEdit.right + 1, 0, 16, 16);
  // Create treeview control
  treeview.Attach(GetDlgItem(IDC_TREE_MAIN));
  treeview.SendMessage(TVM_SETBKCOLOR, 0, ::GetSysColor(COLOR_3DFACE));
  treeview.SetImageList(ui::Theme.GetImageList16().GetHandle());
  treeview.SetItemHeight(20);
  treeview.SetTheme();
  if (Settings.GetBool(taiga::kApp_Option_HideSidebar)) {
    treeview.Hide();
  }
  // Create status bar
  statusbar.Attach(GetDlgItem(IDC_STATUSBAR_MAIN));
  statusbar.SetImageList(ui::Theme.GetImageList16().GetHandle());
  statusbar.InsertPart(-1, 0, 0, 900, nullptr, nullptr);
  statusbar.InsertPart(ui::kIcon16_Clock, 0, 0,  32, nullptr, nullptr);

  // Insert treeview items
  treeview.hti.push_back(treeview.InsertItem(L"Now Playing", ui::kIcon16_Play, SIDEBAR_ITEM_NOWPLAYING, nullptr));
  treeview.hti.push_back(treeview.InsertItem(nullptr, -1, -1, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Anime List", ui::kIcon16_DocumentA, SIDEBAR_ITEM_ANIMELIST, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"History", ui::kIcon16_Clock, SIDEBAR_ITEM_HISTORY, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Statistics", ui::kIcon16_Chart, SIDEBAR_ITEM_STATS, nullptr));
  treeview.hti.push_back(treeview.InsertItem(nullptr, -1, -1, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Search", ui::kIcon16_Search, SIDEBAR_ITEM_SEARCH, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Seasons", ui::kIcon16_Calendar, SIDEBAR_ITEM_SEASONS, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Torrents", ui::kIcon16_Feed, SIDEBAR_ITEM_FEEDS, nullptr));
  if (History.queue.GetItemCount() > 0) {
    treeview.RefreshHistoryCounter();
  }

  // Insert menu toolbar buttons
  BYTE fsStyle0 = BTNS_AUTOSIZE | BTNS_DROPDOWN | BTNS_SHOWTEXT;
  toolbar_menu.InsertButton(0, I_IMAGENONE, 100, 1, fsStyle0, 0, L"  File", nullptr);
  toolbar_menu.InsertButton(1, I_IMAGENONE, 101, 1, fsStyle0, 0, L"  Services", nullptr);
  toolbar_menu.InsertButton(2, I_IMAGENONE, 102, 1, fsStyle0, 0, L"  Tools", nullptr);
  toolbar_menu.InsertButton(3, I_IMAGENONE, 103, 1, fsStyle0, 0, L"  View", nullptr);
  toolbar_menu.InsertButton(4, I_IMAGENONE, 104, 1, fsStyle0, 0, L"  Help", nullptr);
  // Insert main toolbar buttons
  BYTE fsStyle1 = BTNS_AUTOSIZE;
  BYTE fsStyle2 = BTNS_AUTOSIZE | BTNS_WHOLEDROPDOWN;
  toolbar_main.InsertButton(0, ui::kIcon24_Sync, TOOLBAR_BUTTON_SYNCHRONIZE, 
                            1, fsStyle1, 0, nullptr, L"Synchronize list");
  toolbar_main.InsertButton(1, ui::kIcon24_Mal, TOOLBAR_BUTTON_MAL, 
                            1, fsStyle1, 1, nullptr, L"Go to my panel at MyAnimeList");
  toolbar_main.InsertButton(2, ui::kIcon24_Herro, TOOLBAR_BUTTON_HERRO, 
                            1, fsStyle1, 2, nullptr, L"Go to my profile at Herro");
  toolbar_main.InsertButton(3, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_main.InsertButton(4, ui::kIcon24_Folders, TOOLBAR_BUTTON_FOLDERS, 
                            1, fsStyle2, 4, nullptr, L"Root folders");
  toolbar_main.InsertButton(5, ui::kIcon24_Tools, TOOLBAR_BUTTON_TOOLS, 
                            1, fsStyle2, 5, nullptr, L"External links");
  toolbar_main.InsertButton(6, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_main.InsertButton(7, ui::kIcon24_Settings, TOOLBAR_BUTTON_SETTINGS, 
                            1, fsStyle1, 7, nullptr, L"Change program settings");
#ifdef _DEBUG
  toolbar_main.InsertButton(8, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_main.InsertButton(9, ui::kIcon24_About, TOOLBAR_BUTTON_ABOUT, 
                            1, fsStyle1, 9, nullptr, L"Debug");
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
    win::kControlMargin, 0, 0, 0, 0, 0, 
    HIWORD(toolbar_main.GetButtonSize()) + 2, 
    fMask, fStyle | RBBS_BREAK);
  rebar.InsertBand(toolbar_search.GetWindowHandle(), 
    0, win::kControlMargin, 0, 208, 0, 0, 0, 
    HIWORD(toolbar_search.GetButtonSize()), 
    fMask, fStyle);
}

void MainDialog::InitWindowPosition() {
  UINT flags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER;
  const LONG min_w = ScaleX(710);
  const LONG min_h = ScaleX(480);
  
  win::Rect rcParent, rcWindow;
  ::GetWindowRect(GetParent(), &rcParent);
  rcWindow.Set(
    Settings.GetInt(taiga::kApp_Position_X),
    Settings.GetInt(taiga::kApp_Position_Y),
    Settings.GetInt(taiga::kApp_Position_X) + Settings.GetInt(taiga::kApp_Position_W),
    Settings.GetInt(taiga::kApp_Position_Y) + Settings.GetInt(taiga::kApp_Position_H));

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
      !Settings.GetBool(taiga::kApp_Position_Maximized) &&
      Settings.GetBool(taiga::kApp_Position_Remember)) {
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
      auto pCDS = reinterpret_cast<PCOPYDATASTRUCT>(lParam);
      // JetAudio
      if (pCDS->dwData == 0x3000 /* JRC_COPYDATA_ID_TRACK_FILENAME */) {
        MediaPlayers.new_title = StrToWstr(reinterpret_cast<LPCSTR>(pCDS->lpData));
        return TRUE;
      // Media Portal
      } else if (pCDS->dwData == 0x1337) {
        MediaPlayers.new_title = StrToWstr(reinterpret_cast<LPCSTR>(pCDS->lpData));
        return TRUE;
      }
      break;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL MainDialog::PreTranslateMessage(MSG* pMsg) {
  switch (pMsg->message) {
    case WM_KEYDOWN: {
      switch (pMsg->wParam) {
        // Clear search text
        case VK_ESCAPE: {
          if (::GetFocus() == edit.GetWindowHandle()) {
            edit.SetText(L"");
            return TRUE;
          }
          break;
        }
        // Search
        case VK_RETURN: {
          if (::GetFocus() == edit.GetWindowHandle()) {
            wstring text;
            edit.GetText(text);
            if (text.empty()) break;
            switch (search_bar.mode) {
              case SEARCH_MODE_MAL: {
                ExecuteAction(L"SearchAnime(" + text + L")");
                return TRUE;
              }
              case SEARCH_MODE_FEED: {
                TorrentDialog.Search(Settings[taiga::kTorrent_Discovery_SearchUrl], text);
                return TRUE;
              }
            }
          }
          break;
        }
        // Focus search box
        case 'F': {
          if (GetKeyState(VK_CONTROL) & 0x8000) {
            edit.SetFocus();
            edit.SetSel(0, -1);
            return TRUE;
          }
          break;
        }
        case VK_F3: {
          edit.SetFocus();
          edit.SetSel(0, -1);
          return TRUE;
        }
        case VK_F5: {
          switch (navigation.GetCurrentPage()) {
            case SIDEBAR_ITEM_ANIMELIST:
              // Scan available episodes
              ScanAvailableEpisodes(anime::ID_UNKNOWN, true, false);
              return TRUE;
            case SIDEBAR_ITEM_HISTORY:
              // Refresh history
              DlgHistory.RefreshList();
              treeview.RefreshHistoryCounter();
              return TRUE;
            case SIDEBAR_ITEM_STATS:
              // Refresh stats
              Stats.CalculateAll();
              StatsDialog.Refresh();
              return TRUE;
            case SIDEBAR_ITEM_SEASONS:
              // Refresh season data
              SeasonDialog.RefreshData();
              return TRUE;
            case SIDEBAR_ITEM_FEEDS: {
              // Check new torrents
              Feed* feed = Aggregator.Get(FEED_CATEGORY_LINK);
              if (feed) {
                edit.SetText(L"");
                feed->Check(Settings[taiga::kTorrent_Discovery_Source]);
                return TRUE;
              }
              break;
            }
          }
          break;
        }
      }
      break;
    }

    // Forward mouse wheel messages to the active page
    case WM_MOUSEWHEEL: {
      // Ignoring the low-order word of wParam to avoid falling into an infinite
      // message-forwarding loop
      pMsg->wParam = MAKEWPARAM(0, HIWORD(pMsg->wParam));
      switch (navigation.GetCurrentPage()) {
        case SIDEBAR_ITEM_ANIMELIST:
          return AnimeListDialog.SendMessage(
            pMsg->message, pMsg->wParam, pMsg->lParam);
        case SIDEBAR_ITEM_HISTORY:
          return DlgHistory.SendMessage(
            pMsg->message, pMsg->wParam, pMsg->lParam);
        case SIDEBAR_ITEM_STATS:
          return StatsDialog.SendMessage(
            pMsg->message, pMsg->wParam, pMsg->lParam);
        case SIDEBAR_ITEM_SEARCH:
          return SearchDialog.SendMessage(
            pMsg->message, pMsg->wParam, pMsg->lParam);
        case SIDEBAR_ITEM_SEASONS:
          return SeasonDialog.SendMessage(
            pMsg->message, pMsg->wParam, pMsg->lParam);
        case SIDEBAR_ITEM_FEEDS:
          return TorrentDialog.SendMessage(
            pMsg->message, pMsg->wParam, pMsg->lParam);
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
  if (Settings.GetBool(taiga::kApp_Behavior_CloseToTray)) {
    Hide();
    return TRUE;
  }

  return FALSE;
}

BOOL MainDialog::OnDestroy() {
  if (Settings.GetBool(taiga::kApp_Position_Remember)) {
    Settings.Set(taiga::kApp_Position_Maximized, (GetWindowLong() & WS_MAXIMIZE) ? true : false);
    if (!Settings.GetBool(taiga::kApp_Position_Maximized)) {
      bool invisible = !IsVisible();
      if (invisible) ActivateWindow(GetWindowHandle());
      win::Rect rcWindow; GetWindowRect(&rcWindow);
      if (invisible) Hide();
      Settings.Set(taiga::kApp_Position_X, rcWindow.left);
      Settings.Set(taiga::kApp_Position_Y, rcWindow.top);
      Settings.Set(taiga::kApp_Position_W, rcWindow.Width());
      Settings.Set(taiga::kApp_Position_H, rcWindow.Height());
    }
  }

  ui::DestroyDialog(ui::kDialogAbout);
  ui::DestroyDialog(ui::kDialogAnimeInformation);
  ui::DestroyDialog(ui::kDialogTestRecognition);
  ui::DestroyDialog(ui::kDialogSettings);
  ui::DestroyDialog(ui::kDialogUpdate);

  Taiga.Uninitialize();

  return TRUE;
}

void MainDialog::OnDropFiles(HDROP hDropInfo) {
#ifdef _DEBUG
  WCHAR buffer[MAX_PATH];
  if (DragQueryFile(hDropInfo, 0, buffer, MAX_PATH) > 0) {
    anime::Episode episode;
    Meow.ExamineTitle(buffer, episode); 
    MessageBox(ReplaceVariables(Settings[taiga::kSync_Notify_Format], episode).c_str(), TAIGA_APP_TITLE, MB_OK);
  }
#endif
}

LRESULT MainDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // Toolbar controls
  if (idCtrl == IDC_TOOLBAR_MENU ||
      idCtrl == IDC_TOOLBAR_MAIN ||
      idCtrl == IDC_TOOLBAR_SEARCH) {
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
    win::Dc dc = hdc;
    win::Rect rect;

    rect.Copy(rect_sidebar_);
    dc.FillRect(rect, ::GetSysColor(COLOR_3DFACE));

    rect.left = rect.right - 1;
    dc.FillRect(rect, ::GetSysColor(COLOR_ACTIVEBORDER));
  }
}

void MainDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_ENTERSIZEMOVE: {
      if (::IsAppThemed() && win::GetVersion() >= win::kVersionVista) {
        SetTransparency(200);
      }
      break;
    }
    case WM_EXITSIZEMOVE: {
      if (::IsAppThemed() && win::GetVersion() >= win::kVersionVista) {
        SetTransparency(255);
      }
      break;
    }
    case WM_SIZE: {
      if (IsIconic()) {
        if (Settings.GetBool(taiga::kApp_Behavior_MinimizeToTray))
          Hide();
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

  // Check queue
  Taiga.ticker_queue++;
  if (Taiga.ticker_queue >= 5 * 60) { // 5 minutes
    Taiga.ticker_queue = 0;
    if (History.queue.updating == false) {
      History.queue.Check(true);
    }
  }

  // ===========================================================================
  
  // Check new episodes (if folder monitor is disabled)
  if (!Settings.GetBool(taiga::kLibrary_WatchFolders)) {
    Taiga.ticker_new_episodes++;
    if (Taiga.ticker_new_episodes >= 30 * 60) { // 30 minutes
      Taiga.ticker_new_episodes = 0;
      ScanAvailableEpisodes(anime::ID_UNKNOWN, false, true);
    }
  }

  // ===========================================================================

  // Check feeds
  for (unsigned int i = 0; i < Aggregator.feeds.size(); i++) {
    switch (Aggregator.feeds[i].category) {
      case FEED_CATEGORY_LINK:
        if (Settings.GetBool(taiga::kTorrent_Discovery_AutoCheckEnabled)) {
          Aggregator.feeds[i].ticker++;
        }
        if (Settings.GetBool(taiga::kTorrent_Discovery_AutoCheckEnabled) &&
            Settings.GetInt(taiga::kTorrent_Discovery_AutoCheckInterval) > 0) {
          if (TorrentDialog.IsWindow()) {
            TorrentDialog.SetTimerText(L"Check new torrents [" + 
              ToTimeString(Settings.GetInt(taiga::kTorrent_Discovery_AutoCheckInterval) * 60 - Aggregator.feeds[i].ticker) + L"]");
          }
          if (Aggregator.feeds[i].ticker >= Settings.GetInt(taiga::kTorrent_Discovery_AutoCheckInterval) * 60) {
            Aggregator.feeds[i].Check(Settings[taiga::kTorrent_Discovery_Source], true);
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
      if (Settings.GetBool(taiga::kApp_Option_EnableRecognition)) {
        if (Meow.ExamineTitle(MediaPlayers.current_title, CurrentEpisode)) {
          anime_item = Meow.MatchDatabase(CurrentEpisode, false, true, true, true, true, true);
          if (anime_item) {
            MediaPlayers.SetTitleChanged(false);
            CurrentEpisode.Set(anime_item->GetId());
            StartWatching(*anime_item, CurrentEpisode);
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
          MediaPlayers.SetTitleChanged(false);
          NowPlayingDialog.SetCurrentId(anime::ID_NOTINLIST);
          ChangeStatus(L"Watching: " + CurrentEpisode.title + 
            PushString(L" #", CurrentEpisode.number) + L" (Not recognized)");
          if (Settings.GetBool(taiga::kSync_Notify_NotRecognized)) {
            wstring tip_text = ReplaceVariables(Settings[taiga::kSync_Notify_Format], CurrentEpisode);
            tip_text += L"\nClick here to view similar titles for this anime.";
            Taiga.current_tip_type = taiga::kTipTypeNowPlaying;
            Taskbar.Tip(L"", L"", 0);
            Taskbar.Tip(tip_text.c_str(), L"Media is not in your list", NIIF_WARNING);
          }
        }
      }

    // Already watching or not recognized before
    } else {
      // Tick and compare with delay time
      if (Taiga.ticker_media > -1 && Taiga.ticker_media <= Settings.GetInt(taiga::kSync_Update_Delay)) {
        if (Taiga.ticker_media == Settings.GetInt(taiga::kSync_Update_Delay)) {
          // Disable ticker
          Taiga.ticker_media = -1;
          // Announce current episode
          Announcer.Do(taiga::kAnnounceToHttp | taiga::kAnnounceToMessenger |
                       taiga::kAnnounceToMirc | taiga::kAnnounceToSkype);
          // Update
          if (!Settings.GetBool(taiga::kSync_Update_WaitPlayer))
            if (anime_item)
              UpdateList(*anime_item, CurrentEpisode);
          return;
        }
        if (!Settings.GetBool(taiga::kSync_Update_CheckPlayer) ||
            MediaPlayers.items[media_index].window_handle == GetForegroundWindow())
          Taiga.ticker_media++;
      }
      // Caption changed?
      if (MediaPlayers.TitleChanged()) {
        MediaPlayers.SetTitleChanged(false);
        ChangeStatus();
        bool processed = CurrentEpisode.processed; // TODO: not a good solution...
        CurrentEpisode.Set(anime::ID_UNKNOWN);
        if (anime_item) {
          EndWatching(*anime_item, CurrentEpisode);
          CurrentEpisode.anime_id = anime_item->GetId();
          CurrentEpisode.processed = processed;
          UpdateList(*anime_item, CurrentEpisode);
          CurrentEpisode.anime_id = anime::ID_UNKNOWN;
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
        NowPlayingDialog.SetCurrentId(anime::ID_UNKNOWN);
      }
    
    // Was running and watching
    } else {
      bool processed = CurrentEpisode.processed; // TODO: temporary solution...
      CurrentEpisode.Set(anime::ID_UNKNOWN);
      EndWatching(*anime_item, CurrentEpisode);
      if (Settings.GetBool(taiga::kSync_Update_WaitPlayer)) {
        CurrentEpisode.anime_id = anime_item->GetId();
        CurrentEpisode.processed = processed;
        UpdateList(*anime_item, CurrentEpisode);
        CurrentEpisode.anime_id = anime::ID_UNKNOWN;
      }
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
    Taskbar.Create(GetWindowHandle(), nullptr, TAIGA_APP_TITLE);
  
  // Windows 7 taskbar interface
  } else if (uMsg == WM_TASKBARBUTTONCREATED) {
    TaskbarList.Initialize(GetWindowHandle());

  // Taskbar callback
  } else if (uMsg == WM_TASKBARCALLBACK) {
    switch (lParam) {
      case NIN_BALLOONSHOW: {
        debug::Print(L"Tip type: " + ToWstr(Taiga.current_tip_type) + L"\n");
        break;
      }
      case NIN_BALLOONTIMEOUT: {
        Taiga.current_tip_type = taiga::kTipTypeDefault;
        break;
      }
      case NIN_BALLOONUSERCLICK: {
        switch (Taiga.current_tip_type) {
          case taiga::kTipTypeNowPlaying:
            navigation.SetCurrentPage(SIDEBAR_ITEM_NOWPLAYING);
            break;
          case taiga::kTipTypeSearch:
            ExecuteAction(L"SearchAnime(" + CurrentEpisode.title + L")");
            break;
          case taiga::kTipTypeTorrent:
            navigation.SetCurrentPage(SIDEBAR_ITEM_FEEDS);
            break;
          case taiga::kTipTypeUpdateFailed:
            History.queue.Check(false);
            break;
        }
        ActivateWindow(GetWindowHandle());
        Taiga.current_tip_type = taiga::kTipTypeDefault;
        break;
      }
      case WM_LBUTTONUP:
      case WM_LBUTTONDBLCLK: {
        ActivateWindow(GetWindowHandle());
        break;
      }
      case WM_RBUTTONUP: {
        ui::Menus.UpdateAll(AnimeListDialog.GetCurrentItem());
        SetForegroundWindow();
        ExecuteAction(ui::Menus.Show(GetWindowHandle(), 0, 0, L"Tray"));
        ui::Menus.UpdateAll(AnimeListDialog.GetCurrentItem());
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
  DlgHistory.Enable(enable);
}

void MainDialog::UpdateControlPositions(const SIZE* size) {
  // Set client area
  win::Rect rect_client;
  if (size == nullptr) {
    GetClientRect(&rect_client);
  } else {
    rect_client.Set(0, 0, size->cx, size->cy);
  }

  // Resize rebar
  rebar.SendMessage(WM_SIZE, 0, 0);
  rect_client.top += rebar.GetBarHeight();
  
  // Resize status bar
  win::Rect rcStatus;
  statusbar.GetClientRect(&rcStatus);
  statusbar.SendMessage(WM_SIZE, 0, 0);
  UpdateStatusTimer();
  rect_client.bottom -= rcStatus.Height();
  
  // Set sidebar
  rect_sidebar_.Set(0, rect_client.top, 140, rect_client.bottom);
  // Resize treeview
  if (treeview.IsVisible()) {
    win::Rect rect_tree(rect_sidebar_);
    rect_tree.Inflate(-ScaleX(win::kControlMargin), -ScaleY(win::kControlMargin));
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
  win::Rect rect;
  GetClientRect(&rect);

  int seconds = Settings.GetInt(taiga::kSync_Update_Delay) - Taiga.ticker_media;

  if (CurrentEpisode.anime_id > anime::ID_UNKNOWN && 
      seconds > 0 && seconds < Settings.GetInt(taiga::kSync_Update_Delay) &&
      IsUpdateAllowed(*AnimeDatabase.FindItem(CurrentEpisode.anime_id), CurrentEpisode, true)) {
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
  wstring tip = TAIGA_APP_TITLE;
#ifdef _DEBUG
  tip += L" [debug]";
#endif

  if (CurrentEpisode.anime_id > anime::ID_UNKNOWN) {
    auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);
    tip += L"\nWatching: " + anime_item->GetTitle() + 
      (!CurrentEpisode.number.empty() ? L" #" + CurrentEpisode.number : L"");
  }

  Taskbar.Modify(tip.c_str());
}

void MainDialog::UpdateTitle() {
  wstring title = TAIGA_APP_TITLE;
#ifdef _DEBUG
  title += L" [debug]";
#endif

  const std::wstring username = taiga::GetCurrentUsername();
  if (!username.empty())
    title += L" \u2013 " + username;
#ifdef _DEBUG
  auto service = taiga::GetCurrentService();
  if (service)
    title += L" @ " + service->name();
#endif

  if (CurrentEpisode.anime_id > anime::ID_UNKNOWN) {
    auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);
    title += L" \u2013 " + anime_item->GetTitle() + PushString(L" #", CurrentEpisode.number);
    if (Settings.GetBool(taiga::kSync_Update_OutOfRange) &&
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
      parent->search_bar.mode = SEARCH_MODE_MAL;
      cue_text = L"Filter list or search " + taiga::GetCurrentService()->name();
      break;
    case SIDEBAR_ITEM_NOWPLAYING:
    case SIDEBAR_ITEM_HISTORY:
    case SIDEBAR_ITEM_STATS:
    case SIDEBAR_ITEM_SEARCH:
      parent->search_bar.mode = SEARCH_MODE_MAL;
      cue_text = L"Search " + taiga::GetCurrentService()->name() + L" for anime";
      if (current_page_ == SIDEBAR_ITEM_SEARCH)
        search_text = SearchDialog.search_text;
      break;
    case SIDEBAR_ITEM_FEEDS:
      parent->search_bar.mode = SEARCH_MODE_FEED;
      cue_text = L"Search for torrents";
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
  
  if (current_page_ != SIDEBAR_ITEM_NOWPLAYING) NowPlayingDialog.Hide();
  if (current_page_ != SIDEBAR_ITEM_ANIMELIST) AnimeListDialog.Hide();
  if (current_page_ != SIDEBAR_ITEM_HISTORY) HistoryDialog.Hide();
  if (current_page_ != SIDEBAR_ITEM_STATS) StatsDialog.Hide();
  if (current_page_ != SIDEBAR_ITEM_SEARCH) SearchDialog.Hide();
  if (current_page_ != SIDEBAR_ITEM_SEASONS) SeasonDialog.Hide();
  if (current_page_ != SIDEBAR_ITEM_FEEDS) TorrentDialog.Hide();

  parent->treeview.SelectItem(parent->treeview.hti.at(current_page_));

  ui::Menus.UpdateView();
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