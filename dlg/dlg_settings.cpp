/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2013, Eren Okka
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
#include "../debug.h"
#include "../history.h"
#include "../http.h"
#include "../media.h"
#include "../monitor.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../stats.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"

#include "../win32/win_control.h"
#include "../win32/win_taskdialog.h"

const WCHAR* SECTION_TITLE[] = {
  L" Services",
  L" Library",
  L" Application",
  L" Recognition",
  L" Sharing",
  L" Torrents"
};

class SettingsDialog SettingsDialog;

// =============================================================================

SettingsDialog::SettingsDialog()
    : current_section_(SECTION_SERVICES),
      current_page_(PAGE_SERVICES_MAL) {
  RegisterDlgClass(L"TaigaSettingsW");

  pages.resize(PAGE_COUNT);
  for (size_t i = 0; i < PAGE_COUNT; i++) {
    pages[i].index = i;
    pages[i].parent = this;
  }
}

void SettingsDialog::SetCurrentSection(int index) {
  debug::Print(L"SettingsDialog::SetCurrentSection :: " + 
               ToWstr(current_section_) + L" -> " + ToWstr(index) + L"\n");

  current_section_ = index;
  
  if (!IsWindow())
    return;

  SetDlgItemText(IDC_STATIC_TITLE, SECTION_TITLE[index - 1]);

  tab_.DeleteAllItems();
  switch (index) {
    case SECTION_SERVICES:
      tab_.InsertItem(0, L"MyAnimeList", PAGE_SERVICES_MAL);
      break;
    case SECTION_LIBRARY:
      tab_.InsertItem(0, L"Folders", PAGE_LIBRARY_FOLDERS);
      tab_.InsertItem(1, L"Cache", PAGE_LIBRARY_CACHE);
      break;
    case SECTION_APPLICATION:
      tab_.InsertItem(0, L"Behavior", PAGE_APP_BEHAVIOR);
      tab_.InsertItem(1, L"Connection", PAGE_APP_CONNECTION);
      tab_.InsertItem(2, L"Interface", PAGE_APP_INTERFACE);
      tab_.InsertItem(3, L"Anime list", PAGE_APP_LIST);
      break;
    case SECTION_RECOGNITION:
      tab_.InsertItem(0, L"General", PAGE_RECOGNITION_GENERAL);
      tab_.InsertItem(1, L"Media players", PAGE_RECOGNITION_MEDIA);
      tab_.InsertItem(2, L"Media providers", PAGE_RECOGNITION_STREAM);
      break;
    case SECTION_SHARING:
      tab_.InsertItem(0, L"HTTP", PAGE_SHARING_HTTP);
      tab_.InsertItem(1, L"Messenger", PAGE_SHARING_MESSENGER);
      tab_.InsertItem(2, L"mIRC", PAGE_SHARING_MIRC);
      tab_.InsertItem(3, L"Skype", PAGE_SHARING_SKYPE);
      tab_.InsertItem(4, L"Twitter", PAGE_SHARING_TWITTER);
      break;
    case SECTION_TORRENTS:
      tab_.InsertItem(0, L"Discovery", PAGE_TORRENTS_DISCOVERY);
      tab_.InsertItem(1, L"Downloads", PAGE_TORRENTS_DOWNLOADS);
      tab_.InsertItem(2, L"Filters", PAGE_TORRENTS_FILTERS);
      break;
  }

  SetCurrentPage(tab_.GetItemParam(0));
}

void SettingsDialog::SetCurrentPage(int index) {
  debug::Print(L"SettingsDialog::SetCurrentPage :: " + 
               ToWstr(current_page_) + L" -> " + ToWstr(index) + L"\n");

  pages.at(current_page_).Hide();

  current_page_ = index;

  if (!IsWindow())
    return;

  if (!pages.at(index).IsWindow())
    pages.at(index).Create();
  pages.at(index).Show();

  for (int i = 0; i < tab_.GetItemCount(); i++) {
    if (tab_.GetItemParam(i) == index) {
      tab_.SetCurrentlySelected(i);
      break;
    }
  }
}

