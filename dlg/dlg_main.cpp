/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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
#include "../animelist.h"
#include "../announce.h"
#include "../common.h"
#include "dlg_main.h"
#include "dlg_torrent.h"
#include "../gfx.h"
#include "../http.h"
#include "../media.h"
#include "../myanimelist.h"
#include "../process.h"
#include "../recognition.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"
#include "../ui/ui_gdi.h"
#include "../ui/ui_taskbar.h"
#include "../ui/ui_taskdialog.h"

CMainWindow MainWindow;

// =============================================================================

CMainWindow::CMainWindow() {
  RegisterDlgClass(L"TaigaMainW");
  m_iSearchMode = SEARCH_MODE_LIST;
}

BOOL CMainWindow::OnInitDialog() {
  // Set global variables
  g_hMain = GetWindowHandle();
  
  // Set member variables
  CRect rect; GetWindowRect(&rect);
  SetSizeMin(rect.Width(), rect.Height());
  SetSnapGap(10);

  // Set icon
  SetIconLarge(IDI_MAIN);
  SetIconSmall(IDI_MAIN);
  
  // Create rebar
  m_Rebar.Attach(GetDlgItem(IDC_REBAR_MAIN));
  // Create main toolbar
  m_Toolbar.Attach(GetDlgItem(IDC_TOOLBAR_MAIN));
  m_Toolbar.SetImageList(UI.ImgList24.GetHandle(), 24, 24);
  m_Toolbar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  // Create search toolbar
  m_ToolbarSearch.Attach(GetDlgItem(IDC_TOOLBAR_SEARCH));
  m_ToolbarSearch.SetImageList(UI.ImgList16.GetHandle(), 16, 16);
  m_ToolbarSearch.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  // Create search text
  m_EditSearch.Attach(GetDlgItem(IDC_EDIT_SEARCH));
  m_EditSearch.SetCueBannerText(L"Search list");
  m_EditSearch.SetParent(m_ToolbarSearch.GetWindowHandle());
  m_EditSearch.SetPosition(NULL, ScaleX(32), 1, 140, 20);
  m_EditSearch.SetMargins(1, 16);
  CRect rcEdit; m_EditSearch.GetRect(&rcEdit);
  // Create cancel search button
  m_CancelSearch.Attach(GetDlgItem(IDC_BUTTON_CANCELSEARCH));
  m_CancelSearch.SetParent(m_EditSearch.GetWindowHandle());
  m_CancelSearch.SetPosition(NULL, rcEdit.right + 1, 0, 16, 16);
  //m_CancelSearch.SetClassLong(GCL_HCURSOR, reinterpret_cast<LONG>(::LoadImage(NULL, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_SHARED)));
  // Create tab control
  m_Tab.Attach(GetDlgItem(IDC_TAB_MAIN));
  // Create main list
  m_List.Attach(GetDlgItem(IDC_LIST_MAIN));
  m_List.SetExtendedStyle(LVS_EX_AUTOSIZECOLUMNS | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  m_List.SetImageList(UI.ImgList16.GetHandle());
  m_List.SetTheme();
  // Create status bar
  m_Status.Attach(GetDlgItem(IDC_STATUSBAR_MAIN));
  m_Status.SetImageList(UI.ImgList16.GetHandle());
  //m_Status.InsertPart(-1, 0, 0, 700, NULL, NULL);
  //m_Status.InsertPart(-1, 0, 0, 750, NULL, NULL);

  // Insert list columns
  m_List.InsertColumn(0, GetSystemMetrics(SM_CXSCREEN), 370, LVCFMT_LEFT, L"Anime title");
  m_List.InsertColumn(1, 130, 130, LVCFMT_CENTER, L"Progress");
  m_List.InsertColumn(2,  62,  62, LVCFMT_CENTER, L"Score");
  m_List.InsertColumn(3,  62,  62, LVCFMT_CENTER, L"Type");
  m_List.InsertColumn(4, 105, 105, LVCFMT_RIGHT,  L"Season");

  // Insert main toolbar buttons
  BYTE fsStyle1 = BTNS_AUTOSIZE;
  BYTE fsStyle2 = BTNS_AUTOSIZE | BTNS_WHOLEDROPDOWN;
  m_Toolbar.InsertButton(0, Icon24_Offline,  100, 1, fsStyle1, 0, NULL, L"Log in");
  m_Toolbar.InsertButton(1, Icon24_Sync,     101, 1, fsStyle1, 1, NULL, L"Synchronize list");
  m_Toolbar.InsertButton(2, Icon24_MAL,      102, 1, fsStyle1, 2, NULL, L"View your panel at MyAnimeList");
  m_Toolbar.InsertButton(3, 0, 0, 0, BTNS_SEP, NULL, NULL, NULL);
  m_Toolbar.InsertButton(4, Icon24_Folders,  104, 1, fsStyle2, 4, NULL, L"Anime folders");
  m_Toolbar.InsertButton(5, Icon24_Tools,    105, 1, fsStyle2, 5, NULL, L"Tools");
  m_Toolbar.InsertButton(6, Icon24_RSS,      106, 1, fsStyle1, 6, NULL, L"Torrents");
  m_Toolbar.InsertButton(7, 0, 0, 0, BTNS_SEP, NULL, NULL, NULL);
  m_Toolbar.InsertButton(8, Icon24_Filter,   108, 1, fsStyle1, 8, NULL, L"Filter list");
  m_Toolbar.InsertButton(9, Icon24_Settings, 109, 1, fsStyle1, 9, NULL, L"Change program settings");
  #ifdef _DEBUG
  m_Toolbar.InsertButton(10, 0, 0, 0, BTNS_SEP, NULL, NULL, NULL);
  m_Toolbar.InsertButton(11, Icon24_About, 111, 1, fsStyle1, 11, NULL, L"About Taiga");
  #endif
  // Insert search toolbar button
  m_ToolbarSearch.InsertButton(0, Icon16_Search, 200, 1, fsStyle2, NULL, NULL, L"Search");

  // Insert rebar bands
  UINT fMask  = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
  UINT fStyle = RBBS_NOGRIPPER;
  m_Rebar.InsertBand(NULL, 0, 0, 0, 0, 0, 0, 0, 0, fMask, fStyle);
  m_Rebar.InsertBand(m_Toolbar.GetWindowHandle(), GetSystemMetrics(SM_CXSCREEN), 0, 0, 0, 0, 0, 0, 
    HIWORD(m_Toolbar.GetButtonSize()) + (HIWORD(m_Toolbar.GetPadding()) / 2), fMask, fStyle);
  m_Rebar.InsertBand(m_ToolbarSearch.GetWindowHandle(), 0, 0, 0, 180, 0, 0, 0, 
    HIWORD(m_ToolbarSearch.GetButtonSize()), fMask, fStyle);

  // Insert tabs and list groups
  for (int i = MAL_WATCHING; i <= MAL_PLANTOWATCH; i++) {
    if (i != MAL_UNKNOWN) {
      m_Tab.InsertItem(i - 1, MAL.TranslateMyStatus(i, true).c_str(), (LPARAM)i);
      m_List.InsertGroup(i, MAL.TranslateMyStatus(i, false).c_str());
    }
  }

  // Start process timer
  SetTimer(g_hMain, TIMER_MAIN, 1000, NULL);
  // Add icon to taskbar
  Taskbar.Create(g_hMain, NULL, APP_TITLE);
  UpdateTip();
  // Change status
  ChangeStatus(DEFAULT_STATUS);
  // Refresh list
  RefreshList(MAL_WATCHING);
  // Refresh menu bar
  RefreshMenubar();

  return TRUE;
}

// =============================================================================

BOOL CMainWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Drag  list item
    case WM_MOUSEMOVE: {
      if (m_List.m_bDragging) {
        m_List.m_DragImage.DragMove(LOWORD(lParam) + 16, HIWORD(lParam) + 24);
        SetCursor(LoadCursor(NULL, m_Tab.HitTest() > -1 ? IDC_ARROW : IDC_NO));
      }
      break;
    }
    case WM_LBUTTONUP: {
      if (m_List.m_bDragging) {
        m_List.m_DragImage.DragLeave(g_hMain);
        m_List.m_DragImage.EndDrag();
        m_List.m_DragImage.Destroy();
        m_List.m_bDragging = false;
        ReleaseCapture();
        int tab_index = m_Tab.HitTest();
        if (tab_index > -1) {
          ExecuteAction(L"EditStatus(" + ToWSTR(m_Tab.GetItemParam(tab_index)) + L")");
        }
      }
      break;
    }

    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL: {
      return m_List.SendMessage(uMsg, wParam, lParam);
    }
    
    // External programs
    case WM_COPYDATA: {
      PCOPYDATASTRUCT pCDS = (PCOPYDATASTRUCT)lParam;
      // Skype
      if (reinterpret_cast<HWND>(wParam) == Skype.m_APIWindowHandle) {
        return TRUE; // pCDS->lpData is the response

      // JetAudio
      } else if (pCDS->dwData == 0x3000 /* JRC_COPYDATA_ID_TRACK_FILENAME */) {
        MediaPlayers.NewCaption = ToUTF8(reinterpret_cast<LPCSTR>(pCDS->lpData));
        return TRUE;

      // Media Portal
      } else if (pCDS->dwData == 0x1337) {
        MediaPlayers.NewCaption = ToUTF8(reinterpret_cast<LPCSTR>(pCDS->lpData));
        return TRUE;
      }
      break;
    }
    default: {
      // Skype
      if (uMsg == Skype.m_uControlAPIAttach) {
        switch (lParam) {
          case 0: // ATTACH_SUCCESS
            #ifdef _DEBUG
            ChangeStatus(L"Skype attach succeeded.");
            #endif
            Skype.m_APIWindowHandle = reinterpret_cast<HWND>(wParam);
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

BOOL CMainWindow::PreTranslateMessage(MSG* pMsg) {
  switch (pMsg->message) {
    case WM_KEYDOWN: {
      if (::GetFocus() == m_EditSearch.GetWindowHandle()) {
        switch (pMsg->wParam) {
          // Clear search text
          case VK_ESCAPE: {
            m_EditSearch.SetText(L"");
            return TRUE;
          }
          // Search
          case VK_RETURN: {
            switch (MainWindow.GetSearchMode()) {
              case SEARCH_MODE_MAL: {
                wstring text;
                m_EditSearch.GetText(text);
                switch (Settings.Account.MAL.API) {
                  case MAL_API_OFFICIAL: {
                    ExecuteAction(L"SearchAnime(" + text + L")");
                    return TRUE;
                  }
                  case MAL_API_NONE: {
                    MAL.ViewAnimeSearch(text); // TEMP
                    return TRUE;
                  }
                }
                break;
              }
            }
            break;
          }
        }
      }
      break;
    }
  }

  return FALSE;
}

// =============================================================================

BOOL CMainWindow::OnClose() {
  if (Settings.Program.General.Close) {
    Show(SW_HIDE);
    return TRUE;
  }
  if (Settings.Program.Exit.Ask) {
    CTaskDialog dlg;
    dlg.SetWindowTitle(APP_TITLE);
    dlg.SetMainIcon(TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Are you sure you want to exit?");
    dlg.AddButton(L"Yes", IDYES);
    dlg.AddButton(L"No", IDNO);
    dlg.Show(g_hMain);
    if (dlg.GetSelectedButtonID() != IDYES) return TRUE;
  }
  return FALSE;
}

void CMainWindow::OnDestroy() {
  // Announce
  if (Taiga.PlayStatus == PLAYSTATUS_PLAYING) {
    Taiga.PlayStatus = PLAYSTATUS_STOPPED;
    ExecuteAction(L"AnnounceToHTTP", TRUE, reinterpret_cast<LPARAM>(&CurrentEpisode));
  }
  ExecuteAction(L"AnnounceToMessenger", FALSE);
  ExecuteAction(L"AnnounceToSkype", FALSE);
  // Cleanup
  MainClient.Cleanup();
  Settings.Write();
  Taskbar.Destroy();
  TaskbarList.Release();
  // Exit
  Taiga.PostQuitMessage();
}

void CMainWindow::OnDropFiles(HDROP hDropInfo) {
  #ifdef _DEBUG
  WCHAR buffer[MAX_PATH];
  if (DragQueryFile(hDropInfo, 0, buffer, MAX_PATH) > 0) {
    CEpisode episode;
    Meow.ExamineTitle(buffer, episode, true, true, true, true, true); 
    MessageBox(ReplaceVariables(Settings.Program.Balloon.Format, episode).c_str(), APP_TITLE, MB_OK);
  }
  #endif
}

LRESULT CMainWindow::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // ListView control
  if (idCtrl == IDC_LIST_MAIN || pnmh->hwndFrom == m_List.GetHeader()) {
    return OnListNotify(reinterpret_cast<LPARAM>(pnmh));
  
  // Tab control
  } else if (idCtrl == IDC_TAB_MAIN) {
    return OnTabNotify(reinterpret_cast<LPARAM>(pnmh));

  // Toolbar controls
  } else if (idCtrl == IDC_TOOLBAR_MAIN || idCtrl == IDC_TOOLBAR_SEARCH) {
    return OnToolbarNotify(reinterpret_cast<LPARAM>(pnmh));

  // Button control
  } else if (idCtrl == IDC_BUTTON_CANCELSEARCH) {
    if (pnmh->code == NM_CUSTOMDRAW) {
      return OnButtonCustomDraw(reinterpret_cast<LPARAM>(pnmh));
    }
  }
  
  return 0;
}

void CMainWindow::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_ENTERSIZEMOVE: {
      if (::IsAppThemed() && GetWinVersion() >= WINVERSION_VISTA) {
        SetTransparency(200);
      }
      break;
    }
    case WM_EXITSIZEMOVE: {
      if (::IsAppThemed() && GetWinVersion() >= WINVERSION_VISTA) {
        SetTransparency(255);
      }
      break;
    }
    case WM_SIZE: {
      if (IsIconic()) {
        if (Settings.Program.General.Minimize) Show(SW_HIDE);
        return;
      }
      
      CRect rcWindow;
      rcWindow.Set(0, 0, size.cx, size.cy);
      rcWindow.Inflate(-ScaleX(CONTROL_MARGIN), -ScaleY(CONTROL_MARGIN));
      // Resize rebar
      m_Rebar.SendMessage(WM_SIZE, 0, 0);
      rcWindow.top += m_Rebar.GetBarHeight() + 2;
      // Resize status bar
      CRect rcStatus;
      m_Status.GetClientRect(&rcStatus);
      m_Status.SendMessage(WM_SIZE, 0, 0);
      rcWindow.bottom -= rcStatus.Height();
      // Resize tab
      m_Tab.SetPosition(NULL, rcWindow);
      // Resize list
      m_Tab.AdjustRect(FALSE, &rcWindow);
      rcWindow.left -= 3; rcWindow.top -= 1;
      m_List.SetPosition(NULL, rcWindow, 0);
    }
  }
}

// =============================================================================

/* Timer */

// This function is very delicate, even order of things are important.
// Please be careful with what you change.

void CMainWindow::OnTimer(UINT_PTR nIDEvent) {
  // Check new torrents
  if (Settings.RSS.Torrent.CheckEnabled && Settings.RSS.Torrent.CheckInterval) {
    if (Taiga.TickerTorrent >= Settings.RSS.Torrent.CheckInterval * 60) {
      Torrents.Check(Settings.RSS.Torrent.Source);
    } else {
      Taiga.TickerTorrent++;
      if (TorrentWindow.IsWindow()) {
        wstring text = L"Check new torrents [" + 
          ToTimeString(Settings.RSS.Torrent.CheckInterval * 60 - Taiga.TickerTorrent) + L"]";
        TorrentWindow.m_Toolbar.SetButtonText(0, text.c_str());
      }
    }
  } else {
    if (TorrentWindow.IsWindow()) {
      TorrentWindow.m_Toolbar.SetButtonText(0, L"Check new torrents");
    }
  }
  
  // Check process list for media players
  int media_index = MediaPlayers.Check();
  int anime_index = CurrentEpisode.Index;

  // Media player is running
  if (media_index > -1) {
    // Started to watch?
    if (anime_index == 0) {
      // Recognized?
      if (Meow.ExamineTitle(MediaPlayers.CurrentCaption, CurrentEpisode, true, true, true, true, true)) {
        for (int i = AnimeList.Count; i > 0; i--) {
          if (Meow.CompareTitle(CurrentEpisode.Title, i)) {
            CurrentEpisode.Index = i;
            RefreshMenubar(CurrentEpisode.Index);
            AnimeList.Item[i].Start(CurrentEpisode);
            return;
          }
        }
      }
      // Not recognized
      CurrentEpisode.Index = -1;
      RefreshMenubar(CurrentEpisode.Index);
      if (CurrentEpisode.Title.empty()) {
        ChangeStatus(MediaPlayers.Item[MediaPlayers.Index].Name + L" is running.");
      } else {
        ChangeStatus(L"Watching: " + CurrentEpisode.Title + 
          PushString(L" #", CurrentEpisode.Number) + L" (Not recognized)");
        wstring tip_text = ReplaceVariables(Settings.Program.Balloon.Format, CurrentEpisode);
        tip_text += L"\nClick here to search MyAnimeList for this anime.";
        Taiga.CurrentTipType = TIPTYPE_SEARCH;
        Taskbar.Tip(L"", L"", 0);
        Taskbar.Tip(tip_text.c_str(), L"Media is not in your list", NIIF_WARNING);
      }

    // Already watching or not recognized before
    } else {
      // Tick and compare with delay time
      if (Taiga.Ticker > -1 && Taiga.Ticker <= Settings.Account.Update.Delay) {
        if (Taiga.Ticker == Settings.Account.Update.Delay) {
          // Disable ticker
          Taiga.Ticker = -1;
          // Announce current episode
          ExecuteAction(L"AnnounceToHTTP", TRUE, reinterpret_cast<LPARAM>(&CurrentEpisode));
          ExecuteAction(L"AnnounceToMessenger", TRUE, reinterpret_cast<LPARAM>(&CurrentEpisode));
          ExecuteAction(L"AnnounceToMIRC", TRUE, reinterpret_cast<LPARAM>(&CurrentEpisode));
          ExecuteAction(L"AnnounceToSkype", TRUE, reinterpret_cast<LPARAM>(&CurrentEpisode));
          // Update
          if (Settings.Account.Update.Time == UPDATE_MODE_AFTERDELAY && anime_index > 0) {
            AnimeList.Item[anime_index].End(CurrentEpisode, false, true);
          }
          return;
        }
        if (Settings.Account.Update.CheckPlayer == FALSE || 
          MediaPlayers.Item[media_index].WindowHandle == GetForegroundWindow()) {
            Taiga.Ticker++;
            //wstring text = L" " + ToTimeString(Settings.Account.Update.Delay - Taiga.Ticker);
            //m_Status.SetPanelText(1, text.c_str());
        }
      }
      // Caption changed?
      if (MediaPlayers.TitleChanged() == true) {
        CurrentEpisode.Index = 0;
        RefreshMenubar(CurrentEpisode.Index);
        if (anime_index > 0) {
          AnimeList.Item[anime_index].End(CurrentEpisode, true, true);
        }
        Taiga.Ticker = 0;
        //m_Status.SetPanelText(1, L" 00:00");
      }
    }
  
  // Media player is NOT running
  } else {
    // Was running, but not watching
    if (anime_index < 1) {
      if (MediaPlayers.IndexOld > 0){
        ChangeStatus(DEFAULT_STATUS);
        CurrentEpisode.Index = 0;
        MediaPlayers.IndexOld = 0;
        RefreshMenubar(CurrentEpisode.Index);
      }
    
    // Was running and watching
    } else {
      CurrentEpisode.Index = 0;
      RefreshMenubar(CurrentEpisode.Index);
      AnimeList.Item[anime_index].End(CurrentEpisode, true, 
        Settings.Account.Update.Time == UPDATE_MODE_WAITPLAYER);
      Taiga.Ticker = 0;
      //m_Status.SetPanelText(1, L" 00:00");
    }
  }
}

// =============================================================================

/* Taskbar */

void CMainWindow::OnTaskbarCallback(UINT uMsg, LPARAM lParam) {
  // Taskbar creation notification
  if (uMsg == WM_TASKBARCREATED) {
    Taskbar.Create(m_hWindow, NULL, APP_TITLE);
  
  // Windows 7 taskbar interface
  } else if (uMsg == WM_TASKBARBUTTONCREATED) {
    TaskbarList.Initialize(m_hWindow);

  // Taskbar callback
  } else if (uMsg == WM_TASKBARCALLBACK) {
    switch (lParam) {
      case NIN_BALLOONSHOW: {
        DEBUG_PRINT(L"Tip type: " + ToWSTR(Taiga.CurrentTipType) + L"\n");
        break;
      }
      case NIN_BALLOONTIMEOUT: {
        Taiga.CurrentTipType = TIPTYPE_NORMAL;
        break;
      }
      case NIN_BALLOONUSERCLICK: {
        switch (Taiga.CurrentTipType) {
          case TIPTYPE_SEARCH:
            ExecuteAction(L"SearchAnime(" + CurrentEpisode.Title + L")");
            break;
          case TIPTYPE_TORRENT:
            ExecuteAction(L"Torrents");
            break;
        }
        Taiga.CurrentTipType = TIPTYPE_NORMAL;
        break;
      }
      case WM_LBUTTONDBLCLK: {
        ActivateWindow(m_hWindow);
        break;
      }
      case WM_RBUTTONDOWN: {
        UpdateAllMenus(AnimeList.Index);
        SetForegroundWindow();
        ExecuteAction(UI.Menus.Show(m_hWindow, 0, 0, L"Tray"));
        RefreshMenubar(AnimeList.Index);
        break;
      }
    }
  }
}

// =============================================================================

void CMainWindow::ChangeStatus(wstring str) {
  // Change status text
  if (!str.empty()) {
    str = L"  " + str;
    m_Status.SetText(str.c_str());
  }
}

int CMainWindow::GetListIndex(int anime_index) {
  CAnime* pItem;
  for (int i = 0; i < m_List.GetItemCount(); i++) {
    pItem = reinterpret_cast<CAnime*>(m_List.GetItemParam(i));
    if (pItem && pItem->Index == anime_index) return i;
  }
  return -1;
}

void CMainWindow::RefreshList(int index) {
  // Change window title
  wstring title = APP_TITLE;
  if (!Settings.Account.MAL.User.empty()) {
    title += L" - " + Settings.Account.MAL.User + L"'";
    title += EndsWith(Settings.Account.MAL.User, L"s") ? L"" : L"s";
    title += L" Anime List";
  }
  SetText(title.c_str());

  // Hide list to avoid visual defects and gain performance
  m_List.Show(SW_HIDE);
  m_List.EnableGroupView(index == 0);
  m_List.DeleteAllItems();

  // Remember last index
  static int last_index = 0;
  if (index == -1) index = last_index;
  if (index > 0) last_index = index;

  // Add items
  int icon_index = 0;
  int group_count[6] = {0};
  for (int i = 1; i <= AnimeList.Count; i++) {
    if (AnimeList.Item[i].My_Status == index || index == 0) {
      if (AnimeList.Filter.Check(i)) {
        icon_index = AnimeList.Item[i].Playing ? Icon16_Play : StatusToIcon(AnimeList.Item[i].Series_Status);
        group_count[AnimeList.Item[i].My_Status - 1]++;
        int j = m_List.InsertItem(i, AnimeList.Item[i].My_Status, icon_index, LPSTR_TEXTCALLBACK, 
          reinterpret_cast<LPARAM>(&AnimeList.Item[i]));
        int eps_estimate = AnimeList.Item[i].EstimateTotalEpisodes();
        float ratio = eps_estimate ? (float)AnimeList.Item[i].My_WatchedEpisodes / (float)eps_estimate : 0.8f;
        m_List.SetItem(j, 1, ToWSTR(ratio, 4).c_str());
        m_List.SetItem(j, 2, MAL.TranslateNumber(AnimeList.Item[i].My_Score).c_str());
        m_List.SetItem(j, 3, MAL.TranslateType(AnimeList.Item[i].Series_Type).c_str());
        m_List.SetItem(j, 4, MAL.TranslateDate(AnimeList.Item[i].Series_Start).c_str());
      }
    }
  }

  // Set group headers
  for (int i = MAL_WATCHING; i <= MAL_PLANTOWATCH; i++) {
    if (index == 0 && i != MAL_UNKNOWN) {
      wstring text = MAL.TranslateMyStatus(i, false);
      text += group_count[i - 1] > 0 ? L" (" + ToWSTR(group_count[i - 1]) + L")" : L"";
      m_List.SetGroupText(i, text.c_str());
    }
  }

  // Sort items
  m_List.Sort(0, 1, 0, ListViewCompareProc);

  // Show again
  m_List.Show(SW_SHOW);
}

void CMainWindow::RefreshMenubar(int index, bool show) {
  if (show) {
    UpdateAllMenus(index);
    vector<HMENU> hMenu;
    UI.Menus.CreateNewMenu(L"Main", hMenu);
    if (!hMenu.empty()) {
      DestroyMenu(GetMenu());
      SetMenu(hMenu[0]);
    }
  } else {
    SetMenu(NULL);
  }
}

void CMainWindow::RefreshTabs(int index, bool redraw) {
  // Remember last index
  static int last_index = 1;
  if (index == -1) index = last_index;
  if (index == 6) index--;
  last_index = index;
  if (!redraw) return;
  
  // Hide
  m_Tab.Show(SW_HIDE);

  // Refresh text
  for (int i = 1; i <= 6; i++) {
    if (i != 5) {
      m_Tab.SetItemText(i == 6 ? 4 : i - 1, MAL.TranslateMyStatus(i, true).c_str());
    }
  }

  // Select related tab
  m_Tab.SetCurrentlySelected(--index);

  // Show again
  m_Tab.Show(SW_SHOW);
}

void CMainWindow::SetSearchMode(UINT mode) {
  m_iSearchMode = mode;
  switch (mode) {
    case SEARCH_MODE_LIST:
      m_EditSearch.SetCueBannerText(L"Search list");
      m_EditSearch.GetText(AnimeList.Filter.Text);
      if (!AnimeList.Filter.Text.empty()) {
        RefreshList(0);
      }
      break;
    case SEARCH_MODE_MAL:
      m_EditSearch.SetCueBannerText(L"Search MyAnimeList");
      if (!AnimeList.Filter.Text.empty()) {
        AnimeList.Filter.Text.clear();
        RefreshList();
      }
      break;
  }
}

void CMainWindow::UpdateTip() {
  wstring tip = APP_TITLE;
  if (Taiga.LoggedIn) {
    tip += L"\n" + AnimeList.User.Name + L" is logged in.";
  }
  if (CurrentEpisode.Index > 0) {
    tip += L"\nWatching: " + AnimeList.Item[CurrentEpisode.Index].Series_Title + 
      (!CurrentEpisode.Number.empty() ? L" #" + CurrentEpisode.Number : L"");
  }
  Taskbar.Modify(tip.c_str());
}