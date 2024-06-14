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

#include <algorithm>
#include <set>

#include <windows/win/clipboard.h>
#include <windows/win/version.h>

#include "base/format.h"
#include "base/gfx.h"
#include "base/string.h"
#include "media/library/list_util.h"
#include "media/anime_db.h"
#include "media/anime_filter.h"
#include "media/anime_util.h"
#include "ui/resource.h"
#include "sync/service.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "taiga/timer.h"
#include "track/play.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_torrent.h"
#include "ui/command.h"
#include "ui/dialog.h"
#include "ui/list.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace ui {

enum AnimeListTooltips {
  kTooltipFirst,
  kTooltipAnimeSeason = kTooltipFirst,
  kTooltipAnimeStatus,
  kTooltipEpisodeAvailable,
  kTooltipEpisodeMinus,
  kTooltipEpisodePlus,
  kTooltipUserLastUpdated,
  kTooltipLast,
};

AnimeListDialog DlgAnimeList;

AnimeListDialog::AnimeListDialog()
    : current_status_(anime::MyStatus::Watching) {
}

BOOL AnimeListDialog::OnInitDialog() {
  // Create tab control
  tab.Attach(GetDlgItem(IDC_TAB_MAIN));

  // Create main list
  listview.parent = this;
  listview.Attach(GetDlgItem(IDC_LIST_MAIN));
  listview.SetExtendedStyle(LVS_EX_COLUMNSNAPPOINTS |
                            LVS_EX_DOUBLEBUFFER |
                            LVS_EX_FULLROWSELECT |
                            LVS_EX_HEADERDRAGDROP |
                            LVS_EX_INFOTIP |
                            LVS_EX_LABELTIP |
                            LVS_EX_TRACKSELECT);
  listview.SetHoverTime(60 * 1000);
  listview.SetTheme();

  // Create list tooltips
  listview.tooltips.Create(listview.GetWindowHandle());
  listview.tooltips.SetDelayTime(30000, -1, 0);
  listview.tooltips.SetMaxWidth(::GetSystemMetrics(SM_CXSCREEN));  // Required for line breaks
  for (int id = kTooltipFirst; id < kTooltipLast; ++id) {
    listview.tooltips.AddTip(id, nullptr, nullptr, nullptr, false);
  }

  // Insert list columns
  listview.InitializeColumns();
  listview.InsertColumns();

  // Insert tabs and list groups
  listview.InsertGroup(
      static_cast<int>(anime::MyStatus::NotInList),
      ui::TranslateMyStatus(anime::MyStatus::NotInList, false).c_str());
  for (const auto status : anime::kMyStatuses) {
    tab.InsertItem(static_cast<int>(status) - 1,
                   ui::TranslateMyStatus(status, true).c_str(),
                   static_cast<LPARAM>(status));
    listview.InsertGroup(static_cast<int>(status),
                         ui::TranslateMyStatus(status, false).c_str());
  }

  // Track mouse leave event for the list view
  TRACKMOUSEEVENT tme = {0};
  tme.cbSize = sizeof(TRACKMOUSEEVENT);
  tme.dwFlags = TME_LEAVE;
  tme.hwndTrack = listview.GetWindowHandle();
  TrackMouseEvent(&tme);

  // Refresh
  RefreshList(anime::MyStatus::Watching);
  RefreshTabs(anime::MyStatus::Watching);

  // Success
  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

INT_PTR AnimeListDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_MOUSEMOVE: {
      // Drag list item
      if (listview.dragging) {
        bool allow_drop = false;

        if (tab.HitTest() > -1)
          allow_drop = true;

        if (!allow_drop) {
          POINT pt;
          GetCursorPos(&pt);
          win::Rect rect_edit;
          DlgMain.edit.GetWindowRect(&rect_edit);
          if (rect_edit.PtIn(pt))
            allow_drop = true;
        }

        if (!allow_drop) {
          TVHITTESTINFO ht = {0};
          DlgMain.treeview.HitTest(&ht, true);
          if (ht.flags & TVHT_ONITEM) {
            int index = DlgMain.treeview.GetItemData(ht.hItem);
            switch (index) {
              case kSidebarItemSearch:
              case kSidebarItemFeeds:
                allow_drop = true;
                break;
            }
          }
        }

        POINT pt;
        GetCursorPos(&pt);
        ::ScreenToClient(DlgMain.GetWindowHandle(), &pt);
        win::Rect rect_header;
        ::GetClientRect(listview.GetHeader(), &rect_header);
        listview.drag_image.DragMove(pt.x + (GetSystemMetrics(SM_CXCURSOR) / 2),
                                     pt.y + rect_header.Height());
        SetSharedCursor(allow_drop ? IDC_ARROW : IDC_NO);
      }
      break;
    }

    case WM_LBUTTONUP: {
      // Drop list item
      if (listview.dragging) {
        listview.drag_image.DragLeave(DlgMain.GetWindowHandle());
        listview.drag_image.EndDrag();
        listview.drag_image.Destroy();
        listview.dragging = false;
        ReleaseCapture();

        int anime_id = GetCurrentId();
        auto anime_ids = GetCurrentIds();
        auto anime_item = GetCurrentItem();
        if (!anime_item)
          break;

        int tab_index = tab.HitTest();
        if (tab_index > -1) {
          int status = tab.GetItemParam(tab_index);
          if (anime_item->IsInList()) {
            ExecuteCommand(L"EditStatus({})"_format(status), 0,
                          reinterpret_cast<LPARAM>(&anime_ids));
          } else {
            for (const auto& id : anime_ids) {
              anime::db.AddToList(id, static_cast<anime::MyStatus>(status));
            }
          }
          break;
        }

        const auto text = anime::GetPreferredTitle(*anime_item);

        POINT pt;
        GetCursorPos(&pt);
        win::Rect rect_edit;
        DlgMain.edit.GetWindowRect(&rect_edit);
        if (rect_edit.PtIn(pt)) {
          DlgMain.edit.SetText(text);
          break;
        }

        TVHITTESTINFO ht = {0};
        DlgMain.treeview.HitTest(&ht, true);
        if (ht.flags & TVHT_ONITEM) {
          int index = DlgMain.treeview.GetItemData(ht.hItem);
          switch (index) {
            case kSidebarItemSearch:
              ExecuteCommand(L"SearchAnime({})"_format(text));
              break;
            case kSidebarItemFeeds:
              DlgTorrent.Search(taiga::settings.GetTorrentDiscoverySearchUrl(), anime_id);
              break;
          }
        }
      }
      break;
    }

    case WM_MEASUREITEM: {
      if (wParam == IDC_LIST_MAIN) {
        auto mis = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        mis->itemHeight = 48;
        return TRUE;
      }
      break;
    }

    case WM_DRAWITEM: {
      if (wParam == IDC_LIST_MAIN) {
        auto dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        win::Dc dc = dis->hDC;
        win::Rect rect = dis->rcItem;

        int anime_id = dis->itemData;
        auto anime_item = anime::db.Find(anime_id);
        if (!anime_item)
          return TRUE;

        if ((dis->itemState & ODS_SELECTED) == ODS_SELECTED)
          dc.FillRect(rect, ui::kColorLightBlue);
        rect.Inflate(-2, -2);
        dc.FillRect(rect, ui::kColorLightGray);

        // Draw image
        win::Rect rect_image = rect;
        rect_image.right = rect_image.left + static_cast<int>(rect_image.Height() / 1.4);
        dc.FillRect(rect_image, ui::kColorGray);
        if (const auto image = ui::image_db.GetImage(anime_id)) {
          int sbm = dc.SetStretchBltMode(HALFTONE);
          dc.StretchBlt(rect_image.left, rect_image.top,
                        rect_image.Width(), rect_image.Height(),
                        image->dc.Get(), 0, 0,
                        image->rect.Width(),
                        image->rect.Height(),
                        SRCCOPY);
          dc.SetStretchBltMode(sbm);
        }

        // Draw title
        rect.left += rect_image.Width() + 8;
        int bk_mode = dc.SetBkMode(TRANSPARENT);
        const auto& title = anime::GetPreferredTitle(*anime_item);
        dc.AttachFont(ui::Theme.GetHeaderFont());
        dc.DrawText(title.c_str(), title.length(), rect,
                    DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);
        dc.DetachFont();

        // Draw second line of information
        rect.top += 20;
        COLORREF text_color = dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
        std::wstring text = ToWstr(anime_item->GetMyLastWatchedEpisode()) + L"/" +
                       ToWstr(anime_item->GetEpisodeCount());
        dc.DrawText(text.c_str(), -1, rect,
                    DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);
        dc.SetTextColor(text_color);
        dc.SetBkMode(bk_mode);

        // Draw progress bar
        rect.left -= 2;
        rect.top += 12;
        rect.bottom = rect.top + 12;
        rect.right -= 8;
        listview.DrawProgressBar(dc.Get(), &rect, dis->itemID, *anime_item);

        dc.DetachDc();
        return TRUE;
      }
      break;
    }

    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL: {
      return listview.SendMessage(uMsg, wParam, lParam);
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////

void AnimeListDialog::OnContextMenu(HWND hwnd, POINT pt) {
  if (HitTestListHeader(listview, pt))
    hwnd = listview.GetHeader();

  if (hwnd == listview.GetWindowHandle()) {
    if (listview.GetSelectedCount() > 0) {
      if (pt.x == -1 || pt.y == -1)
        GetPopupMenuPositionForSelectedListItem(listview, pt);
      std::wstring menu_name = L"RightClick";
      LPARAM parameter = GetCurrentId();
      auto anime_ids = GetCurrentIds();
      if (kColumnUserRating == listview.FindColumnAtSubItemIndex(listview.HitTest(true))) {
        menu_name = L"EditScore";
        parameter = reinterpret_cast<LPARAM>(&anime_ids);
      } else if (listview.GetSelectedCount() > 1) {
        menu_name = L"Edit";
        parameter = reinterpret_cast<LPARAM>(&anime_ids);
      }
      ui::Menus.UpdateAll(GetCurrentItem());
      const auto command = ui::Menus.Show(DlgMain.GetWindowHandle(), pt.x, pt.y, menu_name.c_str());
      if (command == L"EditDelete()")
        parameter = reinterpret_cast<LPARAM>(&anime_ids);
      if (command == L"InvertSelection") {
        for (int i = 0; i < listview.GetItemCount(); ++i) {
          bool selected = ListView_GetItemState(listview.GetWindowHandle(), i, LVIS_SELECTED);
          listview.SelectItem(i, !selected);
        }
      } else {
        ExecuteCommand(command, 0, parameter);
      }
    }

  } else if (hwnd == listview.GetHeader()) {
    ui::Menus.UpdateAnimeListHeaders();
    auto command = ui::Menus.Show(DlgMain.GetWindowHandle(), pt.x, pt.y, L"AnimeListHeaders");
    if (!command.empty()) {
      bool reset = false;
      if (command == L"ResetAnimeListHeaders()") {
        reset = true;
      } else {
        const auto column_type = listview.TranslateColumnName(command);
        auto& column = listview.columns[column_type];
        column.visible = !column.visible;
        listview.UpdateColumnSetting(column_type);
      }
      listview.RefreshColumns(reset);
    }
  }
}

LRESULT AnimeListDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // ListView control
  if (idCtrl == IDC_LIST_MAIN || pnmh->hwndFrom == listview.GetHeader()) {
    return OnListNotify(reinterpret_cast<LPARAM>(pnmh));

  // Tab control
  } else if (idCtrl == IDC_TAB_MAIN) {
    return OnTabNotify(reinterpret_cast<LPARAM>(pnmh));
  }

  return 0;
}

void AnimeListDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      // Resize tab
      win::Rect rcWindow(-1, 0, size.cx + 3, size.cy + 2);
      if (win::GetVersion() < win::kVersion8)
        rcWindow.top -= 1;
      tab.SetPosition(nullptr, rcWindow);
      // Resize list
      tab.AdjustRect(nullptr, FALSE, &rcWindow);
      rcWindow.Set(0, rcWindow.top - 1, size.cx, size.cy);
      listview.SetPosition(nullptr, rcWindow, 0);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

/* ListView control */

AnimeListDialog::ListView::ListView()
    : dragging(false), hot_item(-1), parent(nullptr), progress_bars_visible(false) {
  button_visible[0] = false;
  button_visible[1] = false;
  button_visible[2] = false;
}

void AnimeListDialog::ListView::ExecuteCommand(AnimeListAction action,
                                               int anime_id) {
  switch (action) {
    case kAnimeListActionNothing:
      break;
    case kAnimeListActionEdit:
      ShowDlgAnimeEdit(anime_id);
      break;
    case kAnimeListActionOpenFolder:
      ui::ExecuteCommand(L"OpenFolder", 0, anime_id);
      break;
    case kAnimeListActionPlayNext:
      track::PlayNextEpisode(anime_id);
      break;
    case kAnimeListActionInfo:
      ShowDlgAnimeInfo(anime_id);
      break;
    case kAnimeListActionPage:
      ui::ExecuteCommand(L"ViewAnimePage", 0, anime_id);
      break;
  }
}

int AnimeListDialog::ListView::GetDefaultSortOrder(AnimeListColumn column) {
  switch (column) {
    case kColumnAnimeRating:
    case kColumnAnimeSeason:
    case kColumnAnimeStatus:
    case kColumnAnimeType:
    case kColumnUserLastUpdated:
    case kColumnUserDateStarted:
    case kColumnUserDateCompleted:
    case kColumnUserProgress:
    case kColumnUserRating:
      return -1;
    case kColumnAnimeTitle:
    case kColumnUserNotes:
    default:
      return 1;
  }
}

int AnimeListDialog::ListView::GetSortType(AnimeListColumn column) {
  switch (column) {
    case kColumnUserDateStarted:
      return ui::kListSortMyDateStart;
    case kColumnUserDateCompleted:
      return ui::kListSortMyDateCompleted;
    case kColumnUserLastUpdated:
      return ui::kListSortLastUpdated;
    case kColumnUserProgress:
      return ui::kListSortProgress;
    case kColumnUserRating:
      return ui::kListSortMyScore;
    case kColumnAnimeRating:
      return ui::kListSortScore;
    case kColumnAnimeSeason:
      return ui::kListSortSeason;
    case kColumnAnimeStatus:
      return ui::kListSortStatus;
    default:
      return ui::kListSortDefault;
  }
}

void AnimeListDialog::ListView::SortFromSettings() {
  const auto set_options = [&](AnimeListColumn column, int order, bool secondary) {
    if (column == kColumnUnknown || column < 0)
      column = kColumnAnimeTitle;

    win::ListView::SortOptions sort_options;
    sort_options.column = columns[column].index;
    sort_options.order = order;
    sort_options.type = GetSortType(column);

    SetSortOptions(sort_options, secondary);
  };

  set_options(TranslateColumnName(taiga::settings.GetAppListSortColumnPrimary()),
              taiga::settings.GetAppListSortOrderPrimary(), false);
  set_options(TranslateColumnName(taiga::settings.GetAppListSortColumnSecondary()),
              taiga::settings.GetAppListSortOrderSecondary(), true);

  win::ListView::Sort(ui::AnimeListCompareProc);

  parent->RebuildIdCache();
}

std::wstring GetAvailableEpisodesTooltip(const anime::Item& anime_item) {
  std::wstring text;

  const int eps_aired_estimated = anime::GetLastEpisodeNumber(anime_item);
  const int eps_available = anime_item.GetAvailableEpisodeCount();
  const int available_episodes = std::max(eps_aired_estimated, eps_available);

  // Find missing episodes
  std::vector<int> missing_episodes;
  for (int i = 1; i <= available_episodes; ++i) {
    if (!anime_item.IsEpisodeAvailable(i))
      missing_episodes.push_back(i);
  }
  // Collapse episode ranges (e.g. "1|3|4|5" -> "1|3-5")
  std::vector<std::pair<int, int>> missing_episode_ranges;
  if (!missing_episodes.empty()) {
    missing_episode_ranges.push_back({0, 0});
    for (const auto i : missing_episodes) {
      auto& current_pair = missing_episode_ranges.back();
      if (current_pair.first == 0) {
        current_pair = {i, i};
      } else if (current_pair.second == i - 1) {
        current_pair.second = i;
      } else {
        missing_episode_ranges.push_back({i, i});
      }
    }
  }

  if (missing_episodes.size() == available_episodes) {
    AppendString(text, L"All episodes are missing");
  } else if (missing_episodes.empty()) {
    AppendString(text, L"All episodes are in library folders");
  } else {
    std::wstring missing_text;
    for (const auto& range : missing_episode_ranges)  {
      AppendString(missing_text, L"#" + ToWstr(range.first) + (range.second > range.first ?
                                 L"-" + ToWstr(range.second) : L""));
    }
    AppendString(text, L"Missing: " + missing_text);
  }

  if (!anime::IsFinishedAiring(anime_item)) {
    if (anime_item.GetNextEpisodeTime()) {
      const auto next_episode_number = anime_item.GetLastAiredEpisodeNumber() + 1;
      const auto next_episode_time = GetAbsoluteTimeString(anime_item.GetNextEpisodeTime(), "%A %H:%M");
      AppendString(text, L"Episode #{} airing {}"_format(next_episode_number, next_episode_time), L"\r\n");
    } else if (eps_aired_estimated > anime_item.GetMyLastWatchedEpisode()) {
      AppendString(text, L"Aired: #{} (estimated)"_format(eps_aired_estimated), L"\r\n");
    }
  }

  return text;
}

void AnimeListDialog::ListView::RefreshItem(int index) {
  for (int i = 0; i < 3; i++) {
    button_rect[i].SetEmpty();
    button_visible[i] = false;
  }

  if (GetSelectedCount() > 1)
    index = -1;

  hot_item = index;

  if (index < 0 || !progress_bars_visible) {
    tooltips.NewToolRect(kTooltipEpisodeAvailable, nullptr);
    tooltips.NewToolRect(kTooltipEpisodeMinus, nullptr);
    tooltips.NewToolRect(kTooltipEpisodePlus, nullptr);
  }
  if (index < 0) {
    tooltips.NewToolRect(kTooltipAnimeSeason, nullptr);
    tooltips.NewToolRect(kTooltipAnimeStatus, nullptr);
    tooltips.NewToolRect(kTooltipUserLastUpdated, nullptr);
    return;
  }

  const int anime_id = GetItemParam(index);
  const auto anime_item = anime::db.Find(anime_id);

  if (!anime_item || !anime_item->IsInList())
    return;

  POINT pt;
  ::GetCursorPos(&pt);
  ::ScreenToClient(GetWindowHandle(), &pt);

  auto get_subitem_rect = [this, &index](AnimeListColumn column, win::Rect& rect_item) {
    GetSubItemRect(index, columns[column].index, &rect_item);
    rect_item.Inflate(0, 2);
  };

  auto update_tooltip = [this](UINT id, LPCWSTR text, LPRECT rect) {
    tooltips.NewToolRect(id, rect);
    tooltips.UpdateText(id, text);
  };

  if (columns[kColumnAnimeSeason].visible) {
    win::Rect rect_item;
    get_subitem_rect(kColumnAnimeSeason, rect_item);
    if (rect_item.PtIn(pt)) {
      auto date_start = anime_item->GetDateStart();
      if (date_start.year() && date_start.month() && date_start.day()) {
        const auto text = date_start.to_string();
        update_tooltip(kTooltipAnimeSeason, text.c_str(), &rect_item);
      }
    }
  }

  if (columns[kColumnAnimeStatus].visible) {
    win::Rect rect_item;
    get_subitem_rect(kColumnAnimeStatus, rect_item);
    if (rect_item.PtIn(pt)) {
      const bool playing = anime_id == CurrentEpisode.anime_id;
      const std::wstring text = playing ? L"Now playing" :
          ui::TranslateStatus(anime_item->GetAiringStatus());
      update_tooltip(kTooltipAnimeStatus, text.c_str(), &rect_item);
    }
  }

  if (columns[kColumnUserLastUpdated].visible) {
    win::Rect rect_item;
    get_subitem_rect(kColumnUserLastUpdated, rect_item);
    if (rect_item.PtIn(pt)) {
      time_t time_last_updated = ToTime(anime_item->GetMyLastUpdated());
      if (time_last_updated > 0) {
        const std::wstring text = GetAbsoluteTimeString(time_last_updated);
        update_tooltip(kTooltipUserLastUpdated, text.c_str(), &rect_item);
      }
    }
  }

  if (columns[kColumnUserProgress].visible && progress_bars_visible) {
    if (anime_item->GetMyStatus() != anime::MyStatus::Dropped &&
        (anime_item->GetMyStatus() != anime::MyStatus::Completed || anime_item->GetMyRewatching())) {
      if (anime_item->GetMyLastWatchedEpisode() > 0)
        button_visible[0] = true;
      if (anime_item->GetEpisodeCount() > anime_item->GetMyLastWatchedEpisode() ||
          !anime::IsValidEpisodeCount(anime_item->GetEpisodeCount()))
        button_visible[1] = true;
    }

    win::Rect rect_item;
    GetSubItemRect(index, columns[kColumnUserProgress].index, &rect_item);
    rect_item.right -= ScaleX(60);
    rect_item.Inflate(ScaleX(-4), ScaleY(-4));
    button_rect[0].Copy(rect_item);
    button_rect[0].right = button_rect[0].left + rect_item.Height();
    button_rect[1].Copy(rect_item);
    button_rect[1].left = button_rect[1].right - rect_item.Height();
    if (button_visible[0])
      rect_item.left += button_rect[0].Width();
    if (button_visible[1])
      rect_item.right -= button_rect[1].Width();

    if (rect_item.PtIn(pt)) {
      const std::wstring text = GetAvailableEpisodesTooltip(*anime_item);
      update_tooltip(kTooltipEpisodeAvailable, text.c_str(), &rect_item);
    }

    if (button_visible[0] && button_rect[0].PtIn(pt))
      update_tooltip(kTooltipEpisodeMinus, L"-1 episode", &button_rect[0]);
    if (button_visible[1] && button_rect[1].PtIn(pt))
      update_tooltip(kTooltipEpisodePlus, L"+1 episode", &button_rect[1]);
  }

  if (columns[kColumnUserRating].visible) {
    win::Rect rect_item;
    GetSubItemRect(index, columns[kColumnUserRating].index, &rect_item);
    rect_item.Inflate(-4, -2);
    button_rect[2].Copy(rect_item);
    button_visible[2] = true;
  }
}

LRESULT AnimeListDialog::ListView::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Middle mouse button
    case WM_MBUTTONDOWN: {
      int item_index = HitTest();
      if (item_index > -1) {
        SelectAllItems(false);
        SelectItem(item_index);
        const auto command = static_cast<AnimeListAction>(taiga::settings.GetAppListMiddleClickAction());
        int anime_id = parent->GetCurrentId();
        ExecuteCommand(command, anime_id);
      }
      break;
    }

    // Mouse leave
    case WM_MOUSELEAVE: {
      int item_index = GetNextItem(-1, LVIS_SELECTED);
      if (item_index != hot_item)
        RefreshItem(-1);
      break;
    }

    // Set cursor
    case WM_SETCURSOR: {
      POINT pt;
      ::GetCursorPos(&pt);
      ::ScreenToClient(GetWindowHandle(), &pt);
      if ((button_visible[0] && button_rect[0].PtIn(pt)) ||
          (button_visible[1] && button_rect[1].PtIn(pt)) ||
          (button_visible[2] && button_rect[2].PtIn(pt))) {
        SetSharedCursor(IDC_HAND);
        return TRUE;
      }
      break;
    }
  }

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT AnimeListDialog::OnListNotify(LPARAM lParam) {
  LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);

  switch (pnmh->code) {
    // Item drag
    case LVN_BEGINDRAG: {
      auto lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      listview.drag_image.Duplicate(ui::Theme.GetImageList16().GetHandle());
      if (listview.drag_image.GetHandle()) {
        int icon_index = ui::kIcon16_DocumentA;
        int anime_id = listview.GetItemParam(lplv->iItem);
        auto anime_item = anime::db.Find(anime_id);
        if (anime_item)
          icon_index = ui::StatusToIcon(anime_item->GetAiringStatus());
        listview.drag_image.BeginDrag(icon_index, 0, 0);
        listview.drag_image.DragEnter(DlgMain.GetWindowHandle(),
                                      lplv->ptAction.x, lplv->ptAction.y);
        listview.dragging = true;
        SetCapture();
      }
      break;
    }

    // Scroll
    case LVN_BEGINSCROLL:
    case LVN_ENDSCROLL: {
      listview.RefreshItem(-1);
      break;
    }

    // Column click
    case LVN_COLUMNCLICK: {
      auto lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      const bool same_column = lplv->iSubItem == listview.GetSortColumn();
      const auto column_type = listview.FindColumnAtSubItemIndex(lplv->iSubItem);
      int order = listview.GetDefaultSortOrder(column_type);
      if (same_column)
        order = listview.GetSortOrder() * -1;
      listview.Sort(lplv->iSubItem, order, listview.GetSortType(column_type), ui::AnimeListCompareProc);
      RebuildIdCache();
      if (!same_column) {
        taiga::settings.SetAppListSortColumnSecondary(taiga::settings.GetAppListSortColumnPrimary());
        taiga::settings.SetAppListSortOrderSecondary(taiga::settings.GetAppListSortOrderPrimary());
      }
      taiga::settings.SetAppListSortColumnPrimary(listview.columns[column_type].key);
      taiga::settings.SetAppListSortOrderPrimary(order);
      break;
    }

    // Item select
    case LVN_ITEMCHANGED: {
      auto lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      if (lplv->uNewState)
        listview.RefreshItem(lplv->iItem);
      const auto selected_count = listview.GetSelectedCount();
      if (selected_count > 0) {
        ui::ChangeStatusText(L"{} {} selected"_format(
            selected_count, selected_count == 1 ? L"item" : L"items"));
      } else {
        ui::ClearStatusText();
      }
      break;
    }

    // Item hover
    case LVN_HOTTRACK: {
      auto lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      listview.RefreshItem(lplv->iItem);
      break;
    }

    // Double click
    case NM_DBLCLK: {
      if (listview.GetSelectedCount() > 0) {
        bool on_button = false;
        int anime_id = GetCurrentId();
        auto lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
        if (listview.button_visible[0] &&
            listview.button_rect[0].PtIn(lpnmitem->ptAction)) {
          anime::DecrementEpisode(anime_id);
          on_button = true;
        } else if (listview.button_visible[1] &&
                   listview.button_rect[1].PtIn(lpnmitem->ptAction)) {
          anime::IncrementEpisode(anime_id);
          on_button = true;
        }
        if (on_button) {
          int list_index = GetListIndex(GetCurrentId());
          listview.RefreshItem(list_index);
          listview.RedrawItems(list_index, list_index, true);
        } else {
          const auto command = static_cast<AnimeListAction>(taiga::settings.GetAppListDoubleClickAction());
          listview.ExecuteCommand(command, anime_id);
        }
      }
      break;
    }

    // Left click
    case NM_CLICK: {
      if (pnmh->hwndFrom == listview.GetWindowHandle()) {
        if (listview.GetSelectedCount() > 0) {
          int anime_id = GetCurrentId();
          auto anime_ids = GetCurrentIds();
          auto lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
          if (listview.button_visible[0] &&
              listview.button_rect[0].PtIn(lpnmitem->ptAction)) {
            anime::DecrementEpisode(anime_id);
          } else if (listview.button_visible[1] &&
                     listview.button_rect[1].PtIn(lpnmitem->ptAction)) {
            anime::IncrementEpisode(anime_id);
          } else if (listview.button_visible[2] &&
                     listview.button_rect[2].PtIn(lpnmitem->ptAction)) {
            POINT pt = {listview.button_rect[2].left, listview.button_rect[2].bottom};
            ClientToScreen(listview.GetWindowHandle(), &pt);
            ui::Menus.UpdateAnime(GetCurrentItem());
            ExecuteCommand(ui::Menus.Show(GetWindowHandle(), pt.x, pt.y, L"EditScore"), 0,
                           reinterpret_cast<LPARAM>(&anime_ids));
          }
          int list_index = GetListIndex(GetCurrentId());
          listview.RefreshItem(list_index);
          listview.RedrawItems(list_index, list_index, true);
        }
      }
      break;
    }

    // Text callback
    case LVN_GETDISPINFO: {
      NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(lParam);
      auto anime_item = anime::db.Find(static_cast<int>(plvdi->item.lParam));
      if (!anime_item)
        break;
      auto column_type = listview.FindColumnAtSubItemIndex(plvdi->item.iSubItem);
      switch (column_type) {
        case kColumnAnimeTitle:
          if (plvdi->item.mask & LVIF_TEXT) {
            plvdi->item.pszText = const_cast<LPWSTR>(
                anime::GetPreferredTitle(*anime_item).data());
          }
          break;
      }
      break;
    }

    // Key press
    case LVN_KEYDOWN: {
      auto pnkd = reinterpret_cast<LPNMLVKEYDOWN>(lParam);
      int anime_id = GetCurrentId();
      auto anime_ids = GetCurrentIds();
      switch (pnkd->wVKey) {
        // Simulate double-click
        case VK_RETURN: {
          if (!anime::IsValidId(anime_id))
            break;
          const auto command = static_cast<AnimeListAction>(taiga::settings.GetAppListDoubleClickAction());
          listview.ExecuteCommand(command, anime_id);
          break;
        }
        // Delete item
        case VK_DELETE: {
          if (!anime::IsValidId(anime_id))
            break;
          ExecuteCommand(L"EditDelete()", 0, reinterpret_cast<LPARAM>(&anime_ids));
          return TRUE;
        }
        // Various
        default: {
          if (GetKeyState(VK_CONTROL) & 0x8000) {
            // Select all items
            if (pnkd->wVKey == 'A') {
              listview.SelectAllItems();
              return TRUE;
            }

            if (!anime::IsValidId(anime_id))
              break;

            // Edit episode
            if (pnkd->wVKey == VK_ADD || pnkd->wVKey == VK_OEM_PLUS) {
              anime::IncrementEpisode(anime_id);
              return TRUE;
            } else if (pnkd->wVKey == VK_SUBTRACT || pnkd->wVKey == VK_OEM_MINUS) {
              anime::DecrementEpisode(anime_id);
              return TRUE;
            // Edit score
            } else if (pnkd->wVKey >= '0' && pnkd->wVKey <= '9') {
              ExecuteCommand(L"EditScore({})"_format((pnkd->wVKey - '0') * 10), 0,
                            reinterpret_cast<LPARAM>(&anime_ids));
              return TRUE;
            } else if (pnkd->wVKey >= VK_NUMPAD0 && pnkd->wVKey <= VK_NUMPAD9) {
              ExecuteCommand(L"EditScore({})"_format((pnkd->wVKey - VK_NUMPAD0) * 10), 0,
                            reinterpret_cast<LPARAM>(&anime_ids));
              return TRUE;
            // Copy anime titles to clipboard
            } else if (pnkd->wVKey == 'C') {
              const auto anime_ids = GetCurrentIds();
              if (!anime_ids.empty()) {
                std::vector<std::wstring> anime_titles;
                for (const auto id : anime_ids) {
                  const auto anime_item = anime::db.Find(id);
                  if (anime_item)
                    anime_titles.push_back(anime::GetPreferredTitle(*anime_item));
                }
                if (!anime_titles.empty()) {
                  const auto str = Join(anime_titles, L"\r\n");
                  win::Clipboard clipboard(DlgMain.GetWindowHandle());
                  clipboard.Empty();
                  clipboard.SetText(str);
                }
              }
              return TRUE;
            // Open folder
            } else if (pnkd->wVKey == 'O') {
              ExecuteCommand(L"OpenFolder", 0, anime_id);
              return TRUE;
            // Play next episode
            } else if (pnkd->wVKey == 'N') {
              track::PlayNextEpisode(anime_id);
              return TRUE;
            // Play random episode
            } else if (pnkd->wVKey == 'R') {
              track::PlayRandomEpisode(anime_id);
              return TRUE;
            }
          }
          break;
        }
      }
      break;
    }

    // Header drag & drop
    case HDN_ENDDRAG: {
      auto nmh = reinterpret_cast<LPNMHEADER>(lParam);
      if (nmh->pitem->iOrder == -1)
        return TRUE;
      listview.RefreshItem(-1);
      listview.MoveColumn(nmh->iItem, nmh->pitem->iOrder);
      break;
    }
    // Header resize
    case HDN_ITEMCHANGING: {
      auto nmh = reinterpret_cast<LPNMHEADER>(lParam);
      auto column_type = listview.FindColumnAtSubItemIndex(nmh->iItem);
      if (column_type == kColumnAnimeStatus)
        return TRUE;
      listview.RefreshItem(-1);
      break;
    }
    case HDN_ITEMCHANGED: {
      auto nmh = reinterpret_cast<LPNMHEADER>(lParam);
      listview.SetColumnSize(nmh->iItem, nmh->pitem->cxy);
      break;
    }

    // Custom draw
    case NM_CUSTOMDRAW: {
      return OnListCustomDraw(lParam);
    }
  }

  return 0;
}

