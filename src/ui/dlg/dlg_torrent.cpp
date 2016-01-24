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

#include "base/file.h"
#include "base/gfx.h"
#include "base/string.h"
#include "base/url.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "taiga/resource.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/recognition.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_torrent.h"
#include "ui/dialog.h"
#include "ui/list.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace ui {

TorrentDialog DlgTorrent;

BOOL TorrentDialog::OnInitDialog() {
  // Create list
  list_.Attach(GetDlgItem(IDC_LIST_TORRENT));
  list_.EnableGroupView(true);
  list_.SetExtendedStyle(LVS_EX_AUTOSIZECOLUMNS | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER |
                         LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  list_.SetImageList(ui::Theme.GetImageList16().GetHandle());
  list_.SetTheme();

  // Insert list columns
  list_.InsertColumn(0, ScaleX(240), ScaleX(240), LVCFMT_LEFT,  L"Anime title");
  list_.InsertColumn(1,  ScaleX(60),  ScaleX(60), LVCFMT_RIGHT, L"Episode");
  list_.InsertColumn(2, ScaleX(100), ScaleX(100), LVCFMT_LEFT,  L"Group");
  list_.InsertColumn(3,  ScaleX(70),  ScaleX(70), LVCFMT_RIGHT, L"Size");
  list_.InsertColumn(4, ScaleX(100), ScaleX(100), LVCFMT_LEFT,  L"Video");
  list_.InsertColumn(5, ScaleX(200), ScaleX(200), LVCFMT_LEFT,  L"Description");
  list_.InsertColumn(6, ScaleX(200), ScaleX(200), LVCFMT_LEFT,  L"Filename");
  list_.InsertColumn(7, ScaleX(190), ScaleX(190), LVCFMT_RIGHT, L"Release date");
  // Insert list groups
  list_.InsertGroup(0, L"Anime");
  list_.InsertGroup(1, L"Batch");
  list_.InsertGroup(2, L"Other");

  // Create main toolbar
  toolbar_.Attach(GetDlgItem(IDC_TOOLBAR_TORRENT));
  toolbar_.SetImageList(ui::Theme.GetImageList16().GetHandle(), ScaleX(16), ScaleY(16));
  toolbar_.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  // Insert toolbar buttons
  BYTE fsState = TBSTATE_ENABLED;
  BYTE fsStyle = BTNS_AUTOSIZE | BTNS_SHOWTEXT;
  toolbar_.InsertButton(0, ui::kIcon16_Refresh,  100, fsState, fsStyle, 0, L"Check new torrents", NULL);
  toolbar_.InsertButton(1, 0, 0, 0, BTNS_SEP, NULL, NULL, NULL);
  toolbar_.InsertButton(2, ui::kIcon16_Download, 101, fsState, fsStyle, 0, L"Download marked torrents", NULL);
  toolbar_.InsertButton(3, ui::kIcon16_Cross,    102, fsState, fsStyle, 0, L"Discard all", NULL);
  toolbar_.InsertButton(4, 0, 0, 0, BTNS_SEP, NULL, NULL, NULL);
  toolbar_.InsertButton(5, ui::kIcon16_Settings, 103, fsState, fsStyle, 0, L"Settings", NULL);

  // Create rebar
  rebar_.Attach(GetDlgItem(IDC_REBAR_TORRENT));
  // Insert rebar bands
  rebar_.InsertBand(NULL, 0, 0, 0, 0, 0, 0, 0, 0,
      RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE, RBBS_NOGRIPPER);
  rebar_.InsertBand(toolbar_.GetWindowHandle(), GetSystemMetrics(SM_CXSCREEN), 0, 0, 0, 0, 0, 0,
      HIWORD(toolbar_.GetButtonSize()) + (HIWORD(toolbar_.GetPadding()) / 2),
      RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE, RBBS_NOGRIPPER);

  // Refresh list
  RefreshList();

  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

INT_PTR TorrentDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL: {
      return list_.SendMessage(uMsg, wParam, lParam);
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL TorrentDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Check new torrents
    case 100: {
      DlgMain.edit.SetText(L"");
      if (GetKeyState(VK_CONTROL) & 0x8000) {
        auto feed = Aggregator.GetFeed(kFeedCategoryLink);
        if (feed) {
          feed->Load();
          Aggregator.ExamineData(*feed);
        }
        RefreshList();
      } else {
        Aggregator.CheckFeed(kFeedCategoryLink, Settings[taiga::kTorrent_Discovery_Source]);
      }
      return TRUE;
    }
    // Download marked torrents
    case 101: {
      Aggregator.Download(kFeedCategoryLink, nullptr);
      return TRUE;
    }
    // Discard marked torrents
    case 102: {
      for (int i = 0; i < list_.GetItemCount(); i++) {
        if (list_.GetCheckState(i) == TRUE) {
          FeedItem* feed_item = reinterpret_cast<FeedItem*>(list_.GetItemParam(i));
          if (feed_item) {
            feed_item->state = kFeedItemDiscardedNormal;
            list_.SetCheckState(i, FALSE);
            Aggregator.AddToArchive(feed_item->title);
          }
        }
      }
      return TRUE;
    }
    // Settings
    case 103: {
      ShowDlgSettings(kSettingsSectionTorrents, kSettingsPageTorrentsDiscovery);
      return TRUE;
    }
  }

  return FALSE;
}

void TorrentDialog::OnContextMenu(HWND hwnd, POINT pt) {
  int item_index = list_.GetNextItem(-1, LVIS_SELECTED);
  if (item_index == -1)
    return;

  auto feed_item = reinterpret_cast<FeedItem*>(list_.GetItemParam(item_index));
  if (!feed_item)
    return;

  if (pt.x == -1 || pt.y == -1)
    GetPopupMenuPositionForSelectedListItem(list_, pt);

  ui::Menus.UpdateTorrentsList(*feed_item);
  std::wstring answer = ui::Menus.Show(GetWindowHandle(), pt.x, pt.y, L"TorrentListRightClick");

  if (answer == L"DownloadTorrent") {
    Aggregator.Download(kFeedCategoryLink, feed_item);

  } else if (answer == L"Info") {
    ShowDlgAnimeInfo(feed_item->episode_data.anime_id);

  } else if (answer == L"TorrentInfo") {
    ExecuteLink(feed_item->info_link);

  } else if (answer == L"DiscardTorrent") {
    feed_item->state = kFeedItemDiscardedNormal;
    list_.SetCheckState(item_index, FALSE);
    Aggregator.AddToArchive(feed_item->title);

  } else if (answer == L"DiscardTorrents") {
    auto anime_item = AnimeDatabase.FindItem(feed_item->episode_data.anime_id);
    if (anime_item) {
      for (int i = 0; i < list_.GetItemCount(); i++) {
        feed_item = reinterpret_cast<FeedItem*>(list_.GetItemParam(i));
        if (feed_item && feed_item->episode_data.anime_id == anime_item->GetId()) {
          feed_item->state = kFeedItemDiscardedNormal;
          list_.SetCheckState(i, FALSE);
        }
      }
      Aggregator.filter_manager.AddFilter(
          kFeedFilterActionDiscard, kFeedFilterMatchAll, kFeedFilterOptionDefault,
          true, L"Discard \"" + anime_item->GetTitle() + L"\"");
      Aggregator.filter_manager.filters.back().AddCondition(
          kFeedFilterElement_Meta_Id, kFeedFilterOperator_Equals,
          ToWstr(anime_item->GetId()));
    }

  } else if (answer == L"SelectFansub") {
    int anime_id = feed_item->episode_data.anime_id;
    std::wstring group_name = feed_item->episode_data.release_group();
    if (anime::IsValidId(anime_id) && !group_name.empty()) {
      for (int i = 0; i < list_.GetItemCount(); i++) {
        feed_item = reinterpret_cast<FeedItem*>(list_.GetItemParam(i));
        if (feed_item && !IsEqual(feed_item->episode_data.release_group(), group_name)) {
          feed_item->state = kFeedItemDiscardedNormal;
          list_.SetCheckState(i, FALSE);
        }
      }
      anime::SetFansubFilter(anime_id, group_name);
    }

  } else if (answer == L"MoreTorrents") {
    Search(Settings[taiga::kTorrent_Discovery_SearchUrl], feed_item->episode_data.anime_title());

  } else if (answer == L"SearchService") {
    ExecuteAction(L"SearchAnime(" + feed_item->episode_data.anime_title() + L")");
  }
}

LRESULT TorrentDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // ListView control
  if (idCtrl == IDC_LIST_TORRENT) {
    switch (pnmh->code) {
      // Column click
      case LVN_COLUMNCLICK: {
        LPNMLISTVIEW lplv = (LPNMLISTVIEW)pnmh;
        int order = 1;
        switch (lplv->iSubItem) {
          case 1:  // Episode
          case 3:  // Size
          case 7:  // Release date
            order = -1;
            break;
        }
        if (lplv->iSubItem == list_.GetSortColumn())
          order = list_.GetSortOrder() * -1;
        switch (lplv->iSubItem) {
          // Episode
          case 1:
            list_.Sort(lplv->iSubItem, order, ui::kListSortEpisodeRange, ui::ListViewCompareProc);
            break;
          // File size
          case 3:
            list_.Sort(lplv->iSubItem, order, ui::kListSortFileSize, ui::ListViewCompareProc);
            break;
          // Release date
          case 7:
            list_.Sort(lplv->iSubItem, order, ui::kListSortRfc822DateTime, ui::ListViewCompareProc);
            break;
          // Other columns
          default:
            list_.Sort(lplv->iSubItem, order, ui::kListSortDefault, ui::ListViewCompareProc);
            break;
        }
        break;
      }

      // Check/uncheck
      case LVN_ITEMCHANGED: {
        if (!list_.IsVisible())
          break;
        LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(pnmh);
        if (pnmv->uOldState != 0 && (pnmv->uNewState == 0x1000 || pnmv->uNewState == 0x2000)) {
          const bool checked = list_.GetCheckState(pnmv->iItem) == TRUE;
          const int group = list_.GetItemGroup(list_.last_checked_item);
          if (list_.last_checked_item > -1 && (GetKeyState(VK_SHIFT) & 0x8000) &&
              list_.GetItemGroup(pnmv->iItem) == group) {
            int item_index = min(pnmv->iItem, list_.last_checked_item);
            const int last_index = max(pnmv->iItem, list_.last_checked_item);
            do {
              if (list_.GetItemGroup(item_index) == group)
                list_.SetCheckState(item_index, checked);
              if (item_index == last_index)
                break;
              item_index = list_.GetNextItem(item_index, LVNI_ALL);
            } while (item_index > -1);
          }
          if (checked)
            list_.last_checked_item = pnmv->iItem;
          int checked_count = 0;
          for (int i = 0; i < list_.GetItemCount(); i++) {
            if (list_.GetCheckState(i))
              checked_count++;
          }
          if (checked_count == 1) {
            DlgMain.ChangeStatus(L"Marked 1 torrent.");
          } else {
            DlgMain.ChangeStatus(L"Marked " + ToWstr(checked_count) + L" torrents.");
          }
          FeedItem* feed_item = reinterpret_cast<FeedItem*>(list_.GetItemParam(pnmv->iItem));
          if (feed_item) {
            feed_item->state = checked ? kFeedItemSelected : kFeedItemDiscardedNormal;
          }
        }
        break;
      }

      // Key press
      case LVN_KEYDOWN: {
        auto pnkd = reinterpret_cast<LPNMLVKEYDOWN>(pnmh);
        switch (pnkd->wVKey) {
          case VK_RETURN: {
            auto param = GetParamFromSelectedListItem(list_);
            if (param) {
              auto feed_item = reinterpret_cast<FeedItem*>(param);
              Aggregator.Download(kFeedCategoryLink, feed_item);
              return TRUE;
            }
            break;
          }
        }
        break;
      }

      // Double click
      case NM_DBLCLK: {
        if (list_.GetSelectedCount() > 0) {
          LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
          if (lpnmitem->iItem == -1)
            break;
          FeedItem* feed_item = reinterpret_cast<FeedItem*>(list_.GetItemParam(lpnmitem->iItem));
          Aggregator.Download(kFeedCategoryLink, feed_item);
        }
        break;
      }

      // Custom draw
      case NM_CUSTOMDRAW: {
        LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmh);
        switch (pCD->nmcd.dwDrawStage) {
          case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;
          case CDDS_ITEMPREPAINT:
            // Alternate background color
            if ((pCD->nmcd.dwItemSpec % 2) && !list_.IsGroupViewEnabled())
              pCD->clrTextBk = ChangeColorBrightness(GetSysColor(COLOR_WINDOW), -0.03f);
            return CDRF_NOTIFYSUBITEMDRAW;

          case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
            FeedItem* feed_item = reinterpret_cast<FeedItem*>(pCD->nmcd.lItemlParam);
            if (feed_item) {
              if (Taiga.debug_mode) {
                // Change background color
                switch (feed_item->state) {
                  case kFeedItemDiscardedNormal:
                  case kFeedItemDiscardedInactive:
                  case kFeedItemDiscardedHidden:
                    pCD->clrTextBk = ui::kColorLightRed;
                    break;
                  case kFeedItemSelected:
                    pCD->clrTextBk = ui::kColorLightGreen;
                    break;
                  default:
                    pCD->clrTextBk = GetSysColor(COLOR_WINDOW);
                    break;
                }
              }
              // Change text color
              if (feed_item->state == kFeedItemDiscardedInactive) {
                pCD->clrText = GetSysColor(COLOR_GRAYTEXT);
              } else if (feed_item->episode_data.new_episode) {
                pCD->clrText = GetSysColor(pCD->iSubItem == 1 ? COLOR_HIGHLIGHT : COLOR_WINDOWTEXT);
              }
            }
            return CDRF_NOTIFYPOSTPAINT;
          }
        }
      }
    }
  }

  return 0;
}

void TorrentDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      win::Rect rcWindow;
      rcWindow.Set(0, 0, size.cx, size.cy);
      // Resize rebar
      rebar_.SendMessage(WM_SIZE, 0, 0);
      rcWindow.top += rebar_.GetBarHeight() + ScaleY(win::kControlMargin / 2);
      // Resize list
      list_.SetPosition(NULL, rcWindow);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void TorrentDialog::EnableInput(bool enable) {
  // Enable/disable toolbar buttons
  toolbar_.EnableButton(100, enable);
  toolbar_.EnableButton(102, enable);
  // Enable/disable list
  list_.Enable(enable);
}

void TorrentDialog::RefreshList() {
  if (!IsWindow())
    return;

  Feed* feed = Aggregator.GetFeed(kFeedCategoryLink);
  if (!feed)
    return;

  // Disable drawing
  list_.SetRedraw(FALSE);

  // Clear list
  list_.DeleteAllItems();
  list_.last_checked_item = -1;

  // Add items
  for (auto it = feed->items.begin(); it != feed->items.end(); ++it) {
    // Skip item if it was discarded and hidden
    if (it->state == kFeedItemDiscardedHidden)
      continue;

    std::wstring title, number, video;
    int group = it->GetTorrentCategory();
    int icon = StatusToIcon(anime::kUnknownStatus);
    auto anime_item = AnimeDatabase.FindItem(it->episode_data.anime_id);
    if (anime_item) {
      icon = StatusToIcon(anime_item->GetAiringStatus());
      title = anime_item->GetTitle();
    } else if (!it->episode_data.anime_title().empty()) {
      title = it->episode_data.anime_title();
    } else {
      group = kTorrentCategoryOther;
      title = it->title;
    }
    if (!it->episode_data.elements().empty(anitomy::kElementEpisodeNumber)) {
      number = anime::GetEpisodeRange(it->episode_data);
    } else if (!it->episode_data.elements().empty(anitomy::kElementVolumeNumber)) {
      number = L"Vol. " + GetVolumeRange(it->episode_data);
    }
    if (it->episode_data.release_version() != 1) {
      number += L"v" + ToWstr(it->episode_data.release_version());
    }
    video = it->episode_data.video_terms();
    if (!it->episode_data.video_resolution().empty()) {
      if (!video.empty()) video += L" ";
      video += it->episode_data.video_resolution();
    }
    int index = list_.InsertItem(it - feed->items.begin(),
                                 group, icon, 0, NULL, title.c_str(),
                                 reinterpret_cast<LPARAM>(&(*it)));
    list_.SetItem(index, 1, number.c_str());
    list_.SetItem(index, 2, it->episode_data.release_group().c_str());
    list_.SetItem(index, 3, it->episode_data.file_size.c_str());
    list_.SetItem(index, 4, video.c_str());
    list_.SetItem(index, 5, it->description.c_str());
    list_.SetItem(index, 6, it->episode_data.file_name_with_extension().c_str());
    list_.SetItem(index, 7, ConvertRfc822ToLocal(it->pub_date).c_str());
    list_.SetCheckState(index, it->state == kFeedItemSelected);
  }

  // Resize columns
  list_.SetColumnWidth(1, LVSCW_AUTOSIZE);
  list_.SetColumnWidth(2, LVSCW_AUTOSIZE);
  list_.SetColumnWidth(3, LVSCW_AUTOSIZE);
  list_.SetColumnWidth(4, LVSCW_AUTOSIZE);
  list_.SetColumnWidth(7, LVSCW_AUTOSIZE_USEHEADER);

  // Redraw
  list_.SetRedraw(TRUE);
  list_.RedrawWindow(nullptr, nullptr,
                     RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void TorrentDialog::Search(std::wstring url, int anime_id) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return;

  std::wstring title = anime_item->GetTitle();
  if (anime_item->GetUseAlternative() &&
      anime_item->UserSynonymsAvailable())
    title = anime_item->GetUserSynonyms().front();

  Search(url, title);
}

void TorrentDialog::Search(std::wstring url, std::wstring title) {
  DlgMain.navigation.SetCurrentPage(kSidebarItemFeeds);
  DlgMain.edit.SetText(title);
  DlgMain.ChangeStatus(L"Searching torrents for \"" + title + L"\"...");

  ReplaceString(url, L"%title%", title);
  Aggregator.CheckFeed(kFeedCategoryLink, url);
}

void TorrentDialog::SetTimer(int ticks) {
  if (!IsWindow())
    return;

  std::wstring text = L"Check new torrents";

  if (Settings.GetBool(taiga::kTorrent_Discovery_AutoCheckEnabled) &&
      Settings.GetInt(taiga::kTorrent_Discovery_AutoCheckInterval) > 0) {
    text += L" [" + ToTimeString(ticks) + L"]";
  }

  toolbar_.SetButtonText(0, text.c_str());
}

}  // namespace ui
