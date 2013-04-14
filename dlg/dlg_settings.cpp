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

#include "dlg_anime_list.h"
#include "dlg_history.h"
#include "dlg_input.h"
#include "dlg_main.h"
#include "dlg_search.h"
#include "dlg_settings.h"
#include "dlg_stats.h"

#include "../anime_db.h"
#include "../anime_filter.h"
#include "../announce.h"
#include "../common.h"
#include "../history.h"
#include "../http.h"
#include "../media.h"
#include "../monitor.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../stats.h"
#include "../string.h"
#include "../theme.h"

#include "../win32/win_control.h"

const WCHAR* PAGE_TITLE[PAGE_COUNT] = {
  L" MyAnimeList",
  L" Update",
  L" HTTP announcement",
  L" Messenger announcement",
  L" mIRC announcement",
  L" Skype announcement",
  L" Twitter announcement",
  L" Root folders",
  L" Anime folders",
  L" Program",
  L" List",
  L" Notifications",
  L" Applications",
  L" Streaming media",
  L" Torrent options",
  L" Torrent filters"
};

class SettingsDialog SettingsDialog;

// =============================================================================

SettingsDialog::SettingsDialog()
    : current_page_(PAGE_ACCOUNT) {
  RegisterDlgClass(L"TaigaSettingsW");

  pages.resize(PAGE_COUNT);
  for (size_t i = 0; i < PAGE_COUNT; i++) {
    pages[i].index = i;
    pages[i].parent = this;
  }
}

void SettingsDialog::SetCurrentPage(int index) {
  current_page_ = index;
  if (IsWindow()) pages[index].Select();
}

// =============================================================================

BOOL SettingsDialog::OnInitDialog() {
  // Set title font
  SendDlgItemMessage(IDC_STATIC_TITLE, WM_SETFONT, 
    reinterpret_cast<WPARAM>(UI.font_bold.Get()), TRUE);
  
  // Create pages
  RECT rcWindow, rcClient;
  ::GetWindowRect(GetDlgItem(IDC_STATIC_TITLE), &rcWindow);
  ::GetClientRect(GetDlgItem(IDC_STATIC_TITLE), &rcClient);
  ::ScreenToClient(m_hWindow, reinterpret_cast<LPPOINT>(&rcWindow));
  for (size_t i = 0; i < PAGE_COUNT; i++) {
    pages[i].Create(IDD_SETTINGS_PAGE01 + i, m_hWindow, false);
    pages[i].SetPosition(nullptr, rcWindow.left, (rcWindow.top * 2) + rcClient.bottom, 
      0, 0, SWP_HIDEWINDOW | SWP_NOSIZE);
  }
  
  // Add tree items
  tree_.Attach(GetDlgItem(IDC_TREE_PAGES));
  tree_.SetTheme();
  // Account
  HTREEITEM htAccount = tree_.InsertItem(L"Account", -1, reinterpret_cast<LPARAM>(&pages[PAGE_ACCOUNT]), nullptr);
  pages[PAGE_ACCOUNT].CreateItem(L"MyAnimeList", htAccount);
  pages[PAGE_UPDATE].CreateItem(L"Update", htAccount);
  tree_.Expand(htAccount);
  // Folders
  HTREEITEM htFolders = tree_.InsertItem(L"Anime library", -1, reinterpret_cast<LPARAM>(&pages[PAGE_FOLDERS_ROOT]), nullptr);
  pages[PAGE_FOLDERS_ROOT].CreateItem(L"Root folders", htFolders);
  pages[PAGE_FOLDERS_ANIME].CreateItem(L"Anime folders", htFolders);
  tree_.Expand(htFolders);
  // Announcements
  HTREEITEM htAnnounce = tree_.InsertItem(L"Announcements", -1, reinterpret_cast<LPARAM>(&pages[PAGE_HTTP]), nullptr);
  pages[PAGE_HTTP].CreateItem(L"HTTP", htAnnounce);
  pages[PAGE_MESSENGER].CreateItem(L"Messenger", htAnnounce);
  pages[PAGE_MIRC].CreateItem(L"mIRC", htAnnounce);
  pages[PAGE_SKYPE].CreateItem(L"Skype", htAnnounce);
  pages[PAGE_TWITTER].CreateItem(L"Twitter", htAnnounce);
  tree_.Expand(htAnnounce);
  // Program
  HTREEITEM htProgram = tree_.InsertItem(L"Program", -1, reinterpret_cast<LPARAM>(&pages[PAGE_PROGRAM]), nullptr);
  pages[PAGE_PROGRAM].CreateItem(L"General", htProgram);
  pages[PAGE_LIST].CreateItem(L"List", htProgram);
  pages[PAGE_NOTIFICATIONS].CreateItem(L"Notifications", htProgram);
  tree_.Expand(htProgram);
  // Recognition
  HTREEITEM htRecognition = tree_.InsertItem(L"Recognition", -1, reinterpret_cast<LPARAM>(&pages[PAGE_MEDIA]), nullptr);
  pages[PAGE_MEDIA].CreateItem(L"Applications", htRecognition);
  pages[PAGE_STREAM].CreateItem(L"Streaming", htRecognition);
  tree_.Expand(htRecognition);
  // Torrent
  HTREEITEM htTorrent = tree_.InsertItem(L"Torrent", -1, reinterpret_cast<LPARAM>(&pages[PAGE_TORRENT1]), nullptr);
  pages[PAGE_TORRENT1].CreateItem(L"Options", htTorrent);
  pages[PAGE_TORRENT2].CreateItem(L"Filters", htTorrent);
  tree_.Expand(htTorrent);

  // Select current page
  pages[current_page_].Select();
  return TRUE;
}

