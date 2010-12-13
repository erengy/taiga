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
#include "../common.h"
#include "dlg_event.h"
#include "dlg_input.h"
#include "dlg_main.h"
#include "dlg_search.h"
#include "dlg_settings.h"
#include "../http.h"
#include "../media.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../theme.h"
#include "../win32/win_control.h"

LPCWSTR PAGE_TITLE[PAGE_COUNT] = {
  L" MyAnimeList",
  L" Update",
  L" HTTP announcement",
  L" Messenger announcement",
  L" mIRC announcement",
  L" Skype announcement",
  L" Twitter announcement",
  L" Root folders",
  L" Specific folders",
  L" Program",
  L" List",
  L" Notifications",
  L" Media players",
  L" Torrent options",
  L" Torrent filters"
};

CSettingsWindow SettingsWindow;

// =============================================================================

CSettingsWindow::CSettingsWindow() :
  m_hStaticFont(NULL), m_iCurrentPage(PAGE_ACCOUNT)
{
  RegisterDlgClass(L"TaigaSettingsW");

  m_Page.resize(PAGE_COUNT);
  for (size_t i = 0; i < PAGE_COUNT; i++) {
    m_Page[i].Index = i;
  }
}

CSettingsWindow::~CSettingsWindow() {
  if (m_hStaticFont) {
    ::DeleteObject(m_hStaticFont);
    m_hStaticFont = NULL;
  }
}

void CSettingsWindow::SetCurrentPage(int index) {
  m_iCurrentPage = index;
  if (IsWindow()) m_Page[index].Select();
}

// =============================================================================

BOOL CSettingsWindow::OnInitDialog() {
  // Set title font
  if (!m_hStaticFont) {
    LOGFONT lFont;
    m_hStaticFont = GetFont();
    ::GetObject(m_hStaticFont, sizeof(LOGFONT), &lFont);
    lFont.lfWeight = FW_BOLD;
    m_hStaticFont = ::CreateFontIndirect(&lFont);
  }
  SendDlgItemMessage(IDC_STATIC_TITLE, WM_SETFONT, reinterpret_cast<WPARAM>(m_hStaticFont), TRUE);
  
  // Create pages
  RECT rcWindow, rcClient;
  ::GetWindowRect(GetDlgItem(IDC_STATIC_TITLE), &rcWindow);
  ::GetClientRect(GetDlgItem(IDC_STATIC_TITLE), &rcClient);
  ::ScreenToClient(m_hWindow, reinterpret_cast<LPPOINT>(&rcWindow));
  for (size_t i = 0; i < PAGE_COUNT; i++) {
    m_Page[i].Create(IDD_SETTINGS_PAGE01 + i, m_hWindow, false);
    m_Page[i].SetPosition(NULL, rcWindow.left, (rcWindow.top * 2) + rcClient.bottom, 
      0, 0, SWP_HIDEWINDOW | SWP_NOSIZE);
  }
  
  // Add tree items
  m_Tree.Attach(GetDlgItem(IDC_TREE_PAGES));
  m_Tree.SetTheme();
  // Account
  HTREEITEM htAccount = m_Tree.InsertItem(L"Account", reinterpret_cast<LPARAM>(&m_Page[PAGE_ACCOUNT]), NULL);
  m_Page[PAGE_ACCOUNT].CreateItem(L"MyAnimeList", htAccount);
  m_Page[PAGE_UPDATE].CreateItem(L"Update", htAccount);
  m_Tree.Expand(htAccount);
  // Folders
  HTREEITEM htFolders = m_Tree.InsertItem(L"Anime folders", reinterpret_cast<LPARAM>(&m_Page[PAGE_FOLDERS_ROOT]), NULL);
  m_Page[PAGE_FOLDERS_ROOT].CreateItem(L"Root", htFolders);
  m_Page[PAGE_FOLDERS_ANIME].CreateItem(L"Specific", htFolders);
  m_Tree.Expand(htFolders);
  // Announcements
  HTREEITEM htAnnounce = m_Tree.InsertItem(L"Announcements", reinterpret_cast<LPARAM>(&m_Page[PAGE_HTTP]), NULL);
  m_Page[PAGE_HTTP].CreateItem(L"HTTP", htAnnounce);
  m_Page[PAGE_MESSENGER].CreateItem(L"Messenger", htAnnounce);
  m_Page[PAGE_MIRC].CreateItem(L"mIRC", htAnnounce);
  m_Page[PAGE_SKYPE].CreateItem(L"Skype", htAnnounce);
  m_Page[PAGE_TWITTER].CreateItem(L"Twitter", htAnnounce); // TODO: Re-enable after Twitter announcements are fixed.
  m_Tree.Expand(htAnnounce);
  // Program
  HTREEITEM htProgram = m_Tree.InsertItem(L"Program", reinterpret_cast<LPARAM>(&m_Page[PAGE_PROGRAM]), NULL);
  m_Page[PAGE_PROGRAM].CreateItem(L"General", htProgram);
  m_Page[PAGE_LIST].CreateItem(L"List", htProgram);
  m_Page[PAGE_NOTIFICATIONS].CreateItem(L"Notifications", htProgram);
  m_Tree.Expand(htProgram);
  // Media players
  HTREEITEM htRecognition = m_Tree.InsertItem(L"Recognition", reinterpret_cast<LPARAM>(&m_Page[PAGE_MEDIA]), NULL);
  m_Page[PAGE_MEDIA].CreateItem(L"Media players", htRecognition);
  m_Tree.Expand(htRecognition);
  // Torrent
  HTREEITEM htTorrent = m_Tree.InsertItem(L"Torrent", reinterpret_cast<LPARAM>(&m_Page[PAGE_TORRENT1]), NULL);
  m_Page[PAGE_TORRENT1].CreateItem(L"Options", htTorrent);
  m_Page[PAGE_TORRENT2].CreateItem(L"Filters", htTorrent);
  m_Tree.Expand(htTorrent);

  // Select current page
  m_Page[m_iCurrentPage].Select();
  return TRUE;
}