void AnimeListDialog::ListView::DrawAiringStatus(HDC hdc, RECT* rc,
                                                 anime::Item& anime_item) {
  win::Dc dc = hdc;
  win::Rect rect = *rc;

  const bool playing = anime_item.GetId() == CurrentEpisode.anime_id;
  const int icon_index = playing ? ui::kIcon16_Play :
      StatusToIcon(anime_item.GetAiringStatus());

  int cx = ScaleX(16);
  int cy = ScaleY(16);
  ui::Theme.GetImageList16().GetIconSize(cx, cy);

  int x = (rect.Width() - cx) / 2;
  int y = (rect.Height() - cy) / 2;

  ui::Theme.GetImageList16().Draw(icon_index, dc.Get(),
                                  rect.left + x, rect.top + y);

  dc.DetachDc();
}

void AnimeListDialog::ListView::DrawProgressBar(HDC hdc, RECT* rc, int index,
                                                anime::Item& anime_item) {
  win::Dc dc = hdc;
  win::Rect rcBar = *rc;

  // Draw background
  ui::Theme.DrawListProgress(dc.Get(), &rcBar, ui::kListProgressBackground);

  win::Rect rcAired = rcBar;
  win::Rect rcAvail = rcBar;
  win::Rect rcWatched = rcBar;

  float ratio_aired = 0.0f;
  float ratio_watched = 0.0f;
  anime::GetProgressRatios(anime_item, ratio_aired, ratio_watched);
  rcAired.right = ratio_aired ?
      rcAired.left + std::lround(rcAired.Width() * ratio_aired) :
      rcAired.left;
  rcWatched.right = ratio_watched ?
      rcWatched.left + std::lround(rcWatched.Width() * ratio_watched) :
      rcWatched.left;

  // Draw aired episodes
  if (taiga::settings.GetAppListProgressDisplayAired() && rcAired.Width()) {
    rcAired.top = rcAired.bottom - ScaleY(3);
    ui::Theme.DrawListProgress(dc.Get(), &rcAired, ui::kListProgressAired);
  }

  // Draw progress
  if (rcWatched.Width()) {
    auto list_progress_type = ui::kListProgressWatching;
    if (anime_item.GetMyRewatching()) {
      list_progress_type = ui::kListProgressWatching;
    } else {
      switch (anime_item.GetMyStatus()) {
        default:
        case anime::MyStatus::Watching:
          list_progress_type = ui::kListProgressWatching;
          break;
        case anime::MyStatus::Completed:
          list_progress_type = ui::kListProgressCompleted;
          break;
        case anime::MyStatus::OnHold:
          list_progress_type = ui::kListProgressOnHold;
          break;
        case anime::MyStatus::Dropped:
          list_progress_type = ui::kListProgressDropped;
          break;
        case anime::MyStatus::PlanToWatch:
          list_progress_type = ui::kListProgressPlanToWatch;
          break;
      }
    }
    ui::Theme.DrawListProgress(dc.Get(), &rcWatched, list_progress_type);
  }

  // Draw episode availability
  if (taiga::settings.GetAppListProgressDisplayAvailable()) {
    const int eps_total = anime::EstimateEpisodeCount(anime_item);
    const int eps_aired = anime::GetLastEpisodeNumber(anime_item);
    const int eps_available = std::max(eps_total, eps_aired);
    const int eps_watched = anime_item.GetMyLastWatchedEpisode(true);
    const int bar_width = eps_total ? rcBar.Width() : std::lround(rcBar.Width() * 0.8f);
    const float ep_width = static_cast<float>(bar_width) / static_cast<float>(eps_available);
    rcAvail.top = rcAvail.bottom - ScaleY(3);
    for (int i = 1; i <= eps_available; i++) {
      if (anime_item.IsEpisodeAvailable(i)) {
        rcAvail.left = rcBar.left + static_cast<int>(std::floor(ep_width * (i - 1)));
        if (i == eps_watched) {
          rcAvail.right = rcWatched.right;
        } else {
          rcAvail.right = rcAvail.left + static_cast<int>(std::ceil(ep_width));
        }
        rcAvail.right = std::min(rcAvail.right, rcBar.left + bar_width);
        ui::Theme.DrawListProgress(dc.Get(), &rcAvail, ui::kListProgressAvailable);
      }
    }
    if (!eps_total && ratio_aired == 0.85f && anime_item.IsNextEpisodeAvailable()) {
      rcAvail.left = rcWatched.right;
      rcAvail.right = rcBar.left + static_cast<int>((rcBar.Width()) * 0.825f);
      ui::Theme.DrawListProgress(dc.Get(), &rcAvail, ui::kListProgressAvailable);
    }
  }

  // Draw buttons
  if (index > -1 && index == hot_item) {
    auto draw_button_background = [&dc](win::Rect& rect) {
      ui::Theme.DrawListProgress(dc.Get(), &rect, ui::kListProgressBackground);
      rect.Inflate(-1, -1);
      dc.FillRect(rect, ui::Theme.GetListProgressColor(ui::kListProgressButton));
      rect.Inflate(-1, -1);
    };
    auto get_inflation = [](int value) {
      return -((value - 1) / 2);
    };
    auto draw_line_h = [&dc, &get_inflation](win::Rect rect) {
      rect.Inflate(0, get_inflation(rect.Height()));
      dc.FillRect(rect, ui::Theme.GetListProgressColor(ui::kListProgressBackground));
    };
    auto draw_line_v = [&dc, &get_inflation](win::Rect rect) {
      rect.Inflate(get_inflation(rect.Width()), 0);
      dc.FillRect(rect, ui::Theme.GetListProgressColor(ui::kListProgressBackground));
    };
    // Draw decrement button
    if (button_visible[0]) {
      win::Rect rcButton = button_rect[0];
      draw_button_background(rcButton);
      draw_line_h(rcButton);
    }
    // Draw increment button
    if (button_visible[1]) {
      win::Rect rcButton = button_rect[1];
      draw_button_background(rcButton);
      draw_line_h(rcButton);
      draw_line_v(rcButton);
    }
  }

  // Rewatching
  if (index > -1 && index == hot_item) {
    if (anime_item.GetMyRewatching()) {
      win::Rect rcText = rcBar;
      rcText.Inflate(-button_rect[0].Width(), 4);
      COLORREF text_color = dc.GetTextColor();
      dc.SetBkMode(TRANSPARENT);
      dc.EditFont(nullptr, 7, -1, TRUE);
      dc.SetTextColor(ui::Theme.GetListProgressColor(ui::kListProgressButton));
      dc.DrawText(L"Rewatching", -1, rcText,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
      dc.SetTextColor(text_color);
    }
  }

  // Don't destroy the DC
  dc.DetachDc();
}

void AnimeListDialog::ListView::DrawProgressText(HDC hdc, RECT* rc,
                                                 anime::Item& anime_item) {
  win::Dc dc = hdc;

  int eps_watched = anime_item.GetMyLastWatchedEpisode(true);
  int eps_total = anime_item.GetEpisodeCount();

  // Draw text
  std::wstring text;
  win::Rect rcText = *rc;
  COLORREF text_color = dc.GetTextColor();
  dc.SetBkMode(TRANSPARENT);

  // Separator
  dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
  dc.DrawText(L"/", 1, rcText,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  dc.SetTextColor(text_color);

  // Episodes watched
  text = ui::TranslateNumber(eps_watched, L"0");
  rcText.right -= (rcText.Width() / 2) + ScaleX(4);
  if (!eps_watched) {
    dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
  } else if (!anime::IsValidEpisodeNumber(eps_watched, eps_total)) {
    dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHT));
  } else if (eps_watched < eps_total && anime_item.GetMyStatus() == anime::MyStatus::Completed) {
    dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHT));
  }
  dc.DrawText(text.c_str(), text.length(), rcText,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
  dc.SetTextColor(text_color);

  // Total episodes
  text = ui::TranslateNumber(eps_total, L"?");
  rcText.left = rcText.right + ScaleX(8);
  rcText.right = rc->right;
  if (!eps_total)
    dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
  dc.DrawText(text.c_str(), text.length(), rcText,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  dc.SetTextColor(text_color);

  // Don't destroy the DC
  dc.DetachDc();
}

void AnimeListDialog::ListView::DrawScoreBox(HDC hdc, RECT* rc, int index,
                                             UINT uItemState, anime::Item& anime_item) {
  win::Dc dc = hdc;
  win::Rect rcBox = button_rect[2];

  if (index > -1 && index == hot_item) {
    rcBox.right -= 2;
    ui::Theme.DrawListProgress(dc.Get(), &rcBox, ui::kListProgressBorder);
    rcBox.Inflate(-1, -1);
    ui::Theme.DrawListProgress(dc.Get(), &rcBox, ui::kListProgressBackground);
    rcBox.Inflate(-4, 0);

    COLORREF text_color = dc.GetTextColor();
    dc.SetBkMode(TRANSPARENT);

    std::wstring text = ui::TranslateMyScore(anime_item.GetMyScore());
    dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
    dc.DrawText(text.c_str(), text.length(), rcBox, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    dc.EditFont(nullptr, 5);
    dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
    dc.DrawText(L"\u25BC", 1, rcBox, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    dc.SetTextColor(text_color);
  }

  dc.DetachDc();
}

LRESULT AnimeListDialog::OnListCustomDraw(LPARAM lParam) {
  LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

  switch (pCD->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
      return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
      // Alternate background color
      if ((pCD->nmcd.dwItemSpec % 2) && !listview.IsGroupViewEnabled())
        pCD->clrTextBk = ChangeColorBrightness(GetSysColor(COLOR_WINDOW), -0.03f);
      return CDRF_NOTIFYSUBITEMDRAW;

    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
      auto anime_item = anime::db.Find(static_cast<int>(pCD->nmcd.lItemlParam));
      if (!anime_item)
        break;

      // Change text color
      pCD->clrText = GetSysColor(COLOR_WINDOWTEXT);
      auto column_type = listview.FindColumnAtSubItemIndex(pCD->iSubItem);
      switch (column_type) {
        case kColumnAnimeRating:
          if (!anime_item->GetScore())
            pCD->clrText = GetSysColor(COLOR_GRAYTEXT);
          break;
        case kColumnAnimeTitle:
          if (anime_item->IsNextEpisodeAvailable() &&
              taiga::settings.GetAppListHighlightNewEpisodes())
            pCD->clrText = GetSysColor(COLOR_HIGHLIGHT);
          break;
        case kColumnAnimeType:
          if (anime_item->GetType() == anime::SeriesType::Unknown)
            pCD->clrText = GetSysColor(COLOR_GRAYTEXT);
          break;
        case kColumnUserRating:
          if (!anime_item->GetMyScore())
            pCD->clrText = GetSysColor(COLOR_GRAYTEXT);
          break;
        case kColumnAnimeSeason:
          if (!anime::IsValidDate(anime_item->GetDateStart()))
            pCD->clrText = GetSysColor(COLOR_GRAYTEXT);
          break;
        case kColumnUserLastUpdated:
          if (anime_item->GetMyLastUpdated().empty() ||
              anime_item->GetMyLastUpdated() == L"0")
            pCD->clrText = GetSysColor(COLOR_GRAYTEXT);
          break;
        case kColumnUserDateStarted:
          if (!anime::IsValidDate(anime_item->GetMyDateStart()))
            pCD->clrText = GetSysColor(COLOR_GRAYTEXT);
          break;
        case kColumnUserDateCompleted:
          if (!anime::IsValidDate(anime_item->GetMyDateEnd()))
            pCD->clrText = GetSysColor(COLOR_GRAYTEXT);
          break;
      }

      // Indicate currently playing
      if (anime_item->GetId() == CurrentEpisode.anime_id) {
        pCD->clrTextBk = ui::kColorLightGreen;
        static HFONT hFontDefault = ChangeDCFont(pCD->nmcd.hdc, nullptr, -1, true, -1, -1);
        static HFONT hFontBold = reinterpret_cast<HFONT>(GetCurrentObject(pCD->nmcd.hdc, OBJ_FONT));
        SelectObject(pCD->nmcd.hdc, column_type == kColumnAnimeTitle ? hFontBold : hFontDefault);
        return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
      }

      return CDRF_NOTIFYPOSTPAINT;
    }

    case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM: {
      auto anime_item = anime::db.Find(static_cast<int>(pCD->nmcd.lItemlParam));
      if (!anime_item)
        break;

      auto column_type = listview.FindColumnAtSubItemIndex(pCD->iSubItem);
      if (column_type == kColumnAnimeStatus ||
          column_type == kColumnUserProgress ||
          column_type == kColumnUserRating) {
        win::Rect rcItem;
        listview.GetSubItemRect(pCD->nmcd.dwItemSpec, pCD->iSubItem, &rcItem);
        if (!rcItem.IsEmpty() && listview.columns[column_type].visible) {
          // Draw airing status
          if (column_type == kColumnAnimeStatus) {
            listview.DrawAiringStatus(pCD->nmcd.hdc, &rcItem, *anime_item);

          // Draw progress bar and text
          } else if (column_type == kColumnUserProgress) {
            listview.progress_bars_visible = rcItem.Width() > ScaleX(100);
            rcItem.Inflate(ScaleX(-4), 0);
            if (listview.progress_bars_visible) {
              win::Rect rcBar = rcItem;
              rcBar.right -= ScaleX(60);
              rcBar.Inflate(0, ScaleY(-4));
              listview.DrawProgressBar(pCD->nmcd.hdc, &rcBar, pCD->nmcd.dwItemSpec,
                                       *anime_item);
              rcItem.left = rcBar.right;
            }
            listview.DrawProgressText(pCD->nmcd.hdc, &rcItem, *anime_item);

          // Draw score box
          } else if (column_type == kColumnUserRating) {
            listview.DrawScoreBox(pCD->nmcd.hdc, &rcItem, pCD->nmcd.dwItemSpec,
                                  pCD->nmcd.uItemState, *anime_item);
          }
        }
      }

      break;
    }
  }

  return CDRF_DODEFAULT;
}

////////////////////////////////////////////////////////////////////////////////

/* Tab control */

LRESULT AnimeListDialog::OnTabNotify(LPARAM lParam) {
  switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
    // Tab select
    case TCN_SELCHANGE: {
      int tab_index = tab.GetCurrentlySelected();
      const auto status = static_cast<anime::MyStatus>(tab.GetItemParam(tab_index));
      RefreshList(status);
      break;
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int AnimeListDialog::GetCurrentId() {
  return GetAnimeIdFromSelectedListItem(listview);
}

std::vector<int> AnimeListDialog::GetCurrentIds() {
  return GetAnimeIdsFromSelectedListItems(listview);
}

anime::Item* AnimeListDialog::GetCurrentItem() {
  return anime::db.Find(GetCurrentId());
}

anime::MyStatus AnimeListDialog::GetCurrentStatus() {
  return current_status_;
}

int AnimeListDialog::GetListIndex(int anime_id) {
  auto it = listview.id_cache.find(anime_id);
  return it != listview.id_cache.end() ? it->second : -1;
}

void AnimeListDialog::RebuildIdCache() {
  listview.id_cache.clear();
  for (int i = 0; i < listview.GetItemCount(); i++)
    listview.id_cache[listview.GetItemParam(i)] = i;
}

void AnimeListDialog::RefreshList(std::optional<anime::MyStatus> status) {
  if (!IsWindow())
    return;

  bool group_view = !DlgMain.search_bar.filters.text[kSidebarItemAnimeList].empty();

  // Remember current position
  int current_position = -1;
  if (!status && !group_view)
    current_position = listview.GetTopIndex() + listview.GetCountPerPage() - 1;

  // Remember current status
  if (status && *status != anime::MyStatus::NotInList)
    current_status_ = *status;

  // Disable drawing
  listview.SetRedraw(FALSE);

  // Clear list
  listview.DeleteAllItems();
  listview.RefreshItem(-1);

  // Enable group view
  listview.EnableGroupView(group_view);

  // Add items to list
  std::map<anime::MyStatus, int> group_count;
  int group_index = -1;
  int item_index = 0;
  for (const auto& [anime_id, anime_item] : anime::db.items) {
    if (!anime_item.IsInList())
      continue;
    if (IsDeletedFromList(anime_item))
      continue;
    if (!group_view) {
      if (anime_item.GetMyRewatching()) {
        if (current_status_ != anime::MyStatus::Watching)
          continue;
      } else if (current_status_ != anime_item.GetMyStatus()) {
        continue;
      }
    }
    if (!DlgMain.search_bar.filters.CheckItem(anime_item, kSidebarItemAnimeList))
      continue;

    group_count[anime_item.GetMyStatus()]++;
    group_index = group_view ? static_cast<int>(anime_item.GetMyStatus()) : -1;
    item_index = listview.GetItemCount();

    listview.InsertItem(item_index, group_index, -1,
                        0, nullptr, LPSTR_TEXTCALLBACK,
                        static_cast<LPARAM>(anime_item.GetId()));
    RefreshListItemColumns(item_index, anime_item);
  }

  auto timer = taiga::timers.timer(taiga::kTimerAnimeList);
  if (timer)
    timer->Reset();

  // Set group headers
  if (group_view) {
    for (const auto status : anime::kMyStatuses) {
      const auto text = L"{} ({})"_format(ui::TranslateMyStatus(status, false),
                                          group_count[status]);
      listview.SetGroupText(static_cast<int>(status), text.c_str());
    }
  }

  // Sort items
  listview.SortFromSettings();

  if (current_position > -1) {
    if (current_position > listview.GetItemCount() - 1)
      current_position = listview.GetItemCount() - 1;
    listview.EnsureVisible(current_position);
  }

  // Clear status bar text, as it might be displaying selected item count
  ui::ClearStatusText();

  // Redraw
  listview.SetRedraw(TRUE);
  listview.RedrawWindow(nullptr, nullptr,
                        RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void AnimeListDialog::RefreshListItem(int anime_id) {
  int index = GetListIndex(anime_id);

  if (index > -1 && listview.IsItemVisible(index)) {
    auto anime_item = anime::db.Find(anime_id);
    if (anime_item)
      RefreshListItemColumns(index, *anime_item);
    listview.RedrawItems(index, index, true);
  }
}

void AnimeListDialog::RefreshListItemColumns(int index, const anime::Item& anime_item) {
  for (const auto& it : listview.columns) {
    const auto& column = it.second;
    if (!column.visible)
      continue;
    std::wstring text;
    switch (column.column) {
      case kColumnAnimeRating:
        text = ui::TranslateScore(anime_item.GetScore());
        break;
      case kColumnAnimeSeason:
        text = ui::TranslateDateToSeasonString(anime_item.GetDateStart());
        break;
      case kColumnAnimeStatus:
        listview.RedrawItems(index, index, true);
        break;
      case kColumnAnimeType:
        text = ui::TranslateType(anime_item.GetType());
        break;
      case kColumnUserDateStarted:
        text = ui::TranslateMyDate(anime_item.GetMyDateStart());
        break;
      case kColumnUserDateCompleted:
        text = ui::TranslateMyDate(anime_item.GetMyDateEnd());
        break;
      case kColumnUserLastUpdated: {
        time_t time_last_updated = ToTime(anime_item.GetMyLastUpdated());
        text = GetRelativeTimeString(time_last_updated, true);
        break;
      }
      case kColumnUserNotes:
        text = anime_item.GetMyNotes();
        break;
      case kColumnUserRating:
        text = ui::TranslateMyScore(anime_item.GetMyScore());
        break;
    }
    if (!text.empty())
      listview.SetItem(index, column.index, text.c_str());
  }
}

void AnimeListDialog::RefreshTabs(std::optional<anime::MyStatus> status) {
  if (!IsWindow())
    return;

  // Remember last index
  if (status && *status != anime::MyStatus::NotInList)
    current_status_ = *status;

  // Disable drawing
  tab.SetRedraw(FALSE);

  // Refresh text
  for (const auto status : anime::kMyStatuses) {
    tab.SetItemText(static_cast<int>(status) - 1, ui::TranslateMyStatus(status, true).c_str());
  }

  // Select related tab
  bool group_view = !DlgMain.search_bar.filters.text[kSidebarItemAnimeList].empty();
  int tab_index = static_cast<int>(current_status_);
  if (group_view) {
    tab_index = -1;
  } else {
    tab_index--;
  }
  tab.SetCurrentlySelected(tab_index);

  // Redraw
  tab.SetRedraw(TRUE);
  tab.RedrawWindow(nullptr, nullptr,
                   RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void AnimeListDialog::GoToPreviousTab() {
  int tab_index = tab.GetCurrentlySelected();
  int tab_count = tab.GetItemCount();

  if (tab_index > 0) {
    --tab_index;
  } else {
    tab_index = tab_count - 1;
  }

  tab.SetCurrentlySelected(tab_index);

  const auto status = static_cast<anime::MyStatus>(tab.GetItemParam(tab_index));
  RefreshList(status);
}

void AnimeListDialog::GoToNextTab() {
  int tab_index = tab.GetCurrentlySelected();
  int tab_count = tab.GetItemCount();

  if (tab_index < tab_count - 1) {
    ++tab_index;
  } else {
    tab_index = 0;
  }

  tab.SetCurrentlySelected(tab_index);

  const auto status = static_cast<anime::MyStatus>(tab.GetItemParam(tab_index));
  RefreshList(status);
}

////////////////////////////////////////////////////////////////////////////////

void AnimeListDialog::ListView::InitializeColumns(bool reset) {
  columns.clear();

  int i = 0;

  columns.insert(std::make_pair(kColumnAnimeStatus, ColumnData(
      {kColumnAnimeStatus, true, i, i++,
       0, static_cast<unsigned short>(ScaleX(16) + 6), static_cast<unsigned short>(ScaleX(16) + 6),
       LVCFMT_CENTER, L"", L"anime_status"})));
  columns.insert(std::make_pair(kColumnAnimeTitle, ColumnData(
      {kColumnAnimeTitle, true, i, i++,
       0, static_cast<unsigned short>(ScaleX(300)), static_cast<unsigned short>(ScaleX(100)),
       LVCFMT_LEFT, L"Anime title", L"anime_title"})));
  columns.insert(std::make_pair(kColumnUserProgress, ColumnData(
      {kColumnUserProgress, true, i, i++,
       0, static_cast<unsigned short>(ScaleX(200)), static_cast<unsigned short>(ScaleX(60)),
       LVCFMT_CENTER, L"Progress", L"user_progress"})));
  columns.insert(std::make_pair(kColumnUserRating, ColumnData(
      {kColumnUserRating, true, i, i++,
       0, static_cast<unsigned short>(ScaleX(50)), static_cast<unsigned short>(ScaleX(50)),
       LVCFMT_CENTER, L"Score", L"user_rating"})));
  columns.insert(std::make_pair(kColumnAnimeRating, ColumnData(
      {kColumnAnimeRating, false, i, i++,
       0, static_cast<unsigned short>(ScaleX(55)), static_cast<unsigned short>(ScaleX(55)),
       LVCFMT_CENTER, L"Average", L"anime_average_rating"})));
  columns.insert(std::make_pair(kColumnAnimeType, ColumnData(
      {kColumnAnimeType, true, i, i++,
       0, static_cast<unsigned short>(ScaleX(65)), static_cast<unsigned short>(ScaleX(65)),
       LVCFMT_CENTER, L"Type", L"anime_type"})));
  columns.insert(std::make_pair(kColumnAnimeSeason, ColumnData(
      {kColumnAnimeSeason, true, i, i++,
       0, static_cast<unsigned short>(ScaleX(95)), static_cast<unsigned short>(ScaleX(95)),
       LVCFMT_RIGHT, L"Season", L"anime_season"})));
  columns.insert(std::make_pair(kColumnUserDateStarted, ColumnData(
      {kColumnUserDateStarted, false, i, i++,
       0, static_cast<unsigned short>(ScaleX(90)), static_cast<unsigned short>(ScaleX(90)),
       LVCFMT_RIGHT, L"Started", L"user_date_started"})));
  columns.insert(std::make_pair(kColumnUserDateCompleted, ColumnData(
      {kColumnUserDateCompleted, false, i, i++,
       0, static_cast<unsigned short>(ScaleX(90)), static_cast<unsigned short>(ScaleX(90)),
       LVCFMT_RIGHT, L"Completed", L"user_date_completed"})));
  columns.insert(std::make_pair(kColumnUserLastUpdated, ColumnData(
      {kColumnUserLastUpdated, false, i, i++,
       0, static_cast<unsigned short>(ScaleX(100)), static_cast<unsigned short>(ScaleX(85)),
       LVCFMT_RIGHT, L"Last updated", L"user_last_updated"})));
  columns.insert(std::make_pair(kColumnUserNotes, ColumnData(
      {kColumnUserNotes, false, i, i++,
       0, static_cast<unsigned short>(ScaleX(100)), static_cast<unsigned short>(ScaleX(85)),
       LVCFMT_LEFT, L"Notes", L"user_notes"})));

  if (reset) {
    for (const auto& [column_type, _] : columns) {
      UpdateColumnSetting(column_type);
    }
  } else {
    for (auto& [_, data] : columns) {
      if (const auto& column = taiga::settings.GetAnimeListColumn(data.key)) {
        data.order = column->order;
        data.visible = column->visible;
        data.width = column->width;
      }
    }
  }
}

void AnimeListDialog::ListView::InsertColumns() {
  // Resolve width issues
  for (auto& it : columns) {
    auto& column = it.second;
    if (column.width < column.width_min)
      column.width = column.width_default;
  }

  // Resolve order issues
  for (int order = 0; order < static_cast<int>(columns.size()); ++order) {
    bool found = false;
    for (auto& it : columns) {
      auto& column = it.second;
      if (column.order == order) {
        found = true;
        break;
      }
    }
    if (!found) {
      int minimum_order = INT_MAX;
      for (auto& it : columns) {
        auto& column = it.second;
        if (column.order > order) {
          minimum_order = std::min(minimum_order, column.order);
        }
      }
      int order_difference = minimum_order - order;
      for (auto& it : columns) {
        auto& column = it.second;
        if (column.order > order)
          column.order -= order_difference;
      }
    }
  }
  for (int order = 0; order < static_cast<int>(columns.size()); ++order) {
    std::set<int> found;
    for (auto& it : columns) {
      auto& column = it.second;
      if (column.order == order)
        found.insert(column.column);
    }
    if (found.size() > 1) {
      int maximum_order = 0;
      for (auto& it : columns) {
        auto& column = it.second;
        maximum_order = std::max(maximum_order, column.order);
      }
      int current_order = maximum_order + 1;
      found.erase(found.begin());
      for (const auto& it : found) {
        auto& column = columns[static_cast<AnimeListColumn>(it)];
        column.order = current_order++;
      }
    }
  }

  // Calculate the index of each column
  size_t current_index = 0;
  for (int order = 0; order < static_cast<int>(columns.size()); ++order) {
    for (auto& it : columns) {
      auto& column = it.second;
      if (column.order == order) {
        column.index = column.visible ? current_index++ : -1;
        break;
      }
    }
  }

  DeleteAllColumns();

  // Make sure the anime title column is inserted first, so that quick key
  // access feature works. This also handles the issue where the alignment of
  // the first column has to be LVCFMT_LEFT.
  int title_index = columns[kColumnAnimeTitle].index;
  if (title_index > 0) {
    for (auto& it : columns) {
      auto& column = it.second;
      if (column.index == 0) {
        column.index = title_index;
        break;
      }
    }
    columns[kColumnAnimeTitle].index = 0;
  }

  // Insert columns
  for (int i = 0; i < static_cast<int>(columns.size()); ++i) {
    for (auto& it : columns) {
      auto& column = it.second;
      if (!column.visible || column.index != i)
        continue;
      InsertColumn(column.index, column.width, column.width_min,
                   column.alignment, column.name.c_str());
      break;
    }
  }

  // Reposition the anime title column if needed
  if (title_index > 0) {
    std::vector<int> order_array;
    for (auto& it : columns) {
      auto& column = it.second;
      if (column.visible)
        order_array.push_back(static_cast<int>(order_array.size()));
    }
    std::swap(order_array.at(0), order_array.at(title_index));
    SetColumnOrderArray(static_cast<int>(order_array.size()),
                        order_array.data());
  }
}

void AnimeListDialog::ListView::MoveColumn(int index, int new_visible_order) {
  int order = 0;

  for (const auto& it : columns) {
    auto& column = it.second;
    if (column.index == index) {
      order = column.order;
      break;
    }
  }

  int new_order = new_visible_order;

  for (const auto& it : columns) {
    auto& column = it.second;
    if (column.order <= new_visible_order && !column.visible)
      ++new_order;
  }

  if (new_order == order)
    return;

  for (auto& [column_type, column] : columns) {
    if (column.index == index) {
      column.order = new_order;
    } else if (column.order >= std::min(order, new_order) &&
               column.order <= std::max(order, new_order)) {
      column.order += new_order > order ? -1 : 1;
    }
    UpdateColumnSetting(column_type);
  }
}

AnimeListColumn AnimeListDialog::ListView::FindColumnAtSubItemIndex(int index) {
  for (const auto& [_, column] : columns) {
    if (column.index == index)
      return column.column;
  }
  return kColumnUnknown;
}

void AnimeListDialog::ListView::RefreshColumns(bool reset) {
  if (reset)
    InitializeColumns(reset);
  InsertColumns();
  parent->RefreshList();
}

void AnimeListDialog::ListView::RefreshLastUpdateColumn() {
  const auto& column = columns[kColumnUserLastUpdated];

  if (!column.visible)
    return;

  int item_count = GetItemCount();
  time_t time_now = time(nullptr);

  for (int i = 0; i < item_count; ++i) {
    auto anime_item = anime::db.Find(GetItemParam(i));
    if (!anime_item)
      continue;
    time_t time_last_updated = ToTime(anime_item->GetMyLastUpdated());
    if (Duration(time_now - time_last_updated).hours() < 24) {
      std::wstring text = GetRelativeTimeString(time_last_updated, true);
      SetItem(i, column.index, text.c_str());
    }
  }
}

void AnimeListDialog::ListView::SetColumnSize(int index, unsigned short width) {
  for (auto& [column_type, column] : columns) {
    if (column.index == index) {
      column.width = width;
      UpdateColumnSetting(column_type);
      break;
    }
  }
}

void AnimeListDialog::ListView::UpdateColumnSetting(
    const AnimeListColumn column) const {
  if (const auto it = columns.find(column); it != columns.end()) {
    taiga::Settings::AnimeListColumn data;
    data.order = it->second.order;
    data.visible = it->second.visible;
    data.width = it->second.width;
    taiga::settings.SetAnimeListColumn(it->second.key, data);
  }
}

AnimeListColumn AnimeListDialog::ListView::TranslateColumnName(const std::wstring& name) {
  static const std::map<std::wstring, AnimeListColumn> names{
    {L"anime_average_rating", kColumnAnimeRating},
    {L"anime_season", kColumnAnimeSeason},
    {L"anime_status", kColumnAnimeStatus},
    {L"anime_title", kColumnAnimeTitle},
    {L"anime_type", kColumnAnimeType},
    {L"user_date_started", kColumnUserDateStarted},
    {L"user_date_completed", kColumnUserDateCompleted},
    {L"user_last_updated", kColumnUserLastUpdated},
    {L"user_notes", kColumnUserNotes},
    {L"user_progress", kColumnUserProgress},
    {L"user_rating", kColumnUserRating},
  };

  const auto it = names.find(name);
  return it != names.end() ? it->second : kColumnUnknown;
}

}  // namespace ui