// =============================================================================

void SettingsDialog::OnOK() {
  // Account
  wstring mal_user_old = Settings.Account.MAL.user;
  pages[PAGE_ACCOUNT].GetDlgItemText(IDC_EDIT_USER, Settings.Account.MAL.user);
  pages[PAGE_ACCOUNT].GetDlgItemText(IDC_EDIT_PASS, Settings.Account.MAL.password);
  Settings.Account.MAL.api = pages[PAGE_ACCOUNT].GetCheckedRadioButton(IDC_RADIO_API1, IDC_RADIO_API2) + 1;
  Settings.Account.MAL.auto_login = pages[PAGE_ACCOUNT].IsDlgButtonChecked(IDC_CHECK_START_LOGIN);

  // Update
  Settings.Account.Update.mode = pages[PAGE_UPDATE].GetCheckedRadioButton(IDC_RADIO_UPDATE_MODE1, IDC_RADIO_UPDATE_MODE3) + 1;
  Settings.Account.Update.time = pages[PAGE_UPDATE].GetCheckedRadioButton(IDC_RADIO_UPDATE_TIME1, IDC_RADIO_UPDATE_TIME3) + 1;
  Settings.Account.Update.delay = pages[PAGE_UPDATE].GetDlgItemInt(IDC_EDIT_DELAY);
  Settings.Account.Update.check_player = pages[PAGE_UPDATE].IsDlgButtonChecked(IDC_CHECK_UPDATE_CHECKMP);
  Settings.Account.Update.out_of_range = pages[PAGE_UPDATE].IsDlgButtonChecked(IDC_CHECK_UPDATE_RANGE);

  // HTTP
  Settings.Announce.HTTP.enabled = pages[PAGE_HTTP].IsDlgButtonChecked(IDC_CHECK_HTTP);
  pages[PAGE_HTTP].GetDlgItemText(IDC_EDIT_HTTP_URL, Settings.Announce.HTTP.url);
  // Messenger
  Settings.Announce.MSN.enabled = pages[PAGE_MESSENGER].IsDlgButtonChecked(IDC_CHECK_MESSENGER);
  // mIRC
  Settings.Announce.MIRC.enabled = pages[PAGE_MIRC].IsDlgButtonChecked(IDC_CHECK_MIRC);
  pages[PAGE_MIRC].GetDlgItemText(IDC_EDIT_MIRC_SERVICE, Settings.Announce.MIRC.service);
  Settings.Announce.MIRC.mode = pages[PAGE_MIRC].GetCheckedRadioButton(IDC_RADIO_MIRC_CHANNEL1, IDC_RADIO_MIRC_CHANNEL3) + 1;
  Settings.Announce.MIRC.multi_server = pages[PAGE_MIRC].IsDlgButtonChecked(IDC_CHECK_MIRC_MULTISERVER);
  Settings.Announce.MIRC.use_action = pages[PAGE_MIRC].IsDlgButtonChecked(IDC_CHECK_MIRC_ACTION);
  pages[PAGE_MIRC].GetDlgItemText(IDC_EDIT_MIRC_CHANNELS, Settings.Announce.MIRC.channels);
  // Skype
  Settings.Announce.Skype.enabled = pages[PAGE_SKYPE].IsDlgButtonChecked(IDC_CHECK_SKYPE);
  // Twitter
  Settings.Announce.Twitter.enabled = pages[PAGE_TWITTER].IsDlgButtonChecked(IDC_CHECK_TWITTER);

  // Folders > Root
  win32::ListView List = pages[PAGE_FOLDERS_ROOT].GetDlgItem(IDC_LIST_FOLDERS_ROOT);
  Settings.Folders.root.clear();
  for (int i = 0; i < List.GetItemCount(); i++) {
    wstring folder;
    List.GetItemText(i, 0, folder);
    Settings.Folders.root.push_back(folder);
  }
  Settings.Folders.watch_enabled = pages[PAGE_FOLDERS_ROOT].IsDlgButtonChecked(IDC_CHECK_FOLDERS_WATCH);
  // Folders > Specific
  List.SetWindowHandle(pages[PAGE_FOLDERS_ANIME].GetDlgItem(IDC_LIST_FOLDERS_ANIME));
  for (int i = 0; i < List.GetItemCount(); i++) {
    auto anime_item = AnimeDatabase.FindItem(static_cast<int>(List.GetItemParam(i)));
    if (anime_item) {
      wstring folder;
      List.GetItemText(i, 1, folder);
      anime_item->SetFolder(folder, true);
    }
  }
  List.SetWindowHandle(nullptr);

  // Program > General
  Settings.Program.General.auto_start = pages[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_AUTOSTART);
  Settings.Program.General.close = pages[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_GENERAL_CLOSE);
  Settings.Program.General.minimize = pages[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_GENERAL_MINIMIZE);
  wstring theme_old = Settings.Program.General.theme;
  pages[PAGE_PROGRAM].GetDlgItemText(IDC_COMBO_THEME, Settings.Program.General.theme);
  Settings.Program.StartUp.check_new_version = pages[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_START_VERSION);
  Settings.Program.StartUp.check_new_episodes = pages[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_START_CHECKEPS);
  Settings.Program.StartUp.minimize = pages[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_START_MINIMIZE);
  Settings.Program.Exit.ask = pages[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_EXIT_ASK);
  Settings.Program.Exit.remember_pos_size = pages[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_EXIT_REMEMBER);
  Settings.Program.Exit.save_event_queue = pages[PAGE_PROGRAM].IsDlgButtonChecked(IDC_CHECK_EXIT_SAVEBUFFER);
  pages[PAGE_PROGRAM].GetDlgItemText(IDC_EDIT_PROXY_HOST, Settings.Program.Proxy.host);
  pages[PAGE_PROGRAM].GetDlgItemText(IDC_EDIT_PROXY_USER, Settings.Program.Proxy.user);
  pages[PAGE_PROGRAM].GetDlgItemText(IDC_EDIT_PROXY_PASS, Settings.Program.Proxy.password);

  // Program > List
  Settings.Program.List.double_click = pages[PAGE_LIST].GetComboSelection(IDC_COMBO_DBLCLICK);
  Settings.Program.List.middle_click = pages[PAGE_LIST].GetComboSelection(IDC_COMBO_MDLCLICK);
  AnimeFilters.new_episodes = pages[PAGE_LIST].IsDlgButtonChecked(IDC_CHECK_FILTER_NEWEPS) == TRUE;
  Settings.Program.List.highlight = pages[PAGE_LIST].IsDlgButtonChecked(IDC_CHECK_HIGHLIGHT);
  Settings.Program.List.progress_mode = pages[PAGE_LIST].GetCheckedRadioButton(IDC_RADIO_LIST_PROGRESS1, IDC_RADIO_LIST_PROGRESS2);
  Settings.Program.List.progress_show_eps = pages[PAGE_LIST].IsDlgButtonChecked(IDC_CHECK_LIST_PROGRESS_EPS);

  // Program > Notifications
  Settings.Program.Balloon.enabled = pages[PAGE_NOTIFICATIONS].IsDlgButtonChecked(IDC_CHECK_BALLOON);

  // Recognition > Media players
  List.SetWindowHandle(pages[PAGE_MEDIA].GetDlgItem(IDC_LIST_MEDIA));
  for (size_t i = 0; i < MediaPlayers.items.size(); i++) {
    MediaPlayers.items[i].enabled = List.GetCheckState(i);
  }
  List.SetWindowHandle(nullptr);
  // Recognition > Streaming
  List.SetWindowHandle(pages[PAGE_STREAM].GetDlgItem(IDC_LIST_STREAM_PROVIDER));
  Settings.Recognition.Streaming.ann_enabled = List.GetCheckState(0) == TRUE;
  Settings.Recognition.Streaming.crunchyroll_enabled = List.GetCheckState(1) == TRUE;
  Settings.Recognition.Streaming.veoh_enabled = List.GetCheckState(2) == TRUE;
  Settings.Recognition.Streaming.viz_enabled = List.GetCheckState(3) == TRUE;
  Settings.Recognition.Streaming.youtube_enabled = List.GetCheckState(4) == TRUE;
  List.SetWindowHandle(nullptr);

  // Torrent > Options
  pages[PAGE_TORRENT1].GetDlgItemText(IDC_COMBO_TORRENT_SOURCE, Settings.RSS.Torrent.source);
  Settings.RSS.Torrent.hide_unidentified = pages[PAGE_TORRENT1].IsDlgButtonChecked(IDC_CHECK_TORRENT_HIDE);
  Settings.RSS.Torrent.check_enabled = pages[PAGE_TORRENT1].IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOCHECK);
  Settings.RSS.Torrent.check_interval = pages[PAGE_TORRENT1].GetDlgItemInt(IDC_EDIT_TORRENT_INTERVAL);
  Settings.RSS.Torrent.app_mode = pages[PAGE_TORRENT1].GetCheckedRadioButton(IDC_RADIO_TORRENT_APP1, IDC_RADIO_TORRENT_APP2) + 1;
  Settings.RSS.Torrent.new_action = pages[PAGE_TORRENT1].GetCheckedRadioButton(IDC_RADIO_TORRENT_NEW1, IDC_RADIO_TORRENT_NEW2) + 1;
  pages[PAGE_TORRENT1].GetDlgItemText(IDC_EDIT_TORRENT_APP, Settings.RSS.Torrent.app_path);
  Settings.RSS.Torrent.set_folder = pages[PAGE_TORRENT1].IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOSETFOLDER);
  Settings.RSS.Torrent.create_folder = pages[PAGE_TORRENT1].IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOCREATEFOLDER);
  pages[PAGE_TORRENT1].GetDlgItemText(IDC_COMBO_TORRENT_FOLDER, Settings.RSS.Torrent.download_path);
  // Torrent > Filters
  Settings.RSS.Torrent.Filters.global_enabled = pages[PAGE_TORRENT2].IsDlgButtonChecked(IDC_CHECK_TORRENT_FILTER);
  List.SetWindowHandle(pages[PAGE_TORRENT2].GetDlgItem(IDC_LIST_TORRENT_FILTER));
  for (int i = 0; i < List.GetItemCount(); i++) {
    FeedFilter* filter = reinterpret_cast<FeedFilter*>(List.GetItemParam(i));
    if (filter) filter->enabled = List.GetCheckState(i) == TRUE;
  }
  List.SetWindowHandle(nullptr);
  Aggregator.filter_manager.filters.clear();
  for (auto it = feed_filters_.begin(); it != feed_filters_.end(); ++it) {
    Aggregator.filter_manager.filters.push_back(*it);
  }

  // Save settings
  MediaPlayers.Save();
  Settings.Save();

  // Change theme
  if (Settings.Program.General.theme != theme_old) {
    UI.Load(Settings.Program.General.theme);
    UI.LoadImages();
    MainDialog.rebar.RedrawWindow();
    UpdateAllMenus();
  }

  // Refresh other windows
  if (Settings.Account.MAL.user != mal_user_old) {
    AnimeDatabase.LoadList();
    History.Load();
    CurrentEpisode.Set(anime::ID_UNKNOWN);
    MainDialog.treeview.RefreshHistoryCounter();
    AnimeListDialog.RefreshList(mal::MYSTATUS_WATCHING);
    AnimeListDialog.RefreshTabs(mal::MYSTATUS_UNKNOWN, false); // We need this to refresh the numbers
    AnimeListDialog.RefreshTabs(mal::MYSTATUS_WATCHING);
    HistoryDialog.RefreshList();
    SearchDialog.RefreshList();
    Stats.CalculateAll();
    StatsDialog.Refresh();
    ExecuteAction(L"Logout(" + mal_user_old + L")");
  } else {
    AnimeListDialog.RefreshList();
  }

  // Enable/disable folder monitor
  FolderMonitor.Enable(Settings.Folders.watch_enabled == TRUE);

  // Setup proxy
  SetProxies(Settings.Program.Proxy.host, 
    Settings.Program.Proxy.user, 
    Settings.Program.Proxy.password);

  // End dialog
  EndDialog(IDOK);
}