// =============================================================================

void CSettingsWindow::OnOK() {
  // Account
  wstring mal_user_old = Settings.Account.MAL.User;
  m_Page[PAGE_ACCOUNT].GetDlgItemText(IDC_EDIT_USER, Settings.Account.MAL.User);
  m_Page[PAGE_ACCOUNT].GetDlgItemText(IDC_EDIT_PASS, Settings.Account.MAL.Password);
  Settings.Account.MAL.API = m_Page[PAGE_ACCOUNT].GetCheckedRadioButton(IDC_RADIO_API1, IDC_RADIO_API2) + 1;
  Settings.Account.MAL.AutoLogin = m_Page[PAGE_ACCOUNT].IsDlgButtonChecked(IDC_CHECK_START_LOGIN);

  // Update
  Settings.Account.Update.Mode = m_Page[PAGE_UPDATE].GetCheckedRadioButton(IDC_RADIO_UPDATE_MODE1, IDC_RADIO_UPDATE_MODE3) + 1;
  Settings.Account.Update.Time = m_Page[PAGE_UPDATE].GetCheckedRadioButton(IDC_RADIO_UPDATE_TIME1, IDC_RADIO_UPDATE_TIME3) + 1;
  Settings.Account.Update.Delay = m_Page[PAGE_UPDATE].GetDlgItemInt(IDC_EDIT_DELAY);
  Settings.Account.Update.CheckPlayer = m_Page[PAGE_UPDATE].IsDlgButtonChecked(IDC_CHECK_UPDATE_CHECKMP);
  Settings.Account.Update.OutOfRange = m_Page[PAGE_UPDATE].IsDlgButtonChecked(IDC_CHECK_UPDATE_RANGE);

  // HTTP
  Settings.Announce.HTTP.Enabled = m_Page[PAGE_HTTP].IsDlgButtonChecked(IDC_CHECK_HTTP);
  m_Page[PAGE_HTTP].GetDlgItemText(IDC_EDIT_HTTP_URL, Settings.Announce.HTTP.URL);
  // Messenger
  Settings.Announce.MSN.Enabled = m_Page[PAGE_MESSENGER].IsDlgButtonChecked(IDC_CHECK_MESSENGER);
  // mIRC
  Settings.Announce.MIRC.Enabled = m_Page[PAGE_MIRC].IsDlgButtonChecked(IDC_CHECK_MIRC);
  m_Page[PAGE_MIRC].GetDlgItemText(IDC_EDIT_MIRC_SERVICE, Settings.Announce.MIRC.Service);
  Settings.Announce.MIRC.Mode = m_Page[PAGE_MIRC].GetCheckedRadioButton(IDC_RADIO_MIRC_CHANNEL1, IDC_RADIO_MIRC_CHANNEL3) + 1;
  Settings.Announce.MIRC.MultiServer = m_Page[PAGE_MIRC].IsDlgButtonChecked(IDC_CHECK_MIRC_MULTISERVER);
  Settings.Announce.MIRC.UseAction = m_Page[PAGE_MIRC].IsDlgButtonChecked(IDC_CHECK_MIRC_ACTION);
  m_Page[PAGE_MIRC].GetDlgItemText(IDC_EDIT_MIRC_CHANNELS, Settings.Announce.MIRC.Channels);
  // Skype
  Settings.Announce.Skype.Enabled = m_Page[PAGE_SKYPE].IsDlgButtonChecked(IDC_CHECK_SKYPE);
  // Twitter
  Settings.Announce.Twitter.Enabled = m_Page[PAGE_TWITTER].IsDlgButtonChecked(IDC_CHECK_TWITTER);
  m_Page[PAGE_TWITTER].GetDlgItemText(IDC_EDIT_TWITTER_USER, Settings.Announce.Twitter.User);
  m_Page[PAGE_TWITTER].GetDlgItemText(IDC_EDIT_TWITTER_PASS, Settings.Announce.Twitter.Password);

  // Folders > Root
  CListView List = m_Page[PAGE_FOLDERS_ROOT].GetDlgItem(IDC_LIST_FOLDERS_ROOT);
  Settings.Folders.Root.clear();
  for (int i = 0; i < List.GetItemCount(); i++) {
    wstring folder;
    List.GetItemText(i, 0, folder);
    Settings.Folders.Root.push_back(folder);
  }
  // Folders > Specific
  CAnime* pItem;
  List.SetWindowHandle(m_Page[PAGE_FOLDERS_ANIME].GetDlgItem(IDC_LIST_FOLDERS_ANIME));
  for (int i = 0; i < List.GetItemCount(); i++) {
    pItem = reinterpret_cast<CAnime*>(List.GetItemParam(i));
    if (pItem) {
      List.GetItemText(i, 1, pItem->Folder);
      Settings.Anime.SetItem(pItem->Series_ID, pItem->Folder, EMPTY_STR);
    }
  }
  List.SetWindowHandle(NULL);

  // Program > General
  Settings.Program.General.AutoStart = m_Page[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_AUTOSTART);
  Settings.Program.General.Close = m_Page[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_GENERAL_CLOSE);
  Settings.Program.General.Minimize = m_Page[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_GENERAL_MINIMIZE);
  wstring theme_old = Settings.Program.General.Theme;
  m_Page[PAGE_PROGRAM].GetDlgItemText(IDC_COMBO_THEME, Settings.Program.General.Theme);
  Settings.Program.StartUp.CheckNewVersion = m_Page[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_START_VERSION);
  Settings.Program.StartUp.CheckNewEpisodes = m_Page[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_START_CHECKEPS);
  Settings.Program.StartUp.Minimize = m_Page[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_START_MINIMIZE);
  Settings.Program.Exit.Ask = m_Page[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_EXIT_ASK);
  Settings.Program.Exit.SaveBuffer = m_Page[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_EXIT_SAVEBUFFER);
  m_Page[PAGE_PROGRAM].GetDlgItemText(IDC_EDIT_PROXY_HOST, Settings.Program.Proxy.Host);
  m_Page[PAGE_PROGRAM].GetDlgItemText(IDC_EDIT_PROXY_USER, Settings.Program.Proxy.User);
  m_Page[PAGE_PROGRAM].GetDlgItemText(IDC_EDIT_PROXY_PASS, Settings.Program.Proxy.Password);

  // Program > List
  Settings.Program.List.DoubleClick = m_Page[PAGE_LIST].GetComboSelection(IDC_COMBO_DBLCLICK);
  Settings.Program.List.MiddleClick = m_Page[PAGE_LIST].GetComboSelection(IDC_COMBO_MDLCLICK);
  AnimeList.Filter.NewEps = m_Page[PAGE_LIST].IsDlgButtonChecked(IDC_CHECK_FILTER_NEWEPS);
  Settings.Program.List.Highlight = m_Page[PAGE_LIST].IsDlgButtonChecked(IDC_CHECK_HIGHLIGHT);

  // Program > Notifications
  Settings.Program.Balloon.Enabled = m_Page[PAGE_NOTIFICATIONS].IsDlgButtonChecked(IDC_CHECK_BALLOON);

  // Recognition > Media players
  List.SetWindowHandle(m_Page[PAGE_MEDIA].GetDlgItem(IDC_LIST_MEDIA));
  for (size_t i = 0; i < MediaPlayers.Item.size(); i++) {
    MediaPlayers.Item[i].Enabled = List.GetCheckState(i);
  }
  List.SetWindowHandle(NULL);

  // Torrent > Options
  m_Page[PAGE_TORRENT1].GetDlgItemText(IDC_COMBO_TORRENT_SOURCE, Settings.RSS.Torrent.Source);
  Settings.RSS.Torrent.HideUnidentified = m_Page[PAGE_TORRENT1].IsDlgButtonChecked(IDC_CHECK_TORRENT_HIDE);
  Settings.RSS.Torrent.CheckEnabled = m_Page[PAGE_TORRENT1].IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOCHECK);
  Settings.RSS.Torrent.CheckInterval = m_Page[PAGE_TORRENT1].GetDlgItemInt(IDC_EDIT_TORRENT_INTERVAL);
  Settings.RSS.Torrent.AppMode = m_Page[PAGE_TORRENT1].GetCheckedRadioButton(IDC_RADIO_TORRENT_APP1, IDC_RADIO_TORRENT_APP2) + 1;
  Settings.RSS.Torrent.NewAction = m_Page[PAGE_TORRENT1].GetCheckedRadioButton(IDC_RADIO_TORRENT_NEW1, IDC_RADIO_TORRENT_NEW2) + 1;
  m_Page[PAGE_TORRENT1].GetDlgItemText(IDC_EDIT_TORRENT_APP, Settings.RSS.Torrent.AppPath);
  Settings.RSS.Torrent.SetFolder = m_Page[PAGE_TORRENT1].IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOSETFOLDER);
  // Torrent > Filters
  Settings.RSS.Torrent.Filters.GlobalEnabled = m_Page[PAGE_TORRENT2].IsDlgButtonChecked(IDC_CHECK_TORRENT_FILTERGLOBAL);
  Settings.RSS.Torrent.Filters.Global.clear();
  List.SetWindowHandle(m_Page[PAGE_TORRENT2].GetDlgItem(IDC_LIST_TORRENT_FILTERGLOBAL));
  for (int i = 0; i < List.GetItemCount(); i++) {
    AddTorrentFilterFromList(List.GetWindowHandle(), i, Settings.RSS.Torrent.Filters.Global);
  }
  List.SetWindowHandle(NULL);

  // Save settings
  MediaPlayers.Write();
  Settings.Write();

  // Change theme
  if (Settings.Program.General.Theme != theme_old) {
    UI.Read(Settings.Program.General.Theme);
    UI.LoadImages();
    MainWindow.m_Rebar.RedrawWindow();
    MainWindow.RefreshMenubar();
  }

  // Refresh other windows
  if (Settings.Account.MAL.User != mal_user_old) {
    AnimeList.Read();
    MainWindow.RefreshList(MAL_WATCHING);
    MainWindow.RefreshTabs(MAL_WATCHING);
    EventWindow.RefreshList();
    SearchWindow.PostMessage(WM_CLOSE);
    ExecuteAction(L"Logout(" + mal_user_old + L")");
  } else {
    MainWindow.RefreshList();
  }

  // Setup proxy
  HTTPClient.SetProxy(Settings.Program.Proxy.Host, Settings.Program.Proxy.User, Settings.Program.Proxy.Password);
  ImageClient.SetProxy(Settings.Program.Proxy.Host, Settings.Program.Proxy.User, Settings.Program.Proxy.Password);
  MainClient.SetProxy(Settings.Program.Proxy.Host, Settings.Program.Proxy.User, Settings.Program.Proxy.Password);
  SearchClient.SetProxy(Settings.Program.Proxy.Host, Settings.Program.Proxy.User, Settings.Program.Proxy.Password);
  TorrentClient.SetProxy(Settings.Program.Proxy.Host, Settings.Program.Proxy.User, Settings.Program.Proxy.Password);
  TwitterClient.SetProxy(Settings.Program.Proxy.Host, Settings.Program.Proxy.User, Settings.Program.Proxy.Password);
  VersionClient.SetProxy(Settings.Program.Proxy.Host, Settings.Program.Proxy.User, Settings.Program.Proxy.Password);
  
  // End dialog
  EndDialog(IDOK);
}