// =============================================================================

BOOL SettingsDialog::OnInitDialog() {
  // Initialize controls
  tree_.Attach(GetDlgItem(IDC_TREE_SECTIONS));
  tree_.SetImageList(UI.ImgList24.GetHandle());
  tree_.SetTheme();
  tab_.Attach(GetDlgItem(IDC_TAB_PAGES));
  
  // Add tree items
  #define TREE_INSERTITEM(s, t, i) \
    tree_.items[s] = tree_.InsertItem(t, i, s, nullptr);
  TREE_INSERTITEM(SECTION_SERVICES, L"Services", ICON24_GLOBE);
  TREE_INSERTITEM(SECTION_LIBRARY, L"Library", ICON24_LIBRARY);
  TREE_INSERTITEM(SECTION_APPLICATION, L"Application", ICON24_APPLICATION);
  TREE_INSERTITEM(SECTION_RECOGNITION, L"Recognition", ICON24_RECOGNITION);
  TREE_INSERTITEM(SECTION_SHARING, L"Sharing", ICON24_SHARING);
  TREE_INSERTITEM(SECTION_TORRENTS, L"Torrents", ICON24_FEED);
  #undef TREE_INSERTITEM

  // Set title font
  SendDlgItemMessage(IDC_STATIC_TITLE, WM_SETFONT, 
                     reinterpret_cast<WPARAM>(UI.font_bold.Get()), TRUE);

  // Select current section and page
  int current_page = current_page_;
  tree_.SelectItem(tree_.items[current_section_]);
  SetCurrentSection(current_section_);
  SetCurrentPage(current_page);
  
  return TRUE;
}

// =============================================================================