// =============================================================================

INT_PTR SettingsDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL: {
      switch (current_page_) {
        case PAGE_FOLDERS_ROOT:
          return pages[PAGE_FOLDERS_ROOT].SendDlgItemMessage(
            IDC_LIST_FOLDERS_ROOT, uMsg, wParam, lParam);
        case PAGE_FOLDERS_ANIME:
          return pages[PAGE_FOLDERS_ANIME].SendDlgItemMessage(
            IDC_LIST_FOLDERS_ANIME, uMsg, wParam, lParam);
        case PAGE_MEDIA:
          return pages[PAGE_MEDIA].SendDlgItemMessage(
            IDC_LIST_MEDIA, uMsg, wParam, lParam);
        case PAGE_TORRENT2:
          return pages[PAGE_TORRENT2].SendDlgItemMessage(
            IDC_LIST_TORRENT_FILTER, uMsg, wParam, lParam);
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
            SettingsPage* page_new = reinterpret_cast<SettingsPage*>(pnmtv->itemNew.lParam);
            SettingsPage* page_old = reinterpret_cast<SettingsPage*>(pnmtv->itemOld.lParam);
            if (page_new != page_old) {
              SetDlgItemText(IDC_STATIC_TITLE, PAGE_TITLE[page_new->index]);
              if (page_old) page_old->Hide();
              current_page_ = page_new->index;
              page_new->Show();
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

LRESULT SettingsDialog::TreeView::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {  
    // Forward mouse wheel messages to parent
    case WM_MOUSEWHEEL: {
      return ::SendMessage(GetParent(), uMsg, wParam, lParam);
    }
  }

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

// =============================================================================

int SettingsDialog::AddTorrentFilterToList(HWND hwnd_list, const FeedFilter& filter) {
  wstring text;
  win32::ListView List = hwnd_list;
  int index = List.GetItemCount();
  int group = filter.anime_ids.empty() ? 0 : 1;
  int icon = ICON16_FUNNEL;
  switch (filter.action) {
    case FEED_FILTER_ACTION_DISCARD: icon = ICON16_FUNNEL_CROSS; break;
    case FEED_FILTER_ACTION_SELECT:  icon = ICON16_FUNNEL_TICK;  break;
    case FEED_FILTER_ACTION_PREFER:  icon = ICON16_FUNNEL_PLUS;  break;
  }

  // Insert item
  index = List.InsertItem(index, group, icon, 0, nullptr, filter.name.c_str(), 
    reinterpret_cast<LPARAM>(&filter));
  List.SetCheckState(index, filter.enabled);
  
  List.SetWindowHandle(nullptr);
  return index;
}

void SettingsDialog::RefreshTorrentFilterList(HWND hwnd_list) {
  win32::ListView List = hwnd_list;
  List.DeleteAllItems();

  for (auto it = feed_filters_.begin(); it != feed_filters_.end(); ++it) {
    AddTorrentFilterToList(hwnd_list, *it);
  }

  List.SetWindowHandle(nullptr);
}

void SettingsDialog::RefreshTwitterLink() {
  wstring text;
  if (Settings.Announce.Twitter.user.empty()) {
    text = L"Taiga is not authorized to post to your Twitter account yet.";
  } else {
    text = L"Taiga is authorized to post to this Twitter account: ";
    text += L"<a href=\"URL(http://twitter.com/" + Settings.Announce.Twitter.user;
    text += L")\">" + Settings.Announce.Twitter.user + L"</a>";
  }
  pages[PAGE_TWITTER].SetDlgItemText(IDC_LINK_TWITTER, text.c_str());
}