/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include <windows/win/task_dialog.h>

#include "base/base64.h"
#include "base/file.h"
#include "base/format.h"
#include "base/string.h"
#include "media/anime_util.h"
#include "media/library/history.h"
#include "taiga/config.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "taiga/stats.h"
#include "track/feed_aggregator.h"
#include "track/feed_filter_manager.h"
#include "track/media.h"
#include "track/media_stream.h"
#include "track/monitor.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/theme.h"
#include "ui/translate.h"

namespace ui {

const WCHAR* kSectionTitles[] = {
  L" Services",
  L" Library",
  L" Application",
  L" Recognition",
  L" Sharing",
  L" Torrents",
  L" Advanced",
};

SettingsDialog DlgSettings;

SettingsDialog::SettingsDialog()
    : current_section_(kSettingsSectionServices),
      current_page_(kSettingsPageServicesMain) {
  RegisterDlgClass(L"TaigaSettingsW");

  pages.resize(kSettingsPageCount);
  for (size_t i = 0; i < kSettingsPageCount; i++) {
    pages[i].index = i;
    pages[i].parent = this;
  }
}

void SettingsDialog::SetCurrentSection(SettingsSections section) {
  current_section_ = section;

  if (!IsWindow())
    return;

  SetDlgItemText(IDC_STATIC_TITLE, kSectionTitles[current_section_ - 1]);

  tab_.DeleteAllItems();
  switch (current_section_) {
    case kSettingsSectionServices:
      tab_.InsertItem(0, L"Main", kSettingsPageServicesMain);
      tab_.InsertItem(1, L"MyAnimeList", kSettingsPageServicesMal);
      tab_.InsertItem(2, L"Kitsu", kSettingsPageServicesKitsu);
      tab_.InsertItem(3, L"AniList", kSettingsPageServicesAniList);
      break;
    case kSettingsSectionLibrary:
      tab_.InsertItem(0, L"Folders", kSettingsPageLibraryFolders);
      break;
    case kSettingsSectionApplication:
      tab_.InsertItem(0, L"Anime list", kSettingsPageAppList);
      tab_.InsertItem(1, L"General", kSettingsPageAppGeneral);
      break;
    case kSettingsSectionRecognition:
      tab_.InsertItem(0, L"General", kSettingsPageRecognitionGeneral);
      tab_.InsertItem(1, L"Media players", kSettingsPageRecognitionMedia);
      tab_.InsertItem(2, L"Streaming media", kSettingsPageRecognitionStream);
      break;
    case kSettingsSectionSharing:
      tab_.InsertItem(0, L"Discord", kSettingsPageSharingDiscord);
      tab_.InsertItem(1, L"HTTP", kSettingsPageSharingHttp);
      tab_.InsertItem(2, L"mIRC", kSettingsPageSharingMirc);
      break;
    case kSettingsSectionTorrents:
      tab_.InsertItem(0, L"Discovery", kSettingsPageTorrentsDiscovery);
      tab_.InsertItem(1, L"Downloads", kSettingsPageTorrentsDownloads);
      tab_.InsertItem(2, L"Filters", kSettingsPageTorrentsFilters);
      break;
    case kSettingsSectionAdvanced:
      tab_.InsertItem(0, L"Settings", kSettingsPageAdvancedSettings);
      tab_.InsertItem(1, L"Cache", kSettingsPageAdvancedCache);
      break;
  }
}

void SettingsDialog::SetCurrentPage(SettingsPages page) {
  const auto previous_page = current_page_;
  current_page_ = page;

  pages.at(previous_page).Hide();

  if (!IsWindow())
    return;

  if (!pages.at(current_page_).IsWindow())
    pages.at(current_page_).Create();
  pages.at(current_page_).Show();

  if (previous_page != current_page_) {
    const HWND hwnd = GetFocus();
    if (::IsWindow(hwnd) && !::IsWindowVisible(hwnd))
      pages.at(current_page_).SetFocus();
  }

  for (int i = 0; i < tab_.GetItemCount(); i++) {
    if (tab_.GetItemParam(i) == current_page_) {
      tab_.SetCurrentlySelected(i);
      break;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

BOOL SettingsDialog::OnInitDialog() {
  // Initialize controls
  tree_.Attach(GetDlgItem(IDC_TREE_SECTIONS));
  tree_.SetImageList(ui::Theme.GetImageList24().GetHandle());
  tree_.SetTheme();
  tab_.Attach(GetDlgItem(IDC_TAB_PAGES));

  // Add tree items
  #define TREE_INSERTITEM(s, t, i) \
    tree_.items[s] = tree_.InsertItem(t, i, s, nullptr);
  TREE_INSERTITEM(kSettingsSectionServices, L"Services", ui::kIcon24_Globe);
  TREE_INSERTITEM(kSettingsSectionLibrary, L"Library", ui::kIcon24_Library);
  TREE_INSERTITEM(kSettingsSectionApplication, L"Application", ui::kIcon24_Application);
  TREE_INSERTITEM(kSettingsSectionRecognition, L"Recognition", ui::kIcon24_Recognition);
  TREE_INSERTITEM(kSettingsSectionSharing, L"Sharing", ui::kIcon24_Sharing);
  TREE_INSERTITEM(kSettingsSectionTorrents, L"Torrents", ui::kIcon24_Feed);
  TREE_INSERTITEM(kSettingsSectionAdvanced, L"Advanced", ui::kIcon24_Settings);
  #undef TREE_INSERTITEM

  // Set title font
  SendDlgItemMessage(IDC_STATIC_TITLE, WM_SETFONT,
                     reinterpret_cast<WPARAM>(ui::Theme.GetBoldFont()), TRUE);

  // Select current section and page
  SettingsPages current_page = current_page_;
  tree_.SelectItem(tree_.items[current_section_]);
  SetCurrentSection(current_section_);
  SetCurrentPage(current_page);

  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

void SettingsDialog::OnOK() {
  win::ListView list;
  SettingsPage* page = nullptr;

  // Services > Main
  page = &pages[kSettingsPageServicesMain];
  if (page->IsWindow()) {
    win::ComboBox combo(page->GetDlgItem(IDC_COMBO_SERVICE));
    auto service_id = static_cast<sync::ServiceId>(combo.GetItemData(combo.GetCurSel()));
    taiga::settings.SetSyncActiveService(service_id);
    taiga::settings.SetSyncAutoOnStart(page->IsDlgButtonChecked(IDC_CHECK_START_LOGIN));
    combo.SetWindowHandle(nullptr);
  }
  // Services > MyAnimeList
  page = &pages[kSettingsPageServicesMal];
  if (page->IsWindow()) {
    taiga::settings.SetSyncServiceMalUsername(page->GetDlgItemText(IDC_EDIT_USER_MAL));
  }
  // Services > Kitsu
  page = &pages[kSettingsPageServicesKitsu];
  if (page->IsWindow()) {
    taiga::settings.SetSyncServiceKitsuEmail(page->GetDlgItemText(IDC_EDIT_USER_KITSU));
    taiga::settings.SetSyncServiceKitsuPassword(Base64Encode(page->GetDlgItemText(IDC_EDIT_PASS_KITSU)));
  }
  // Services > AniList
  page = &pages[kSettingsPageServicesAniList];
  if (page->IsWindow()) {
    taiga::settings.SetSyncServiceAniListUsername(page->GetDlgItemText(IDC_EDIT_USER_ANILIST));
  }

  // Library > Folders
  page = &pages[kSettingsPageLibraryFolders];
  if (page->IsWindow()) {
    list.SetWindowHandle(page->GetDlgItem(IDC_LIST_FOLDERS_ROOT));
    std::vector<std::wstring> library_folders;
    for (int i = 0; i < list.GetItemCount(); i++) {
      std::wstring folder;
      list.GetItemText(i, 0, folder);
      library_folders.push_back(folder);
    }
    taiga::settings.SetLibraryFolders(library_folders);
    taiga::settings.SetLibraryWatchFolders(page->IsDlgButtonChecked(IDC_CHECK_FOLDERS_WATCH));
    list.SetWindowHandle(nullptr);
  }

  // Application > General
  page = &pages[kSettingsPageAppGeneral];
  if (page->IsWindow()) {
    taiga::settings.SetAppBehaviorAutostart(page->IsDlgButtonChecked(IDC_CHECK_AUTOSTART));
    taiga::settings.SetAppBehaviorCloseToTray(page->IsDlgButtonChecked(IDC_CHECK_GENERAL_CLOSE));
    taiga::settings.SetAppBehaviorMinimizeToTray(page->IsDlgButtonChecked(IDC_CHECK_GENERAL_MINIMIZE));
    taiga::settings.SetAppBehaviorCheckForUpdates(page->IsDlgButtonChecked(IDC_CHECK_START_VERSION));
    taiga::settings.SetAppBehaviorScanAvailableEpisodes(page->IsDlgButtonChecked(IDC_CHECK_START_CHECKEPS));
    taiga::settings.SetAppBehaviorStartMinimized(page->IsDlgButtonChecked(IDC_CHECK_START_MINIMIZE));
    taiga::settings.SetAppInterfaceExternalLinks(page->GetDlgItemText(IDC_EDIT_EXTERNALLINKS));
  }
  // Application > List
  page = &pages[kSettingsPageAppList];
  if (page->IsWindow()) {
    taiga::settings.SetAppListDoubleClickAction(page->GetComboSelection(IDC_COMBO_DBLCLICK));
    taiga::settings.SetAppListMiddleClickAction(page->GetComboSelection(IDC_COMBO_MDLCLICK));
    taiga::settings.SetAppListTitleLanguagePreference(static_cast<anime::TitleLanguage>(
        page->GetComboSelection(IDC_COMBO_TITLELANG)));
    taiga::settings.SetAppListHighlightNewEpisodes(page->IsDlgButtonChecked(IDC_CHECK_HIGHLIGHT));
    taiga::settings.SetAppListDisplayHighlightedOnTop(page->IsDlgButtonChecked(IDC_CHECK_HIGHLIGHT_ONTOP));
    taiga::settings.SetAppListProgressDisplayAired(page->IsDlgButtonChecked(IDC_CHECK_LIST_PROGRESS_AIRED));
    taiga::settings.SetAppListProgressDisplayAvailable(page->IsDlgButtonChecked(IDC_CHECK_LIST_PROGRESS_AVAILABLE));
  }

  // Recognition > General
  page = &pages[kSettingsPageRecognitionGeneral];
  if (page->IsWindow()) {
    taiga::settings.SetSyncUpdateAskToConfirm(page->IsDlgButtonChecked(IDC_CHECK_UPDATE_CONFIRM));
    taiga::settings.SetSyncUpdateCheckPlayer(page->IsDlgButtonChecked(IDC_CHECK_UPDATE_CHECKMP));
    taiga::settings.SetSyncUpdateOutOfRange(page->IsDlgButtonChecked(IDC_CHECK_UPDATE_RANGE));
    taiga::settings.SetSyncUpdateOutOfRoot(page->IsDlgButtonChecked(IDC_CHECK_UPDATE_ROOT));
    taiga::settings.SetSyncUpdateWaitPlayer(page->IsDlgButtonChecked(IDC_CHECK_UPDATE_WAITMP));
    taiga::settings.SetSyncUpdateDelay(static_cast<int>(page->GetDlgItemInt(IDC_EDIT_DELAY)));
    taiga::settings.SetSyncGoToNowPlayingRecognized(page->IsDlgButtonChecked(IDC_CHECK_GOTO_RECOGNIZED));
    taiga::settings.SetSyncGoToNowPlayingNotRecognized(page->IsDlgButtonChecked(IDC_CHECK_GOTO_NOTRECOGNIZED));
    taiga::settings.SetSyncNotifyRecognized(page->IsDlgButtonChecked(IDC_CHECK_NOTIFY_RECOGNIZED));
    taiga::settings.SetSyncNotifyNotRecognized(page->IsDlgButtonChecked(IDC_CHECK_NOTIFY_NOTRECOGNIZED));
  }
  // Recognition > Media players
  page = &pages[kSettingsPageRecognitionMedia];
  if (page->IsWindow()) {
    taiga::settings.SetRecognitionDetectMediaPlayers(page->IsDlgButtonChecked(IDC_CHECK_DETECT_MEDIA_PLAYER) == TRUE);
    list.SetWindowHandle(page->GetDlgItem(IDC_LIST_MEDIA));
    for (int i = 0; i < list.GetItemCount(); ++i) {
      const auto& player = track::media_players.items[list.GetItemParam(i)];
      taiga::settings.SetMediaPlayerEnabled(StrToWstr(player.name), list.GetCheckState(i));
    }
    list.SetWindowHandle(nullptr);
  }
  // Recognition > Media providers
  page = &pages[kSettingsPageRecognitionStream];
  if (page->IsWindow()) {
    taiga::settings.SetRecognitionDetectStreamingMedia(page->IsDlgButtonChecked(IDC_CHECK_DETECT_STREAMING_MEDIA) == TRUE);
    list.SetWindowHandle(page->GetDlgItem(IDC_LIST_STREAM_PROVIDER));
    for (int i = 0; i < list.GetItemCount(); ++i) {
      track::recognition::EnableStream(static_cast<track::recognition::Stream>(list.GetItemParam(i)), list.GetCheckState(i) == TRUE);
    }
    list.SetWindowHandle(nullptr);
  }

  // Sharing > Discord
  page = &pages[kSettingsPageSharingDiscord];
  if (page->IsWindow()) {
    taiga::settings.SetShareDiscordEnabled(page->IsDlgButtonChecked(IDC_CHECK_DISCORD));
    taiga::settings.SetShareDiscordUsernameEnabled(page->IsDlgButtonChecked(IDC_CHECK_DISCORD_USERNAME));
    taiga::settings.SetShareDiscordGroupEnabled(page->IsDlgButtonChecked(IDC_CHECK_DISCORD_GROUP));
    taiga::settings.SetShareDiscordTimeEnabled(page->IsDlgButtonChecked(IDC_CHECK_DISCORD_TIME));
    taiga::settings.SetShareDiscordWatchingEnabled(page->IsDlgButtonChecked(IDC_CHECK_DISCORD_WATCHING));
  }
  // Sharing > HTTP
  page = &pages[kSettingsPageSharingHttp];
  if (page->IsWindow()) {
    taiga::settings.SetShareHttpEnabled(page->IsDlgButtonChecked(IDC_CHECK_HTTP));
    taiga::settings.SetShareHttpUrl(page->GetDlgItemText(IDC_EDIT_HTTP_URL));
  }
  // Sharing > mIRC
  page = &pages[kSettingsPageSharingMirc];
  if (page->IsWindow()) {
    taiga::settings.SetShareMircEnabled(page->IsDlgButtonChecked(IDC_CHECK_MIRC));
    taiga::settings.SetShareMircService(page->GetDlgItemText(IDC_EDIT_MIRC_SERVICE));
    taiga::settings.SetShareMircMode(page->GetCheckedRadioButton(IDC_RADIO_MIRC_CHANNEL1, IDC_RADIO_MIRC_CHANNEL3) + 1);
    taiga::settings.SetShareMircMultiServer(page->IsDlgButtonChecked(IDC_CHECK_MIRC_MULTISERVER));
    taiga::settings.SetShareMircUseMeAction(page->IsDlgButtonChecked(IDC_CHECK_MIRC_ACTION));
    taiga::settings.SetShareMircChannels(page->GetDlgItemText(IDC_EDIT_MIRC_CHANNELS));
  }

  // Torrents > Discovery
  page = &pages[kSettingsPageTorrentsDiscovery];
  if (page->IsWindow()) {
    taiga::settings.SetTorrentDiscoverySource(page->GetDlgItemText(IDC_COMBO_TORRENT_SOURCE));
    taiga::settings.SetTorrentDiscoverySearchUrl(page->GetDlgItemText(IDC_COMBO_TORRENT_SEARCH));
    taiga::settings.SetTorrentDiscoveryAutoCheckEnabled(page->IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOCHECK));
    taiga::settings.SetTorrentDiscoveryAutoCheckInterval(static_cast<int>(page->GetDlgItemInt(IDC_EDIT_TORRENT_INTERVAL)));
    taiga::settings.SetTorrentDiscoveryNewAction(static_cast<track::TorrentAction>(
        page->GetCheckedRadioButton(IDC_RADIO_TORRENT_NEW1, IDC_RADIO_TORRENT_NEW2) + 1));
  }
  // Torrents > Downloads
  page = &pages[kSettingsPageTorrentsDownloads];
  if (page->IsWindow()) {
    taiga::settings.SetTorrentDownloadSortBy(
        page->GetComboSelection(IDC_COMBO_TORRENTS_QUEUE_SORTBY) == 0 ?
        std::wstring(L"episode_number") : std::wstring(L"release_date"));
    taiga::settings.SetTorrentDownloadSortOrder(
        page->GetComboSelection(IDC_COMBO_TORRENTS_QUEUE_SORTORDER) == 0 ?
        std::wstring(L"ascending") : std::wstring(L"descending"));
    taiga::settings.SetTorrentDownloadUseAnimeFolder(page->IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOSETFOLDER));
    taiga::settings.SetTorrentDownloadFallbackOnFolder(page->IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOUSEFOLDER));
    taiga::settings.SetTorrentDownloadLocation(page->GetDlgItemText(IDC_COMBO_TORRENT_FOLDER));
    taiga::settings.SetTorrentDownloadCreateSubfolder(page->IsDlgButtonChecked(IDC_CHECK_TORRENT_AUTOCREATEFOLDER));
    taiga::settings.SetTorrentDownloadAppOpen(page->IsDlgButtonChecked(IDC_CHECK_TORRENT_APP_OPEN));
    taiga::settings.SetTorrentDownloadAppMode(static_cast<track::TorrentApp>(
        page->GetCheckedRadioButton(IDC_RADIO_TORRENT_APP1, IDC_RADIO_TORRENT_APP2) + 1));
    taiga::settings.SetTorrentDownloadAppPath(page->GetDlgItemText(IDC_EDIT_TORRENT_APP));
  }
  // Torrents > Filters
  page = &pages[kSettingsPageTorrentsFilters];
  if (page->IsWindow()) {
    taiga::settings.SetTorrentFilterEnabled(page->IsDlgButtonChecked(IDC_CHECK_TORRENT_FILTER));
    list.SetWindowHandle(page->GetDlgItem(IDC_LIST_TORRENT_FILTER));
    for (int i = 0; i < list.GetItemCount(); i++) {
      const auto filter = reinterpret_cast<track::FeedFilter*>(list.GetItemParam(i));
      if (filter) filter->enabled = list.GetCheckState(i) == TRUE;
    }
    list.SetWindowHandle(nullptr);
    track::feed_filter_manager.SetFilters(feed_filters_);
  }

  // Advanced
  page = &pages[kSettingsPageAdvancedSettings];
  if (page->IsWindow()) {
    for (const auto& [key, value] : advanced_settings_) {
      SetAdvancedSetting(key, value);
    }
  }

  // Save settings
  taiga::settings.Save();
  taiga::settings.ApplyChanges();

  // End dialog
  EndDialog(IDOK);
}

////////////////////////////////////////////////////////////////////////////////

INT_PTR SettingsDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Set title color
    case WM_CTLCOLORSTATIC: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      HWND hwnd_static = reinterpret_cast<HWND>(lParam);
      if (hwnd_static == GetDlgItem(IDC_STATIC_TITLE)) {
        ::SetBkMode(hdc, TRANSPARENT);
        ::SetTextColor(hdc, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
        return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_APPWORKSPACE));
      }
      break;
    }

    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL:
      switch (current_page_) {
        case kSettingsPageLibraryFolders:
          return pages[kSettingsPageLibraryFolders].SendDlgItemMessage(
              IDC_LIST_FOLDERS_ROOT, uMsg, wParam, lParam);
        case kSettingsPageRecognitionMedia:
          return pages[kSettingsPageRecognitionMedia].SendDlgItemMessage(
              IDC_LIST_MEDIA, uMsg, wParam, lParam);
        case kSettingsPageTorrentsFilters:
          return pages[kSettingsPageTorrentsFilters].SendDlgItemMessage(
              IDC_LIST_TORRENT_FILTER, uMsg, wParam, lParam);
      }
      break;
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT SettingsDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (idCtrl) {
    case IDC_TREE_SECTIONS: {
      switch (pnmh->code) {
        // Select section
        case TVN_SELCHANGED: {
          LPNMTREEVIEW pnmtv = reinterpret_cast<LPNMTREEVIEW>(pnmh);
          auto section_new = static_cast<SettingsSections>(pnmtv->itemNew.lParam);
          auto section_old = static_cast<SettingsSections>(pnmtv->itemOld.lParam);
          if (section_new != section_old) {
            SetCurrentSection(section_new);
            SetCurrentPage(static_cast<SettingsPages>(tab_.GetItemParam(0)));
          }
          break;
        }
      }
      break;
    }

    case IDC_TAB_PAGES: {
      switch (pnmh->code) {
        // Select tab
        case TCN_SELCHANGE: {
          auto page = static_cast<SettingsPages>(tab_.GetItemParam(tab_.GetCurrentlySelected()));
          SetCurrentPage(page);
          break;
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

////////////////////////////////////////////////////////////////////////////////

int SettingsDialog::AddTorrentFilterToList(HWND hwnd_list, const track::FeedFilter& filter) {
  win::ListView list = hwnd_list;

  const int index = list.GetItemCount();

  const int icon = [&filter]() {
    switch (filter.action) {
      default: return ui::kIcon16_Funnel;
      case track::kFeedFilterActionDiscard: return ui::kIcon16_FunnelCross;
      case track::kFeedFilterActionSelect: return ui::kIcon16_FunnelTick;
      case track::kFeedFilterActionPrefer: return ui::kIcon16_FunnelPlus;
    }
  }();

  const std::wstring limits = filter.anime_ids.empty()
      ? L"All"
      : L"{} anime"_format(filter.anime_ids.size());

  list.InsertItem(index, -1, icon, 0, nullptr, filter.name.c_str(),
                  reinterpret_cast<LPARAM>(&filter));
  list.SetItem(index, 1, limits.c_str());
  list.SetCheckState(index, filter.enabled);
  list.SetWindowHandle(nullptr);

  return index;
}

void SettingsDialog::RefreshCache() {
  std::wstring text;
  taiga::stats.CalculateLocalData();
  SettingsPage& page = pages[kSettingsPageAdvancedCache];

  page.SetRedraw(FALSE);

  // History
  text = ToWstr(library::history.items.size()) + L" item(s)";
  page.SetDlgItemText(IDC_STATIC_CACHE1, text.c_str());

  // Image files
  text = ToWstr(taiga::stats.image_count) + L" item(s), " + ToSizeString(taiga::stats.image_size);
  page.SetDlgItemText(IDC_STATIC_CACHE2, text.c_str());

  // Torrent files
  text = ToWstr(taiga::stats.torrent_count) + L" item(s), " + ToSizeString(taiga::stats.torrent_size);
  page.SetDlgItemText(IDC_STATIC_CACHE3, text.c_str());

  // Torrent history
  text = ToWstr(track::aggregator.archive.Size()) + L" item(s)";
  page.SetDlgItemText(IDC_STATIC_CACHE4, text.c_str());

  page.SetRedraw(TRUE);
  page.RedrawWindow(nullptr, nullptr,
                    RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void SettingsDialog::RefreshTorrentFilterList(HWND hwnd_list) {
  win::ListView list = hwnd_list;
  list.DeleteAllItems();

  for (const auto& feed_filter : feed_filters_) {
    AddTorrentFilterToList(hwnd_list, feed_filter);
  }

  list.SetWindowHandle(nullptr);
}

void SettingsDialog::UpdateTorrentFilterList(HWND hwnd_list) {
  win::ListView list = hwnd_list;

  for (size_t i = 0; i < feed_filters_.size(); ++i) {
    const auto& feed_filter = feed_filters_.at(i);
    list.SetItemParam(i, reinterpret_cast<LPARAM>(&feed_filter));
  }

  list.SetWindowHandle(nullptr);
}

}  // namespace ui