void SettingsDialog::OnOK() {
  win32::ListView list;
  SettingsPage* page = nullptr;

  wstring previous_user = Settings.Account.MAL.user;
  wstring previous_theme = Settings.Program.General.theme;

  // Services > MyAnimeList
  page = &pages[PAGE_SERVICES_MAL];
  if (page->IsWindow()) {
    page->GetDlgItemText(IDC_EDIT_USER, Settings.Account.MAL.user);
    page->GetDlgItemText(IDC_EDIT_PASS, Settings.Account.MAL.password);
    Settings.Account.MAL.auto_sync = page->IsDlgButtonChecked(IDC_CHECK_START_LOGIN);
  }

  // Library > Folders
  page = &pages[PAGE_LIBRARY_FOLDERS];
  if (page->IsWindow()) {
    list.SetWindowHandle(page->GetDlgItem(IDC_LIST_FOLDERS_ROOT));
    Settings.Folders.root.clear();
    for (int i = 0; i < list.GetItemCount(); i++) {
      wstring folder;
      list.GetItemText(i, 0, folder);
      Settings.Folders.root.push_back(folder);
    }
    Settings.Folders.watch_enabled = page->IsDlgButtonChecked(IDC_CHECK_FOLDERS_WATCH);
    list.SetWindowHandle(nullptr);
  }

  // Application > Behavior
  page = &pages[PAGE_APP_BEHAVIOR];
  if (page->IsWindow()) {
    Settings.Program.General.auto_start = page->IsDlgButtonChecked(IDC_CHECK_AUTOSTART);
    Settings.Program.General.close = page->IsDlgButtonChecked(IDC_CHECK_GENERAL_CLOSE);
    Settings.Program.General.minimize = page->IsDlgButtonChecked(IDC_CHECK_GENERAL_MINIMIZE);
    Settings.Program.StartUp.check_new_version = page->IsDlgButtonChecked(IDC_CHECK_START_VERSION);
    Settings.Program.StartUp.check_new_episodes = page->IsDlgButtonChecked(IDC_CHECK_START_CHECKEPS);
    Settings.Program.StartUp.minimize = page->IsDlgButtonChecked(IDC_CHECK_START_MINIMIZE);
  }
  // Application > Connection
  page = &pages[PAGE_APP_CONNECTION];
  if (page->IsWindow()) {
    page->GetDlgItemText(IDC_EDIT_PROXY_HOST, Settings.Program.Proxy.host);
    page->GetDlgItemText(IDC_EDIT_PROXY_USER, Settings.Program.Proxy.user);
    page->GetDlgItemText(IDC_EDIT_PROXY_PASS, Settings.Program.Proxy.password);
  }
  // Application > Interface
  page = &pages[PAGE_APP_INTERFACE];
  if (page->IsWindow()) {
    page->GetDlgItemText(IDC_COMBO_THEME, Settings.Program.General.theme);
    page->GetDlgItemText(IDC_EDIT_EXTERNALLINKS, Settings.Program.General.external_links);
  }
  // Application > List
  page = &pages[PAGE_APP_LIST];
  if (page->IsWindow()) {
    Settings.Program.List.double_click = page->GetComboSelection(IDC_COMBO_DBLCLICK);
    Settings.Program.List.middle_click = page->GetComboSelection(IDC_COMBO_MDLCLICK);
    Settings.Program.List.english_titles = page->IsDlgButtonChecked(IDC_CHECK_LIST_ENGLISH);
    Settings.Program.List.highlight = page->IsDlgButtonChecked(IDC_CHECK_HIGHLIGHT);
    Settings.Program.List.progress_show_aired = page->IsDlgButtonChecked(IDC_CHECK_LIST_PROGRESS_AIRED);
    Settings.Program.List.progress_show_available = page->IsDlgButtonChecked(IDC_CHECK_LIST_PROGRESS_AVAILABLE);
    Settings.Program.List.progress_show_eps = page->IsDlgButtonChecked(IDC_CHECK_LIST_PROGRESS_EPS);
  }
  
  // Recognition > General
  page = &pages[PAGE_RECOGNITION_GENERAL];
  if (page->IsWindow()) {
    Settings.Account.Update.ask_to_confirm = page->IsDlgButtonChecked(IDC_CHECK_UPDATE_CONFIRM);
    Settings.Account.Update.check_player = page->IsDlgButtonChecked(IDC_CHECK_UPDATE_CHECKMP);
    Settings.Account.Update.go_to_nowplaying = page->IsDlgButtonChecked(IDC_CHECK_UPDATE_GOTO);
    Settings.Account.Update.out_of_range = page->IsDlgButtonChecked(IDC_CHECK_UPDATE_RANGE);
    Settings.Account.Update.out_of_root = page->IsDlgButtonChecked(IDC_CHECK_UPDATE_ROOT);
    Settings.Account.Update.wait_mp = page->IsDlgButtonChecked(IDC_CHECK_UPDATE_WAITMP);
    Settings.Account.Update.delay = page->GetDlgItemInt(IDC_EDIT_DELAY);
    Settings.Program.Notifications.recognized = page->IsDlgButtonChecked(IDC_CHECK_NOTIFY_RECOGNIZED);
    Settings.Program.Notifications.notrecognized = page->IsDlgButtonChecked(IDC_CHECK_NOTIFY_NOTRECOGNIZED);
  }
  // Recognition > Media players
  page = &pages[PAGE_RECOGNITION_MEDIA];
  if (page->IsWindow()) {
    list.SetWindowHandle(page->GetDlgItem(IDC_LIST_MEDIA));
    for (size_t i = 0; i < MediaPlayers.items.size(); i++)
      MediaPlayers.items[i].enabled = list.GetCheckState(i);
    list.SetWindowHandle(nullptr);
  }
  // Recognition > Media providers
  page = &pages[PAGE_RECOGNITION_STREAM];
  if (page->IsWindow()) {
    list.SetWindowHandle(page->GetDlgItem(IDC_LIST_STREAM_PROVIDER));
    Settings.Recognition.Streaming.ann_enabled = list.GetCheckState(0) == TRUE;
    Settings.Recognition.Streaming.crunchyroll_enabled = list.GetCheckState(1) == TRUE;
    Settings.Recognition.Streaming.veoh_enabled = list.GetCheckState(2) == TRUE;
    Settings.Recognition.Streaming.viz_enabled = list.GetCheckState(3) == TRUE;
    Settings.Recognition.Streaming.youtube_enabled = list.GetCheckState(4) == TRUE;
    list.SetWindowHandle(nullptr);
  }

  // Sharing > HTTP
  page = &pages[PAGE_SHARING_HTTP];
  if (page->IsWindow()) {
    Settings.Announce.HTTP.enabled = page->IsDlgButtonChecked(IDC_CHECK_HTTP);
    page->GetDlgItemText(IDC_EDIT_HTTP_URL, Settings.Announce.HTTP.url);
  }
  // Sharing > Messenger
  page = &pages[PAGE_SHARING_MESSENGER];
  if (page->IsWindow()) {
    Settings.Announce.MSN.enabled = page->IsDlgButtonChecked(IDC_CHECK_MESSENGER);
  }
  // Sharing > mIRC
  page = &pages[PAGE_SHARING_MIRC];
  if (page->IsWindow()) {
    Settings.Announce.MIRC.enabled = page->IsDlgButtonChecked(IDC_CHECK_MIRC);
    page->GetDlgItemText(IDC_EDIT_MIRC_SERVICE, Settings.Announce.MIRC.service);
    Settings.Announce.MIRC.mode = page->GetCheckedRadioButton(IDC_RADIO_MIRC_CHANNEL1, IDC_RADIO_MIRC_CHANNEL3) + 1;
    Settings.Announce.MIRC.multi_server = page->IsDlgButtonChecked(IDC_CHECK_MIRC_MULTISERVER);
    Settings.Announce.MIRC.use_action = page->IsDlgButtonChecked(IDC_CHECK_MIRC_ACTION);
    page->GetDlgItemText(IDC_EDIT_MIRC_CHANNELS, Settings.Announce.MIRC.channels);
  }
  // Sharing > Skype
  page = &pages[PAGE_SHARING_SKYPE];
  if (page->IsWindow()) {
    Settings.Announce.Skype.enabled = page->IsDlgButtonChecked(IDC_CHECK_SKYPE);
  }
  // Sharing > Twitter
  page = &pages[PAGE_SHARING_TWITTER];
  if (page->IsWindow()) {
    Settings.Announce.Twitter.enabled = page->IsDlgButtonChecked(IDC_CHECK_TWITTER);
  }

  // Torrents > Discovery
  page = &pages[PAGE_TORRENTS_DISCOVERY];
  if (page->IsWindow()) {
    page->GetDlgItemText(IDC_COMBO_TORRENT_SOURCE, Settings.RSS.Torrent.source);
    page->GetDlgItemText(IDC_COMBO_TORRENT_SEARCH, Settings.RSS.Torrent.search_url);
    Settings.RSS.Torrent.check_enabled = page->IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOCHECK);
    Settings.RSS.Torrent.check_interval = page->GetDlgItemInt(IDC_EDIT_TORRENT_INTERVAL);
    Settings.RSS.Torrent.hide_unidentified = page->IsDlgButtonChecked(IDC_CHECK_TORRENT_HIDE);
    Settings.RSS.Torrent.new_action = page->GetCheckedRadioButton(IDC_RADIO_TORRENT_NEW1, IDC_RADIO_TORRENT_NEW2) + 1;
  }
  // Torrents > Downloads
  page = &pages[PAGE_TORRENTS_DOWNLOADS];
  if (page->IsWindow()) {
    Settings.RSS.Torrent.app_mode = page->GetCheckedRadioButton(IDC_RADIO_TORRENT_APP1, IDC_RADIO_TORRENT_APP2) + 1;
    page->GetDlgItemText(IDC_EDIT_TORRENT_APP, Settings.RSS.Torrent.app_path);
    Settings.RSS.Torrent.set_folder = page->IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOSETFOLDER);
    Settings.RSS.Torrent.create_folder = page->IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOCREATEFOLDER);
    page->GetDlgItemText(IDC_COMBO_TORRENT_FOLDER, Settings.RSS.Torrent.download_path);
  }
  // Torrents > Filters
  page = &pages[PAGE_TORRENTS_FILTERS];
  if (page->IsWindow()) {
    Settings.RSS.Torrent.Filters.global_enabled = page->IsDlgButtonChecked(IDC_CHECK_TORRENT_FILTER);
    list.SetWindowHandle(page->GetDlgItem(IDC_LIST_TORRENT_FILTER));
    for (int i = 0; i < list.GetItemCount(); i++) {
      FeedFilter* filter = reinterpret_cast<FeedFilter*>(list.GetItemParam(i));
      if (filter) filter->enabled = list.GetCheckState(i) == TRUE;
    }
    list.SetWindowHandle(nullptr);
    Aggregator.filter_manager.filters.clear();
    for (auto it = feed_filters_.begin(); it != feed_filters_.end(); ++it)
      Aggregator.filter_manager.filters.push_back(*it);
  }

  // Save settings
  MediaPlayers.Save();
  Settings.Save();

  // Apply changes
  Settings.ApplyChanges(previous_user, previous_theme);

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
        ::SetTextColor(hdc, ::GetSysColor(COLOR_WINDOW));
        return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_APPWORKSPACE));
      }
      break;
    }

    // Drag window
    case WM_ENTERSIZEMOVE:
      if (::IsAppThemed() && win32::GetWinVersion() >= win32::VERSION_VISTA)
        SetTransparency(200);
      break;
    case WM_EXITSIZEMOVE:
      if (::IsAppThemed() && win32::GetWinVersion() >= win32::VERSION_VISTA)
        SetTransparency(255);
      break;

    // Taiga, help! Only you can save us!
    case WM_HELP: {
      OnHelp(reinterpret_cast<LPHELPINFO>(lParam));
      return TRUE;
    }

    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL:
      switch (current_page_) {
        case PAGE_LIBRARY_FOLDERS:
          return pages[PAGE_LIBRARY_FOLDERS].SendDlgItemMessage(
            IDC_LIST_FOLDERS_ROOT, uMsg, wParam, lParam);
        case PAGE_RECOGNITION_MEDIA:
          return pages[PAGE_RECOGNITION_MEDIA].SendDlgItemMessage(
            IDC_LIST_MEDIA, uMsg, wParam, lParam);
        case PAGE_TORRENTS_FILTERS:
          return pages[PAGE_TORRENTS_FILTERS].SendDlgItemMessage(
            IDC_LIST_TORRENT_FILTER, uMsg, wParam, lParam);
      }
      break;
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void SettingsDialog::OnHelp(LPHELPINFO lphi) {
  wstring message, text;
  pages.at(current_page_).GetDlgItemText(lphi->iCtrlId, text);

  #define SET_HELP_TEXT(i, m) \
    case i: message = m; break;
  switch (lphi->iCtrlId) {
    // Library > Folders
    SET_HELP_TEXT(IDC_LIST_FOLDERS_ROOT,
      L"These folders will be scanned and monitored for new episodes.\n\n"
      L"Suppose that you have an HDD like this:\n\n"
      L"    D:\\\n"
      L"    \u2514 Anime\n"
      L"        \u2514 Bleach\n"
      L"        \u2514 Naruto\n"
      L"        \u2514 One Piece\n"
      L"    \u2514 Games\n"
      L"    \u2514 Music\n\n"
      L"In this case, \"D:\\Anime\" is the root folder you should add.");
    SET_HELP_TEXT(IDC_CHECK_FOLDERS_WATCH,
      L"With this feature on, Taiga instantly detects when a file is added, removed, or renamed under root folders and their subfolders.\n\n"
      L"Enabling this feature is recommended.");
    // Not available
    default:
      message = L"There's no help message associated with this item.";
      break;
  }
  #undef SET_HELP_TEXT

  MessageBox(message.c_str(), L"Help", MB_ICONINFORMATION | MB_OK);
}