// =============================================================================

INT_PTR CSettingsWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Set title color
    case WM_CTLCOLORSTATIC: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      HWND hwnd_static = reinterpret_cast<HWND>(lParam);
      if (hwnd_static == GetDlgItem(IDC_STATIC_TITLE)) {
        ::SetBkMode(hdc, TRANSPARENT);
        ::SetTextColor(hdc, RGB(255, 255, 255));
        return reinterpret_cast<INT_PTR>(::GetStockObject(LTGRAY_BRUSH));
      }
      break;
    }

    // Drag window
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

    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL: {
      switch (m_iCurrentPage) {
        case PAGE_FOLDERS_ROOT:
          return m_Page[PAGE_FOLDERS_ROOT].SendDlgItemMessage(
            IDC_LIST_FOLDERS_ROOT, uMsg, wParam, lParam);
        case PAGE_FOLDERS_ANIME:
          return m_Page[PAGE_FOLDERS_ANIME].SendDlgItemMessage(
            IDC_LIST_FOLDERS_ANIME, uMsg, wParam, lParam);
        case PAGE_MEDIA:
          return m_Page[PAGE_MEDIA].SendDlgItemMessage(
            IDC_LIST_MEDIA, uMsg, wParam, lParam);
        case PAGE_TORRENT2:
          return m_Page[PAGE_TORRENT2].SendDlgItemMessage(
            IDC_LIST_TORRENT_FILTERGLOBAL, uMsg, wParam, lParam);
      }
      break;
    }
    
    // Select page
    case WM_NOTIFY: {
      LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
      if (pnmh->hwndFrom == GetDlgItem(IDC_TREE_PAGES)) {
        switch (pnmh->code) {
          case TVN_SELCHANGED: {
            LPNMTREEVIEW pnmtv = reinterpret_cast<LPNMTREEVIEW>(lParam);
            CSettingsPage* pPageNew = reinterpret_cast<CSettingsPage*>(pnmtv->itemNew.lParam);
            CSettingsPage* pPageOld = reinterpret_cast<CSettingsPage*>(pnmtv->itemOld.lParam);
            if (pPageNew != pPageOld) {
              SetDlgItemText(IDC_STATIC_TITLE, PAGE_TITLE[pPageNew->Index]);
              if (pPageOld) pPageOld->Show(SW_HIDE);
              m_iCurrentPage = pPageNew->Index;
              pPageNew->Show();
            }
          }
          break;
        }
      }
      break;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT CSettingsWindow::CSettingsTree::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {  
    // Forward mouse wheel messages to parent
    case WM_MOUSEWHEEL: {
      return ::SendMessage(GetParent(), uMsg, wParam, lParam);
    }
  }

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}