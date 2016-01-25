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

#include "base/process.h"
#include "base/string.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "library/resource.h"
#include "sync/service.h"
#include "sync/sync.h"
#include "taiga/announce.h"
#include "taiga/resource.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "taiga/stats.h"
#include "taiga/taiga.h"
#include "taiga/timer.h"
#include "track/media.h"
#include "track/monitor.h"
#include "track/recognition.h"
#include "track/search.h"
#include "ui/dialog.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_history.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_search.h"
#include "ui/dlg/dlg_season.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_stats.h"
#include "ui/dlg/dlg_torrent.h"
#include "ui/dlg/dlg_update.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "win/win_taskbar.h"
#include "win/win_taskdialog.h"

namespace ui {

MainDialog DlgMain;

MainDialog::MainDialog() {
  navigation.parent = this;
  search_bar.parent = this;

  RegisterDlgClass(L"TaigaMainW");
}

BOOL MainDialog::OnInitDialog() {
  // Initialize window properties
  SetIconLarge(IDI_MAIN);
  SetIconSmall(IDI_MAIN);
  SetSizeMin(320, 240);
  SetSnapGap(10);

  // Create default brushes and fonts
  ui::Theme.CreateBrushes();
  ui::Theme.CreateFonts(GetDC());

  // Create controls
  CreateDialogControls();

  // Start process timer
  taiga::timers.Initialize();

  // Add icon to taskbar
  Taskbar.Create(GetWindowHandle(), nullptr, TAIGA_APP_TITLE);

  ChangeStatus();
  UpdateTip();
  UpdateTitle();

  // Refresh menus
  ui::Menus.UpdateAll();

  // Apply startup settings
  if (Settings.GetBool(taiga::kSync_AutoOnStart)) {
    sync::Synchronize();
  }
  if (Settings.GetBool(taiga::kApp_Behavior_ScanAvailableEpisodes)) {
    ScanAvailableEpisodesQuick();
  }

  // Select default content page
  navigation.SetCurrentPage(kSidebarItemAnimeList);

  // Display window
  InitWindowPosition();

  if (taiga::GetCurrentUsername().empty()) {
    win::TaskDialog dlg(TAIGA_APP_TITLE, TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Welcome to Taiga!");
    dlg.SetContent(L"Username is not set. Would you like to open settings window to set it now?");
    dlg.AddButton(L"Yes", IDYES);
    dlg.AddButton(L"No", IDNO);
    dlg.Show(GetWindowHandle());
    if (dlg.GetSelectedButtonID() == IDYES)
      ShowDlgSettings(kSettingsSectionServices, kSettingsPageServicesMain);
  }
  if (Settings.GetBool(taiga::kLibrary_WatchFolders)) {
    FolderMonitor.SetWindowHandle(GetWindowHandle());
    FolderMonitor.Enable();
  }

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
  toolbar_main.SetImageList(ui::Theme.GetImageList24().GetHandle(), ScaleX(24), ScaleY(24));
  toolbar_main.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  toolbar_main.SendMessage(TB_SETBUTTONSIZE, 0, MAKELPARAM(ScaleX(24), ScaleY(24)));
  // Create search toolbar
  toolbar_search.Attach(GetDlgItem(IDC_TOOLBAR_SEARCH));
  toolbar_search.SetImageList(nullptr, 0, 0);
  toolbar_search.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  toolbar_search.SendMessage(TB_SETBUTTONSIZE, 0, MAKELPARAM(ScaleX(24), ScaleY(24)));
  // Create search text
  edit.Attach(GetDlgItem(IDC_EDIT_SEARCH));
  edit.SetCueBannerText(L"Search list");
  edit.SetMargins(ScaleX(2), ScaleX(16));
  edit.SetParent(toolbar_search.GetWindowHandle());
  win::Rect rcEditWindow; edit.GetWindowRect(&rcEditWindow);
  win::Rect rcToolbar; toolbar_search.GetClientRect(&rcToolbar);
  rcEditWindow.bottom = rcEditWindow.top + ScaleY(22);
  int edit_y = (ScaleY(24) - rcEditWindow.Height()) / 2;
  edit.SetPosition(nullptr, 0, edit_y, rcEditWindow.Width(), rcEditWindow.Height(),
                   SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
  // Create cancel search button
  cancel_button.Attach(GetDlgItem(IDC_BUTTON_CANCELSEARCH));
  cancel_button.SetParent(edit.GetWindowHandle());
  win::Rect rcEdit;
  edit.GetClientRect(&rcEdit);
  win::Rect rcButton;
  rcButton.left = rcEdit.right - ScaleX(16);
  rcButton.top = static_cast<LONG>(std::floor((rcEdit.Height() - ScaleY(16)) / 2.0));
  rcButton.right = rcButton.left + ScaleX(16);
  rcButton.bottom = rcButton.top + ScaleY(16);
  cancel_button.SetPosition(nullptr, rcButton);
  // Create treeview control
  treeview.Attach(GetDlgItem(IDC_TREE_MAIN));
  treeview.SendMessage(TVM_SETBKCOLOR, 0, ::GetSysColor(COLOR_3DFACE));
  treeview.SetImageList(ui::Theme.GetImageList16().GetHandle());
  treeview.SetItemHeight(ScaleY(20));
  treeview.SetTheme();
  if (Settings.GetBool(taiga::kApp_Option_HideSidebar)) {
    treeview.Hide();
  }
  // Create status bar
  statusbar.Attach(GetDlgItem(IDC_STATUSBAR_MAIN));
  statusbar.SetImageList(ui::Theme.GetImageList16().GetHandle());
  statusbar.InsertPart(-1, 0, 0, ScaleX(900), nullptr, nullptr);
  statusbar.InsertPart(ui::kIcon16_Clock, 0, 0, ScaleX(32), nullptr, nullptr);
  statusbar.InsertPart(ui::kIcon16_Cross, 0, 0, ScaleX(32), nullptr, nullptr);

  // Insert treeview items
  treeview.hti.push_back(treeview.InsertItem(L"Now Playing", ui::kIcon16_Play, kSidebarItemNowPlaying, nullptr));
  treeview.hti.push_back(treeview.InsertItem(nullptr, -1, kSidebarItemSeparator1, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Anime List", ui::kIcon16_DocumentA, kSidebarItemAnimeList, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"History", ui::kIcon16_Clock, kSidebarItemHistory, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Statistics", ui::kIcon16_Chart, kSidebarItemStats, nullptr));
  treeview.hti.push_back(treeview.InsertItem(nullptr, -1, kSidebarItemSeparator2, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Search", ui::kIcon16_Search, kSidebarItemSearch, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Seasons", ui::kIcon16_Calendar, kSidebarItemSeasons, nullptr));
  treeview.hti.push_back(treeview.InsertItem(L"Torrents", ui::kIcon16_Feed, kSidebarItemFeeds, nullptr));
  if (History.queue.GetItemCount() > 0) {
    treeview.RefreshHistoryCounter();
  }

  // Insert menu toolbar buttons
  BYTE fsState = TBSTATE_ENABLED;
  BYTE fsStyle0 = BTNS_AUTOSIZE | BTNS_DROPDOWN | BTNS_SHOWTEXT;
  toolbar_menu.InsertButton(0, I_IMAGENONE, 100, fsState, fsStyle0, 0, L"  &File", nullptr);
  toolbar_menu.InsertButton(1, I_IMAGENONE, 101, fsState, fsStyle0, 0, L"  &Services", nullptr);
  toolbar_menu.InsertButton(2, I_IMAGENONE, 102, fsState, fsStyle0, 0, L"  &Tools", nullptr);
  toolbar_menu.InsertButton(3, I_IMAGENONE, 103, fsState, fsStyle0, 0, L"  &View", nullptr);
  toolbar_menu.InsertButton(4, I_IMAGENONE, 104, fsState, fsStyle0, 0, L"  &Help", nullptr);
  // Insert main toolbar buttons
  BYTE fsStyle1 = BTNS_AUTOSIZE;
  BYTE fsStyle2 = BTNS_AUTOSIZE | BTNS_WHOLEDROPDOWN;
  std::wstring tooltip = L"Synchronize list with " + taiga::GetCurrentService()->name();
  toolbar_main.InsertButton(0, ui::kIcon24_Sync, kToolbarButtonSync,
                            fsState, fsStyle1, 0, nullptr, tooltip.c_str());
  toolbar_main.InsertButton(1, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_main.InsertButton(2, ui::kIcon24_Folders, kToolbarButtonFolders,
                            fsState, fsStyle2, 2, nullptr, L"Library folders");
  toolbar_main.InsertButton(3, ui::kIcon24_Tools, kToolbarButtonTools,
                            fsState, fsStyle2, 3, nullptr, L"External links");
  toolbar_main.InsertButton(4, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_main.InsertButton(5, ui::kIcon24_Settings, kToolbarButtonSettings,
                            fsState, fsStyle1, 5, nullptr, L"Change program settings");
#ifdef _DEBUG
  toolbar_main.InsertButton(6, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_main.InsertButton(7, ui::kIcon24_About, kToolbarButtonDebug,
                            fsState, fsStyle1, 7, nullptr, L"Debug");
#endif

  // Insert rebar bands
  UINT fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_HEADERSIZE | RBBIM_SIZE | RBBIM_STYLE;
  UINT fStyle = RBBS_NOGRIPPER;
  rebar.InsertBand(toolbar_menu.GetWindowHandle(),
    GetSystemMetrics(SM_CXSCREEN),
    -2, 0, 0, 0, 0, 0,
    HIWORD(toolbar_menu.GetButtonSize()),
    fMask, fStyle);
  rebar.InsertBand(toolbar_main.GetWindowHandle(),
    GetSystemMetrics(SM_CXSCREEN),
    win::kControlMargin - 2, 0, 0, 0, 0, 0,
    HIWORD(toolbar_main.GetButtonSize()) + 2,
    fMask, fStyle | RBBS_BREAK);
  rebar.InsertBand(toolbar_search.GetWindowHandle(),
    0, win::kControlMargin, 0, rcEditWindow.Width() + win::kControlMargin, 0, 0, 0,
    HIWORD(toolbar_search.GetButtonSize()),
    fMask, fStyle);
}

void MainDialog::InitWindowPosition() {
  win::Rect rcWindow(
      Settings.GetInt(taiga::kApp_Position_X),
      Settings.GetInt(taiga::kApp_Position_Y),
      Settings.GetInt(taiga::kApp_Position_X) + Settings.GetInt(taiga::kApp_Position_W),
      Settings.GetInt(taiga::kApp_Position_Y) + Settings.GetInt(taiga::kApp_Position_H));

  bool first_time = rcWindow.left == -1 && rcWindow.top == -1;
  bool center_owner = first_time;

  WINDOWPLACEMENT wp = {0};
  wp.length = sizeof(WINDOWPLACEMENT);
  wp.showCmd = SW_SHOWNORMAL;

  if (Settings.GetBool(taiga::kApp_Position_Remember)) {
    CopyRect(&wp.rcNormalPosition, &rcWindow);
    if (Settings.GetBool(taiga::kApp_Position_Maximized)) {
      wp.flags = WPF_RESTORETOMAXIMIZED;
      wp.showCmd = SW_SHOWMAXIMIZED;
    }
  } else {
    wp.rcNormalPosition = {0, 0, 960, 640};
    center_owner = true;
  }

  if (Settings.GetBool(taiga::kApp_Behavior_StartMinimized)) {
    wp.showCmd = SW_SHOWMINIMIZED;
  }

  SetPlacement(wp);

  if (center_owner)
    CenterOwner();

  if (Settings.GetBool(taiga::kApp_Behavior_StartMinimized) &&
      Settings.GetBool(taiga::kApp_Behavior_MinimizeToTray)) {
    Show(SW_HIDE);
  }
}

////////////////////////////////////////////////////////////////////////////////

INT_PTR MainDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Log off / Shutdown
    case WM_ENDSESSION: {
      OnDestroy();
      return FALSE;
    }

    // Monitor anime folders
    case WM_MONITORCALLBACK: {
      FolderMonitor.Callback(*reinterpret_cast<DirectoryChangeEntry*>(lParam));
      return TRUE;
    }

    // Show menu
    case WM_TAIGA_SHOWMENU: {
      toolbar_wm.ShowMenu();
      return TRUE;
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL MainDialog::PreTranslateMessage(MSG* pMsg) {
  auto handle_menu_accelerator = [&]() {
    UINT command_id = 0;
    if ((pMsg->wParam >= 'A' && pMsg->wParam <= 'Z') &&
        toolbar_menu.MapAcelerator(pMsg->wParam, command_id)) {
      int button_index = toolbar_menu.GetButtonIndex(command_id);
      toolbar_menu.SetHotItem(button_index, HICF_ACCELERATOR);
      toolbar_wm.toolbar = &toolbar_menu;
      toolbar_wm.ShowMenu();
      return true;
    }
    return false;
  };

  auto is_anime_list_focused = [&]() {
    return navigation.GetCurrentPage() == kSidebarItemAnimeList &&
        ::GetFocus() == DlgAnimeList.listview.GetWindowHandle() &&
        DlgAnimeList.listview.GetSelectedCount() > 0;
  };

  switch (pMsg->message) {
    case WM_KEYDOWN: {
      // Menubar
      auto hot_item = toolbar_menu.GetHotItem();
      if (hot_item > -1) {
        switch (pMsg->wParam) {
          case VK_ESCAPE: {
            toolbar_menu.SetHotItem(-1);
            return TRUE;
          }
          case VK_RETURN:
          case VK_UP:
          case VK_DOWN: {
            toolbar_wm.toolbar = &toolbar_menu;
            toolbar_wm.ShowMenu();
            return TRUE;
          }
          case VK_LEFT:
          case VK_RIGHT: {
            switch (pMsg->wParam) {
              case VK_LEFT:
                if (--hot_item < 0)
                  hot_item = toolbar_menu.GetButtonCount() - 1;
                break;
              case VK_RIGHT:
                if (++hot_item == toolbar_menu.GetButtonCount())
                  hot_item = 0;
                break;
            }
            toolbar_menu.SetHotItem(hot_item, HICF_ARROWKEYS);
            return TRUE;
          }
          default: {
            if (handle_menu_accelerator())
              return TRUE;
            break;
          }
        }
      }

      switch (pMsg->wParam) {
        // Clear search text
        case VK_ESCAPE: {
          if (::GetFocus() == edit.GetWindowHandle()) {
            edit.SetText(L"");
            return TRUE;
          }
          break;
        }
        // Switch tabs
        case VK_TAB: {
          switch (navigation.GetCurrentPage()) {
            case kSidebarItemAnimeList:
              if (GetKeyState(VK_CONTROL) & 0x8000) {
                if (GetKeyState(VK_SHIFT) & 0x8000) {
                  DlgAnimeList.GoToPreviousTab();
                } else {
                  DlgAnimeList.GoToNextTab();
                }
                return TRUE;
              }
              break;
          }
          break;
        }
        // Search
        case VK_RETURN: {
          if (::GetFocus() == edit.GetWindowHandle()) {
            std::wstring text;
            edit.GetText(text);
            if (text.empty())
              break;
            switch (search_bar.mode) {
              case kSearchModeService: {
                BOOL local_search = GetKeyState(VK_CONTROL) & 0x8000;
                ExecuteAction(L"SearchAnime(" + text + L")", local_search);
                return TRUE;
              }
              case kSearchModeFeed: {
                DlgTorrent.Search(Settings[taiga::kTorrent_Discovery_SearchUrl], text);
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
        // Play next episode
        case 'N': {
          if ((GetKeyState(VK_CONTROL) & 0x8000) && !is_anime_list_focused()) {
            anime::PlayNextEpisodeOfLastWatchedAnime();
            return TRUE;
          }
          break;
        }
        // Play random anime
        case 'R': {
          if ((GetKeyState(VK_CONTROL) & 0x8000) && !is_anime_list_focused()) {
            anime::PlayRandomAnime();
            return TRUE;
          }
          break;
        }
        // Synchronize list
        case 'S': {
          if (GetKeyState(VK_CONTROL) & 0x8000) {
            sync::Synchronize();
            return TRUE;
          }
          break;
        }
        // Help
        case VK_F1: {
          ExecuteLink(L"http://taiga.moe/#support");
          return TRUE;
        }
        // Various
        case VK_F5: {
          switch (navigation.GetCurrentPage()) {
            case kSidebarItemAnimeList:
              // Scan available episodes
              ScanAvailableEpisodes(false);
              return TRUE;
            case kSidebarItemHistory:
              // Refresh history
              DlgHistory.RefreshList();
              treeview.RefreshHistoryCounter();
              return TRUE;
            case kSidebarItemStats:
              // Refresh stats
              Stats.CalculateAll();
              DlgStats.Refresh();
              return TRUE;
            case kSidebarItemSearch:
              // Refresh search results
              DlgSearch.RefreshList();
              return TRUE;
            case kSidebarItemSeasons:
              // Refresh season data
              DlgSeason.RefreshData();
              return TRUE;
            case kSidebarItemFeeds: {
              // Check new torrents
              edit.SetText(L"");
              Aggregator.CheckFeed(kFeedCategoryLink,
                                   Settings[taiga::kTorrent_Discovery_Source]);
              return TRUE;
            }
          }
          break;
        }
      }
      break;
    }

    case WM_SYSKEYDOWN: {
      if (IsNumericChar(pMsg->wParam)) {
        int index = (pMsg->wParam - '0') - 1;
        if (index >= 0 &&
            index < static_cast<int>(Settings.library_folders.size())) {
          Execute(Settings.library_folders.at(index));
          return TRUE;
        }
      } else if (handle_menu_accelerator()) {
        return TRUE;
      }
      break;
    }
    case WM_SYSKEYUP: {
      switch (pMsg->wParam) {
        case VK_MENU:
        case VK_F10: {
          if (!(GetKeyState(VK_SHIFT) & 0x8000)) {
            auto hot_item = toolbar_menu.GetHotItem();
            hot_item = hot_item == -1 ? 0 : -1;
            toolbar_menu.SetHotItem(hot_item, HICF_ACCELERATOR);
          }
          return TRUE;
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
        case kSidebarItemAnimeList:
          return DlgAnimeList.SendMessage(
            pMsg->message, pMsg->wParam, pMsg->lParam);
        case kSidebarItemHistory:
          return DlgHistory.SendMessage(
            pMsg->message, pMsg->wParam, pMsg->lParam);
        case kSidebarItemStats:
          return DlgStats.SendMessage(
            pMsg->message, pMsg->wParam, pMsg->lParam);
        case kSidebarItemSearch:
          return DlgSearch.SendMessage(
            pMsg->message, pMsg->wParam, pMsg->lParam);
        case kSidebarItemSeasons:
          return DlgSeason.SendMessage(
            pMsg->message, pMsg->wParam, pMsg->lParam);
        case kSidebarItemFeeds:
          return DlgTorrent.SendMessage(
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

  if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) {
    switch (pMsg->wParam) {
      case VK_ESCAPE:
      case VK_RETURN:
        break;  // We don't want to close the dialogs
      default:
        if (!IsModal() && IsDialogMessage(GetWindowHandle(), pMsg)) {
          return TRUE;
        }
    }
  }

  return FALSE;
}

////////////////////////////////////////////////////////////////////////////////

BOOL MainDialog::OnClose() {
  if (Settings.GetBool(taiga::kApp_Behavior_CloseToTray)) {
    Hide();
    return TRUE;
  }

  return FALSE;
}

BOOL MainDialog::OnDestroy() {
  if (Settings.GetBool(taiga::kApp_Position_Remember)) {
    WINDOWPLACEMENT wp = {0};
    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetPlacement(wp)) {
      bool maximized = wp.showCmd == SW_SHOWMAXIMIZED;
      bool restore_to_maximized = (wp.showCmd == SW_SHOWMINIMIZED) &&
                                  (wp.flags & WPF_RESTORETOMAXIMIZED);
      Settings.Set(taiga::kApp_Position_Maximized, maximized || restore_to_maximized);
      win::Rect rcWindow(wp.rcNormalPosition);
      Settings.Set(taiga::kApp_Position_X, rcWindow.left);
      Settings.Set(taiga::kApp_Position_Y, rcWindow.top);
      Settings.Set(taiga::kApp_Position_W, rcWindow.Width());
      Settings.Set(taiga::kApp_Position_H, rcWindow.Height());
    }
  }

  ui::DestroyDialog(ui::kDialogAbout);
  ui::DestroyDialog(ui::kDialogAnimeInformation);
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
    track::recognition::ParseOptions parse_options;
    Meow.Parse(buffer, parse_options, episode);
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

  // Statusbar control
  } else if (idCtrl == IDC_STATUSBAR_MAIN) {
    return OnStatusbarNotify(reinterpret_cast<LPARAM>(pnmh));

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
    case WM_SIZE: {
      if (nType == SIZE_MINIMIZED) {
        if (Settings.GetBool(taiga::kApp_Behavior_MinimizeToTray))
          Hide();
        return;
      }
      UpdateControlPositions(&size);
      break;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

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
        break;
      }
      case NIN_BALLOONTIMEOUT: {
        Taiga.current_tip_type = taiga::kTipTypeDefault;
        break;
      }
      case NIN_BALLOONUSERCLICK: {
        switch (Taiga.current_tip_type) {
          case taiga::kTipTypeNowPlaying:
            navigation.SetCurrentPage(kSidebarItemNowPlaying);
            break;
          case taiga::kTipTypeSearch:
            ExecuteAction(L"SearchAnime(" + CurrentEpisode.anime_title() + L")");
            break;
          case taiga::kTipTypeTorrent:
            navigation.SetCurrentPage(kSidebarItemFeeds);
            break;
          case taiga::kTipTypeUpdateFailed:
            History.queue.Check(false);
            break;
          case taiga::kTipTypeNotApproved:
            navigation.SetCurrentPage(kSidebarItemHistory);
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
        SetForegroundWindow();
        ui::Menus.UpdateAll();
        ExecuteAction(ui::Menus.Show(GetWindowHandle(), 0, 0, L"Tray"));
        break;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void MainDialog::ChangeStatus(std::wstring str) {
  if (!str.empty())
    str = L" " + str;  // padding
  statusbar.SetText(str.c_str());
}

void MainDialog::EnableInput(bool enable) {
  // Toolbar buttons
  toolbar_main.EnableButton(kToolbarButtonSync, enable);
  // Content
  DlgAnimeList.Enable(enable);
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
  rect_sidebar_.Set(0, rect_client.top, ScaleX(140), rect_client.bottom);
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
  DlgAnimeList.SetPosition(nullptr, rect_content_);
  DlgHistory.SetPosition(nullptr, rect_content_);
  DlgNowPlaying.SetPosition(nullptr, rect_content_);
  DlgSearch.SetPosition(nullptr, rect_content_);
  DlgSeason.SetPosition(nullptr, rect_content_);
  DlgStats.SetPosition(nullptr, rect_content_);
  DlgTorrent.SetPosition(nullptr, rect_content_);
}

void MainDialog::UpdateStatusTimer() {
  win::Rect rect;
  GetClientRect(&rect);

  auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);

  if (anime_item && IsUpdateAllowed(*anime_item, CurrentEpisode, true)) {
    auto timer = taiga::timers.timer(taiga::kTimerMedia);
    int seconds = timer ? timer->ticks() : 0;
    bool waiting_for_media_player = seconds == 0 &&
        Settings.GetBool(taiga::kSync_Update_WaitPlayer);

    std::wstring str = L"List update in " + ToTimeString(seconds);
    if (waiting_for_media_player)
      str += L" (waiting for media player to close)";

    statusbar.SetPartText(1, str.c_str());
    statusbar.SetPartTipText(1, str.c_str());
    statusbar.SetPartTipText(2, L"Cancel update");

    const int icon_width = ScaleX(16);
    win::Dc hdc = statusbar.GetDC();
    hdc.AttachFont(statusbar.GetFont());
    const int timer_width = ScaleX(5) + icon_width + ScaleX(5) + GetTextWidth(hdc.Get(), str) + ScaleX(4);
    hdc.DetachFont();
    const int cancel_width = ScaleX(5) + icon_width + ScaleX(16);

    statusbar.SetPartWidth(0, rect.Width() - timer_width - cancel_width);
    statusbar.SetPartWidth(1, timer_width);
    statusbar.SetPartWidth(2, cancel_width);

  } else {
    statusbar.SetPartWidth(0, rect.Width());
    statusbar.SetPartWidth(1, 0);
    statusbar.SetPartWidth(2, 0);
  }
}

void MainDialog::UpdateTip() {
  std::wstring tip = TAIGA_APP_TITLE;
  if (Taiga.debug_mode)
    tip += L" [debug]";

  auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);
  if (anime_item) {
    tip += L"\nWatching: " + anime_item->GetTitle() +
           PushString(L" #", anime::GetEpisodeRange(CurrentEpisode));
  }

  Taskbar.Modify(tip.c_str());
}

void MainDialog::UpdateTitle() {
  std::wstring title = TAIGA_APP_TITLE;
  if (Taiga.debug_mode)
    title += L" [debug]";

  const std::wstring username = taiga::GetCurrentUsername();
  if (!username.empty())
    title += L" \u2013 " + username;
  if (Taiga.debug_mode) {
    auto service = taiga::GetCurrentService();
    if (service)
      title += L" @ " + service->name();
  }

  auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);
  if (anime_item) {
    title += L" \u2013 " + anime_item->GetTitle() +
             PushString(L" #", anime::GetEpisodeRange(CurrentEpisode));
    if (Settings.GetBool(taiga::kSync_Update_OutOfRange) &&
        anime::GetEpisodeLow(CurrentEpisode) > anime_item->GetMyLastWatchedEpisode() + 1) {
      title += L" (out of range)";
    }
  }

  SetText(title);
}

////////////////////////////////////////////////////////////////////////////////

int MainDialog::Navigation::GetCurrentPage() {
  return current_page_;
}

void MainDialog::Navigation::SetCurrentPage(int page, bool add_to_history) {
  if (page == current_page_)
    return;

  int previous_page = current_page_;
  current_page_ = page;

  RefreshSearchText(previous_page);

  #define DISPLAY_PAGE(item, dialog, resource_id) \
    case item: \
      if (!dialog.IsWindow()) dialog.Create(resource_id, parent->GetWindowHandle(), false); \
      parent->UpdateControlPositions(); \
      dialog.Show(); \
      break;
  switch (current_page_) {
    DISPLAY_PAGE(kSidebarItemNowPlaying, DlgNowPlaying, IDD_ANIME_INFO);
    DISPLAY_PAGE(kSidebarItemAnimeList, DlgAnimeList, IDD_ANIME_LIST);
    DISPLAY_PAGE(kSidebarItemHistory, DlgHistory, IDD_HISTORY);
    DISPLAY_PAGE(kSidebarItemStats, DlgStats, IDD_STATS);
    DISPLAY_PAGE(kSidebarItemSearch, DlgSearch, IDD_SEARCH);
    DISPLAY_PAGE(kSidebarItemSeasons, DlgSeason, IDD_SEASON);
    DISPLAY_PAGE(kSidebarItemFeeds, DlgTorrent, IDD_TORRENT);
  }
  #undef DISPLAY_PAGE

  if (current_page_ != kSidebarItemNowPlaying) DlgNowPlaying.Hide();
  if (current_page_ != kSidebarItemAnimeList) DlgAnimeList.Hide();
  if (current_page_ != kSidebarItemHistory) DlgHistory.Hide();
  if (current_page_ != kSidebarItemStats) DlgStats.Hide();
  if (current_page_ != kSidebarItemSearch) DlgSearch.Hide();
  if (current_page_ != kSidebarItemSeasons) DlgSeason.Hide();
  if (current_page_ != kSidebarItemFeeds) DlgTorrent.Hide();

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
  if (index_ < static_cast<int>(items_.size()) - 1) {
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

void MainDialog::Navigation::RefreshSearchText(int previous_page) {
  std::wstring cue_text;
  switch (current_page_) {
    case kSidebarItemAnimeList:
    case kSidebarItemSeasons:
      parent->search_bar.mode = kSearchModeService;
      cue_text = L"Filter list or search " + taiga::GetCurrentService()->name();
      break;
    case kSidebarItemNowPlaying:
    case kSidebarItemHistory:
    case kSidebarItemStats:
    case kSidebarItemSearch:
      parent->search_bar.mode = kSearchModeService;
      cue_text = L"Search " + taiga::GetCurrentService()->name() + L" for anime";
      break;
    case kSidebarItemFeeds:
      parent->search_bar.mode = kSearchModeFeed;
      cue_text = L"Search for torrents";
      break;
  }
  parent->edit.SetCueBannerText(cue_text.c_str());

  switch (current_page_) {
    case kSidebarItemAnimeList:
    case kSidebarItemSeasons:
    case kSidebarItemSearch:
    case kSidebarItemFeeds:
      parent->search_bar.filters.text = parent->search_bar.text[current_page_];
      break;
    default:
      parent->search_bar.filters.text.clear();
      parent->search_bar.text[current_page_].clear();
      break;
  }
  parent->edit.SetText(parent->search_bar.text[current_page_]);
}

}  // namespace ui