LRESULT SettingsDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (idCtrl) {
    case IDC_TREE_SECTIONS: {
      switch (pnmh->code) {
        // Select section
        case TVN_SELCHANGED: {
          LPNMTREEVIEW pnmtv = reinterpret_cast<LPNMTREEVIEW>(pnmh);
          int section_new = pnmtv->itemNew.lParam;
          int section_old = pnmtv->itemOld.lParam;
          if (section_new != section_old)
            SetCurrentSection(section_new);
          break;
        }
      }
      break;
    }

    case IDC_TAB_PAGES: {
      switch (pnmh->code) {
        // Select tab
        case TCN_SELCHANGE: {
          int index = static_cast<int>(tab_.GetItemParam(tab_.GetCurrentlySelected()));
          SetCurrentPage(index);
          break;
        }
      }
      break;
    }

    case IDC_LINK_DEFAULTS: {
      switch (pnmh->code) {
        // Restore default settings
        case NM_CLICK: {
          win32::TaskDialog dlg;
          dlg.SetWindowTitle(APP_NAME);
          dlg.SetMainIcon(TD_ICON_WARNING);
          dlg.SetMainInstruction(L"Are you sure you want to restore default settings?");
          dlg.SetContent(L"All your current settings will be lost.");
          dlg.AddButton(L"Yes", IDYES);
          dlg.AddButton(L"No", IDNO);
          dlg.Show(GetWindowHandle());
          if (dlg.GetSelectedButtonID() == IDYES)
            Settings.RestoreDefaults();
          return TRUE;
        }
      }
      break;
    }
  }

  return 0;
}

LRESULT SettingsDialog::TreeView::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {  
    // Forward mouse wheel messages to parent
    case WM_MOUSEWHEEL:
      return ::SendMessage(GetParent(), uMsg, wParam, lParam);
  }

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

// =============================================================================

int SettingsDialog::AddTorrentFilterToList(HWND hwnd_list, const FeedFilter& filter) {
  wstring text;
  win32::ListView list = hwnd_list;
  int index = list.GetItemCount();
  int group = filter.anime_ids.empty() ? 0 : 1;
  
  int icon = ICON16_FUNNEL;
  switch (filter.action) {
    case FEED_FILTER_ACTION_DISCARD: icon = ICON16_FUNNEL_CROSS; break;
    case FEED_FILTER_ACTION_SELECT:  icon = ICON16_FUNNEL_TICK;  break;
    case FEED_FILTER_ACTION_PREFER:  icon = ICON16_FUNNEL_PLUS;  break;
  }

  // Insert item
  index = list.InsertItem(index, group, icon, 0, nullptr, filter.name.c_str(), 
                          reinterpret_cast<LPARAM>(&filter));
  list.SetCheckState(index, filter.enabled);
  list.SetWindowHandle(nullptr);

  return index;
}

void SettingsDialog::RefreshCache() {
  wstring text;
  Stats.CalculateLocalData();
  SettingsPage& page = pages[PAGE_LIBRARY_CACHE];

  // History
  text = ToWstr(static_cast<int>(History.items.size())) + L" item(s)";
  page.SetDlgItemText(IDC_STATIC_CACHE1, text.c_str());

  // Image files
  text = ToWstr(Stats.image_count) + L" item(s), " + ToSizeString(Stats.image_size);
  page.SetDlgItemText(IDC_STATIC_CACHE2, text.c_str());

  // Torrent files
  text = ToWstr(Stats.torrent_count) + L" item(s), " + ToSizeString(Stats.torrent_size);
  page.SetDlgItemText(IDC_STATIC_CACHE3, text.c_str());
}

void SettingsDialog::RefreshTorrentFilterList(HWND hwnd_list) {
  win32::ListView list = hwnd_list;
  list.DeleteAllItems();

  for (auto it = feed_filters_.begin(); it != feed_filters_.end(); ++it) {
    AddTorrentFilterToList(hwnd_list, *it);
  }
  
  list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
  list.SetWindowHandle(nullptr);
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

  pages[PAGE_SHARING_TWITTER].SetDlgItemText(IDC_LINK_TWITTER, text.c_str());
}