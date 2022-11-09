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

#include "base/format.h"
#include "base/gfx.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_season_db.h"
#include "media/anime_util.h"
#include "ui/resource.h"
#include "sync/service.h"
#include "sync/sync.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_season.h"
#include "ui/command.h"
#include "ui/dialog.h"
#include "ui/list.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace ui {

enum SeasonToolbarCommand {
  kCommandSelectSeason = 100,
  kCommandPreviousSeason,
  kCommandNextSeason,
  kCommandRefreshSeason,
  kCommandSeasonGroupBy,
  kCommandSeasonSortBy,
  kCommandSeasonView,
};

constexpr int kNotInListGroupIndex = 1000;

SeasonDialog DlgSeason;

SeasonDialog::SeasonDialog()
    : hot_item_(-1) {
}

BOOL SeasonDialog::OnInitDialog() {
  // Create list
  list_.Attach(GetDlgItem(IDC_LIST_SEASON));
  list_.EnableGroupView(true);
  list_.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT);
  list_.SetTheme();
  list_.SetView(LV_VIEW_TILE);
  SetViewMode(taiga::settings.GetAppSeasonsViewAs());

  // Create list tooltips
  tooltips_.Create(list_.GetWindowHandle());
  tooltips_.SetDelayTime(30000, -1, 0);
  tooltips_.SetMaxWidth(::GetSystemMetrics(SM_CXSCREEN));  // Required for line breaks
  tooltips_.AddTip(0, nullptr, nullptr, nullptr, false);

  // Create main toolbar
  toolbar_.Attach(GetDlgItem(IDC_TOOLBAR_SEASON));
  toolbar_.SetImageList(ui::Theme.GetImageList16().GetHandle(), ScaleX(16), ScaleY(16));
  toolbar_.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);

  // Insert toolbar buttons
  BYTE fsState = TBSTATE_ENABLED;
  BYTE fsStyle0 = BTNS_AUTOSIZE;
  BYTE fsStyle1 = BTNS_AUTOSIZE | BTNS_SHOWTEXT;
  BYTE fsStyle2 = BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_WHOLEDROPDOWN;
  toolbar_.InsertButton(0, ui::kIcon16_CalendarPrev, kCommandPreviousSeason, fsState, fsStyle0, 0, nullptr, L"Previous season");
  toolbar_.InsertButton(1, ui::kIcon16_CalendarNext, kCommandNextSeason, fsState, fsStyle0, 1, nullptr, L"Next season");
  toolbar_.InsertButton(2, ui::kIcon16_Calendar, kCommandSelectSeason, fsState, fsStyle2, 2, L"Select season", L"Select season");
  toolbar_.InsertButton(3, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_.InsertButton(4, ui::kIcon16_Refresh, kCommandRefreshSeason, fsState, fsStyle1, 4, L"Refresh data", L"Download anime details and missing images");
  toolbar_.InsertButton(5, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_.InsertButton(6, ui::kIcon16_Category, kCommandSeasonGroupBy, fsState, fsStyle2, 6, L"Group by", nullptr);
  toolbar_.InsertButton(7, ui::kIcon16_Sort, kCommandSeasonSortBy, fsState, fsStyle2, 7, L"Sort by", nullptr);
  toolbar_.InsertButton(8, ui::kIcon16_Details, kCommandSeasonView, fsState, fsStyle2, 8, L"View", nullptr);

  // Create rebar
  rebar_.Attach(GetDlgItem(IDC_REBAR_SEASON));
  // Insert rebar bands
  UINT fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
  UINT fStyle = RBBS_NOGRIPPER;
  rebar_.InsertBand(nullptr, 0, 0, 0, 0, 0, 0, 0, 0, fMask, fStyle);
  rebar_.InsertBand(toolbar_.GetWindowHandle(), GetSystemMetrics(SM_CXSCREEN), 0, 0, 0, 0, 0, 0,
                    HIWORD(toolbar_.GetButtonSize()) + (HIWORD(toolbar_.GetPadding()) / 2), fMask, fStyle);

  // Load the last selected season
  const auto last_season = taiga::settings.GetAppSeasonsLastSeason();
  if (!last_season.empty()) {
    anime::season_db.Set(anime::Season(WstrToStr(last_season)));
  }

  // Refresh
  RefreshList();
  RefreshStatus();
  RefreshToolbar();

  return TRUE;
}

////////////////////////////////////////////////////////////////////////////////

INT_PTR SeasonDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL: {
      return list_.SendMessage(uMsg, wParam, lParam);
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL SeasonDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  // Toolbar
  switch (LOWORD(wParam)) {
    // Previous/Next season
    case kCommandPreviousSeason:
    case kCommandNextSeason: {
      auto season = anime::season_db.current_season;
      const auto season_str = ui::TranslateSeason(
          LOWORD(wParam) == kCommandPreviousSeason ? --season : ++season);
      ui::ExecuteCommand(L"Season_Load(" + season_str + L")");
      return TRUE;
    }

    // Refresh data
    case kCommandRefreshSeason:
      sync::GetSeason(anime::season_db.current_season);
      return TRUE;
  }

  return FALSE;
}

void SeasonDialog::OnContextMenu(HWND hwnd, POINT pt) {
  if (hwnd == list_.GetWindowHandle()) {
    if (list_.GetSelectedCount() > 0) {
      if (pt.x == -1 || pt.y == -1)
        GetPopupMenuPositionForSelectedListItem(list_, pt);
      auto anime_id = GetAnimeIdFromSelectedListItem(list_);
      auto anime_ids = GetAnimeIdsFromSelectedListItems(list_);
      bool is_in_list = true;
      for (const auto& anime_id : anime_ids) {
        auto anime_item = anime::db.Find(anime_id);
        if (anime_item && !anime_item->IsInList()) {
          is_in_list = false;
          break;
        }
      }
      bool trailer_available = false;
      for (const auto& anime_id : anime_ids) {
        auto anime_item = anime::db.Find(anime_id);
        if (anime_item && !anime_item->GetTrailerId().empty()) {
          trailer_available = true;
          break;
        }
      }
      ui::Menus.UpdateSeasonList(is_in_list, trailer_available);
      const auto command = ui::Menus.Show(DlgMain.GetWindowHandle(), pt.x, pt.y, L"SeasonList");
      bool multi_id = StartsWith(command, L"AddToList") ||
                      StartsWith(command, L"Season_RefreshItemData") ||
                      StartsWith(command, L"ViewAnimePage") ||
                      StartsWith(command, L"WatchTrailer");
      ExecuteCommand(command, TRUE, multi_id ? reinterpret_cast<LPARAM>(&anime_ids) : anime_id);
      list_.RedrawWindow();
    }
  }
}

BOOL SeasonDialog::OnDestroy() {
  return TRUE;
}

LRESULT SeasonDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // List
  if (idCtrl == IDC_LIST_SEASON) {
    return OnListNotify(reinterpret_cast<LPARAM>(pnmh));
  // Toolbar
  } else if (idCtrl == IDC_TOOLBAR_SEASON) {
    return OnToolbarNotify(reinterpret_cast<LPARAM>(pnmh));
  }

  return 0;
}

void SeasonDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      win::Rect rcWindow;
      rcWindow.Set(0, 0, size.cx, size.cy);
      // Resize rebar
      rebar_.SendMessage(WM_SIZE, 0, 0);
      rcWindow.top += rebar_.GetBarHeight() + ScaleY(kControlMargin / 2);
      // Resize list
      list_.SetPosition(nullptr, rcWindow);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

LRESULT SeasonDialog::ListView::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                           LPARAM lParam) {
  switch (uMsg) {
    // Middle mouse button
    case WM_MBUTTONDOWN: {
      const int item_index = HitTest();
      if (item_index > -1) {
        SelectAllItems(false);
        SelectItem(item_index);
        const int anime_id =
            static_cast<int>(GetParamFromSelectedListItem(*this));
        ExecuteCommand(L"ViewAnimePage", false, anime_id);
      }
      break;
    }
  }

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT SeasonDialog::OnListNotify(LPARAM lParam) {
  LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
  switch (pnmh->code) {
    // Key press
    case LVN_KEYDOWN: {
      auto pnkd = reinterpret_cast<LPNMLVKEYDOWN>(pnmh);
      switch (pnkd->wVKey) {
        case VK_RETURN: {
          int anime_id = GetAnimeIdFromSelectedListItem(list_);
          if (anime::IsValidId(anime_id)) {
            ShowDlgAnimeInfo(anime_id);
            return TRUE;
          }
          break;
        }
      }
      break;
    }

    // Item hover
    case LVN_HOTTRACK: {
      auto lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      if (taiga::settings.GetAppSeasonsViewAs() != kSeasonViewAsImages) {
        tooltips_.NewToolRect(0, nullptr);
        break;
      }
      if (hot_item_ != lplv->iItem) {
        hot_item_ = lplv->iItem;
        win::Rect rect_item;
        list_.GetSubItemRect(lplv->iItem, 0, &rect_item);
        tooltips_.NewToolRect(0, &rect_item);
      }
      auto anime_item = anime::db.Find(list_.GetItemParam(lplv->iItem));
      if (anime_item) {
        tooltips_.UpdateTitle(anime::GetPreferredTitle(*anime_item).c_str());
        std::wstring text;
        const std::wstring separator = L" \u2022 ";
        text += ui::TranslateType(anime_item->GetType()) + separator +
                ui::TranslateNumber(anime_item->GetEpisodeCount(), L"?") + L" eps." + separator +
                ui::TranslateScore(anime_item->GetScore()) + separator;
        switch (sync::GetCurrentServiceId()) {
          default:
            text += L"#" + ToWstr(anime_item->GetPopularity());
            break;
          case sync::ServiceId::AniList:
            text += ToWstr(anime_item->GetPopularity()) + L" users";
            break;
        }
        if (!anime_item->GetGenres().empty())
          text += L"\n" + Join(anime_item->GetGenres(), L", ");
        const auto producers = anime::GetStudiosAndProducers(*anime_item);
        if (!producers.empty())
          text += L"\n" + Join(producers, L", ");
        tooltips_.UpdateText(0, text.c_str());
      }
      break;
    }

    // Custom draw
    case NM_CUSTOMDRAW: {
      return OnListCustomDraw(lParam);
    }

    // Double click
    case NM_DBLCLK: {
      LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
      if (lpnmitem->iItem == -1)
        break;
      LPARAM param = list_.GetItemParam(lpnmitem->iItem);
      if (param)
        ShowDlgAnimeInfo(param);
      break;
    }
  }

  return 0;
}

LRESULT SeasonDialog::OnListCustomDraw(LPARAM lParam) {
  LRESULT result = CDRF_DODEFAULT;
  LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

  win::Dc hdc = pCD->nmcd.hdc;
  win::Rect rect = pCD->nmcd.rc;

  switch (pCD->nmcd.dwDrawStage) {
    case CDDS_PREPAINT: {
      // LVN_GETEMPTYMARKUP notification is sent only once, so we paint our own
      // markup text when the control has no items.
      if (list_.GetItemCount() == 0) {
        list_.GetClientRect(&rect);  // nmcd.rc is invalid while drawing a selection box
        std::wstring text;
        if (!anime::season_db.current_season) {
          text = L"Please select a season.";
        } else if (anime::season_db.items.empty()) {
          text = L"No anime found for {} season."_format(
              ui::TranslateSeason(anime::season_db.current_season));
        } else {
          text = L"No anime found for \"{}\"."_format(
              DlgMain.search_bar.filters.text[kSidebarItemSeasons]);
        }
        hdc.EditFont(L"Segoe UI", 9, -1, TRUE);
        hdc.SetBkMode(TRANSPARENT);
        hdc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
        hdc.DrawText(text.c_str(), text.length(), rect,
                     DT_CENTER | DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
      }
      result = CDRF_NOTIFYITEMDRAW;
      break;
    }

    case CDDS_ITEMPREPAINT: {
      result = CDRF_NOTIFYPOSTPAINT;
      break;
    }

    case CDDS_ITEMPOSTPAINT: {
      auto anime_item = anime::db.Find(static_cast<int>(pCD->nmcd.lItemlParam));
      if (!anime_item)
        break;

      const auto current_service = sync::GetCurrentServiceId();

      // Draw border
      rect.Inflate(-4, -4);
      hdc.FillRect(rect, ui::kColorGray);

      // Draw background
      rect.Inflate(-1, -1);
      hdc.FillRect(rect, ui::kColorLightGray);

      // Calculate text height
      int text_height = GetTextHeight(hdc.Get());

      // Calculate line count
      const int line_count = GetLineCount();

      // Calculate areas
      win::Rect rect_image(
          rect.left + 4, rect.top + 4,
          rect.left + ScaleX(124), rect.bottom - 4);
      win::Rect rect_title(
          rect_image.right + 4, rect_image.top,
          rect.right - 4, rect_image.top + text_height + 8);
      win::Rect rect_details(
          rect_title.left + 4, rect_title.bottom + 4,
          rect_title.right, rect_title.bottom + 4 + (line_count * (text_height + 2)));
      win::Rect rect_synopsis(
          rect_details.left, rect_details.bottom + 4,
          rect_details.right, rect_image.bottom);

      // Draw image
      if (const auto image = ui::image_db.GetImage(anime_item->GetId())) {
        rect_image = ResizeRect(rect_image,
                                image->rect.Width(),
                                image->rect.Height(),
                                true, true, false);
        hdc.SetStretchBltMode(HALFTONE);
        hdc.StretchBlt(rect_image.left, rect_image.top,
                       rect_image.Width(), rect_image.Height(),
                       image->dc.Get(), 0, 0,
                       image->rect.Width(),
                       image->rect.Height(),
                       SRCCOPY);
      }

      // Draw title background
      COLORREF color;
      switch (anime_item->GetAiringStatus()) {
        case anime::SeriesStatus::Airing:
          color = ui::kColorLightGreen;
          break;
        case anime::SeriesStatus::FinishedAiring:
        default:
          color = ui::kColorLightBlue;
          break;
        case anime::SeriesStatus::NotYetAired:
          color = ui::kColorLightRed;
          break;
      }

      auto view_as = taiga::settings.GetAppSeasonsViewAs();

      if (view_as == kSeasonViewAsImages) {
        rect_title.Copy(rect);
        rect_title.top = rect_title.bottom - (text_height + 8);
      }
      hdc.FillRect(rect_title, color);

      // Draw anime list indicator
      if (anime_item->IsInList()) {
        ui::Theme.GetImageList16().Draw(
            ui::kIcon16_DocumentA, hdc.Get(),
            rect_title.right - ScaleX(16) - 4,
            rect_title.top + ((rect_title.Height() - ScaleY(16)) / 2));
        rect_title.right -= ScaleX(16) + 4;
      }

      // Set title
      std::wstring text = anime::GetPreferredTitle(*anime_item);
      if (view_as == kSeasonViewAsImages) {
        switch (taiga::settings.GetAppSeasonsSortBy()) {
          case kSeasonSortByAiringDate:
            text = ui::TranslateDate(anime_item->GetDateStart());
            break;
          case kSeasonSortByEpisodes:
            text = ui::TranslateNumber(anime_item->GetEpisodeCount(), L"");
            if (text.empty()) {
              text = L"Unknown";
            } else {
              text += text == L"1" ? L" episode" : L" episodes";
            }
            break;
          case kSeasonSortByPopularity:
            switch (current_service) {
              default:
                text = L"#" + ToWstr(anime_item->GetPopularity());
                break;
              case sync::ServiceId::AniList:
                text = ToWstr(anime_item->GetPopularity()) + L" users";
                break;
            }
            break;
          case kSeasonSortByScore:
            text = ui::TranslateScore(anime_item->GetScore());
            break;
        }
      }

      // Draw title
      rect_title.Inflate(-4, 0);
      hdc.EditFont(nullptr, -1, TRUE);
      hdc.SetBkMode(TRANSPARENT);
      UINT nFormat = DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER;
      if (view_as == kSeasonViewAsImages)
        nFormat |= DT_CENTER;
      hdc.DrawText(text.c_str(), text.length(), rect_title, nFormat);

      // Draw details
      if (view_as == kSeasonViewAsImages)
        break;
      int text_top = rect_details.top;
      #define DRAWLINE(t) \
        text = t; \
        hdc.DrawText(text.c_str(), text.length(), rect_details, \
                     DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE); \
        rect_details.Offset(0, text_height + 2);

      DRAWLINE(L"Aired:");
      DRAWLINE(L"Episodes:");
      switch (current_service) {
        case sync::ServiceId::MyAnimeList:
        case sync::ServiceId::AniList:
          DRAWLINE(L"Genres:");
          break;
        case sync::ServiceId::Kitsu:
          DRAWLINE(L"Categories:");
          break;
      }
      switch (current_service) {
        case sync::ServiceId::MyAnimeList:
        case sync::ServiceId::AniList:
          DRAWLINE(L"Producers:");
          break;
      }
      DRAWLINE(L"Score:");
      DRAWLINE(L"Popularity:");

      rect_details.Set(rect_details.left + ScaleX(75), text_top,
                       rect_details.right, rect_details.top + text_height);
      DeleteObject(hdc.DetachFont());

      text = ui::TranslateDate(anime_item->GetDateStart());
      text += anime_item->GetDateEnd() != anime_item->GetDateStart() ?
              L" to " + ui::TranslateDate(anime_item->GetDateEnd()) : L"";
      text += L" (" + ui::TranslateStatus(anime_item->GetAiringStatus()) + L")";
      DRAWLINE(text);
      DRAWLINE(ui::TranslateNumber(anime_item->GetEpisodeCount(), L"Unknown"));
      DRAWLINE(anime_item->GetGenres().empty() ? L"?" : Join(anime_item->GetGenres(), L", "));
      switch (current_service) {
        case sync::ServiceId::MyAnimeList:
        case sync::ServiceId::AniList: {
          const auto producers = anime::GetStudiosAndProducers(*anime_item);
          DRAWLINE(producers.empty() ? L"?" : Join(producers, L", "));
          break;
        }
      }
      DRAWLINE(ui::TranslateScore(anime_item->GetScore()));
      switch (current_service) {
        default:
          DRAWLINE(L"#" + ToWstr(anime_item->GetPopularity()));
          break;
        case sync::ServiceId::AniList:
          DRAWLINE(ToWstr(anime_item->GetPopularity()) + L" users");
          break;
      }

      #undef DRAWLINE

      // Draw synopsis
      if (!anime_item->GetSynopsis().empty()) {
        text = anime_item->GetSynopsis();
        // DT_WORDBREAK doesn't go well with DT_*_ELLIPSIS, so we need to make
        // sure our text ends with ellipses by clipping that extra pixel.
        rect_synopsis.bottom -= (rect_synopsis.Height() % text_height) + 1;
        hdc.DrawText(text.c_str(), text.length(), rect_synopsis,
                     DT_END_ELLIPSIS | DT_NOPREFIX | DT_WORDBREAK);
      }

      break;
    }
  }

  hdc.DetachDc();
  return result;
}

LRESULT SeasonDialog::OnToolbarNotify(LPARAM lParam) {
  switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
    // Dropdown button click
    case TBN_DROPDOWN: {
      RECT rect;
      LPNMTOOLBAR nmt = reinterpret_cast<LPNMTOOLBAR>(lParam);
      ::SendMessage(nmt->hdr.hwndFrom, TB_GETRECT,
                    static_cast<WPARAM>(nmt->iItem),
                    reinterpret_cast<LPARAM>(&rect));
      MapWindowPoints(nmt->hdr.hwndFrom, HWND_DESKTOP, reinterpret_cast<LPPOINT>(&rect), 2);
      ui::Menus.UpdateSeason();
      std::wstring command;
      switch (LOWORD(nmt->iItem)) {
        // Select season
        case kCommandSelectSeason:
          command = ui::Menus.Show(GetWindowHandle(), rect.left, rect.bottom, L"SeasonSelect");
          break;
        // Group by
        case kCommandSeasonGroupBy:
          command = ui::Menus.Show(GetWindowHandle(), rect.left, rect.bottom, L"SeasonGroup");
          break;
        // Sort by
        case kCommandSeasonSortBy:
          command = ui::Menus.Show(GetWindowHandle(), rect.left, rect.bottom, L"SeasonSort");
          break;
        // View as
        case kCommandSeasonView:
          command = ui::Menus.Show(GetWindowHandle(), rect.left, rect.bottom, L"SeasonView");
          break;
      }
      if (!command.empty()) {
        ExecuteCommand(command);
      }
      break;
    }

    // Show tooltips
    case TBN_GETINFOTIP: {
      NMTBGETINFOTIP* git = reinterpret_cast<NMTBGETINFOTIP*>(lParam);
      git->cchTextMax = INFOTIPSIZE;
      if (git->hdr.hwndFrom == toolbar_.GetWindowHandle()) {
        git->pszText = const_cast<LPWSTR>(toolbar_.GetButtonTooltip(git->lParam));
      }
      break;
    }
  }

  return 0L;
}

////////////////////////////////////////////////////////////////////////////////

void SeasonDialog::EnableInput(bool enable) {
  toolbar_.Enable(enable);
  list_.Enable(enable);
}

void SeasonDialog::RefreshData(const std::vector<int>& anime_ids) {
  for (const auto& id : anime_ids) {
    sync::GetMetadataById(id);

    if (const auto anime_item = anime::db.Find(id)) {
      if (!anime_item->GetImageUrl().empty()) {
        sync::DownloadImage(id, anime_item->GetImageUrl());
      }
    }
  }
}

void SeasonDialog::RefreshList(bool redraw_only) {
  if (!IsWindow())
    return;

  if (redraw_only) {
    list_.RedrawWindow();
    return;
  }

  // Disable drawing
  list_.SetRedraw(FALSE);

  struct ListGroup {
    int item_count = 0;
    std::wstring text;
  };
  std::map<int, ListGroup> groups;

  // Insert list groups
  list_.RemoveAllGroups();
  list_.EnableGroupView(true);  // Required for XP
  switch (taiga::settings.GetAppSeasonsGroupBy()) {
    case kSeasonGroupByAiringStatus:
      for (const auto status : anime::kSeriesStatuses) {
        ListGroup group{0, ui::TranslateStatus(status)};
        groups[static_cast<int>(status)] = group;
        list_.InsertGroup(static_cast<int>(status), group.text.c_str(), true, false);
      }
      break;
    case kSeasonGroupByListStatus: {
      for (const auto status : anime::kMyStatuses) {
        ListGroup group{0, ui::TranslateMyStatus(status, false)};
        groups[static_cast<int>(status)] = group;
        list_.InsertGroup(static_cast<int>(status), group.text.c_str(), true, false);
      }
      ListGroup group{0, ui::TranslateMyStatus(anime::MyStatus::NotInList, false)};
      groups[kNotInListGroupIndex] = group;
      list_.InsertGroup(kNotInListGroupIndex, group.text.c_str(), true, false);
      break;
    }
    case kSeasonGroupByType:
    default:
      for (const auto type : anime::kSeriesTypes) {
        ListGroup group{0, ui::TranslateType(type)};
        groups[static_cast<int>(type)] = group;
        list_.InsertGroup(static_cast<int>(type), group.text.c_str(), true, false);
      }
      break;
  }

  // Filter
  std::vector<std::wstring> filters;
  Split(DlgMain.search_bar.filters.text[kSidebarItemSeasons], L" ", filters);
  RemoveEmptyStrings(filters);

  // Add items
  list_.DeleteAllItems();
  for (auto i = anime::season_db.items.begin(); i != anime::season_db.items.end(); ++i) {
    auto anime_item = anime::db.Find(*i);
    if (!anime_item)
      continue;
    bool passed_filters = true;
    std::wstring genres = Join(anime_item->GetGenres(), L", ");
    std::wstring tags = Join(anime_item->GetTags(), L", ");
    std::wstring producers = Join(anime::GetStudiosAndProducers(*anime_item), L", ");
    std::wstring titles = [i] {
      std::vector<std::wstring> titles;
      anime::GetAllTitles(*i, titles);
      return Join(titles, L", ");
    }();
    for (auto j = filters.begin(); passed_filters && j != filters.end(); ++j) {
      if (InStr(genres, *j, 0, true) == -1 &&
          InStr(tags, *j, 0, true) == -1 &&
          InStr(producers, *j, 0, true) == -1 &&
          InStr(titles, *j, 0, true) == -1) {
        passed_filters = false;
        break;
      }
    }
    if (!passed_filters)
      continue;
    int group = -1;
    switch (taiga::settings.GetAppSeasonsGroupBy()) {
      case kSeasonGroupByAiringStatus:
        group = static_cast<int>(anime_item->GetAiringStatus());
        break;
      case kSeasonGroupByListStatus: {
        const auto my_status = anime_item->GetMyStatus();
        group = my_status == anime::MyStatus::NotInList
                    ? kNotInListGroupIndex
                    : static_cast<int>(my_status);
        break;
      }
      case kSeasonGroupByType:
      default:
        group = static_cast<int>(anime_item->GetType());
        break;
    }
    groups[group].item_count += 1;
    list_.InsertItem(i - anime::season_db.items.begin(),
                     group, -1, 0, nullptr, LPSTR_TEXTCALLBACK,
                     static_cast<LPARAM>(anime_item->GetId()));
  }

  // Sort items
  switch (taiga::settings.GetAppSeasonsSortBy()) {
    case kSeasonSortByAiringDate:
      list_.Sort(0, 1, ui::kListSortDateStart, ui::ListViewCompareProc);
      break;
    case kSeasonSortByEpisodes:
      list_.Sort(0, -1, ui::kListSortEpisodeCount, ui::ListViewCompareProc);
      break;
    case kSeasonSortByPopularity:
      switch (sync::GetCurrentServiceId()) {
        default:
          list_.Sort(0, 1, ui::kListSortPopularity, ui::ListViewCompareProc);
          break;
        case sync::ServiceId::AniList:
          list_.Sort(0, -1, ui::kListSortPopularity, ui::ListViewCompareProc);
          break;
      }
      break;
    case kSeasonSortByScore:
      list_.Sort(0, -1, ui::kListSortScore, ui::ListViewCompareProc);
      break;
    case kSeasonSortByTitle:
    default:
      list_.Sort(0, 1, ui::kListSortTitle, ui::ListViewCompareProc);
      break;
  }

  // Update group headers
  for (const auto& [index, group] : groups) {
    list_.SetGroupText(index, L"{} ({})"_format(group.text, group.item_count).c_str());
  }

  // Redraw
  list_.SetRedraw(TRUE);
  list_.RedrawWindow(nullptr, nullptr,
                     RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void SeasonDialog::RefreshStatus() {
  if (!anime::season_db.current_season)
    return;

  std::wstring text = ui::TranslateSeason(anime::season_db.current_season) + L", from " +
                      ui::TranslateSeasonToMonths(anime::season_db.current_season);

  ui::ChangeStatusText(text);
}

void SeasonDialog::RefreshToolbar() {
  toolbar_.SetButtonText(2, anime::season_db.current_season ?
      ui::TranslateSeason(anime::season_db.current_season).c_str() :
      L"Select season");

  toolbar_.EnableButton(kCommandPreviousSeason, anime::season_db.current_season);
  toolbar_.EnableButton(kCommandNextSeason, anime::season_db.current_season);
  toolbar_.EnableButton(kCommandRefreshSeason, anime::season_db.current_season);

  std::wstring text = L"Group by: ";
  switch (taiga::settings.GetAppSeasonsGroupBy()) {
    case kSeasonGroupByAiringStatus:
      text += L"Airing status";
      break;
    case kSeasonGroupByListStatus:
      text += L"List status";
      break;
    case kSeasonGroupByType:
    default:
      text += L"Type";
      break;
  }
  toolbar_.SetButtonText(6, text.c_str());

  text = L"Sort by: ";
  switch (taiga::settings.GetAppSeasonsSortBy()) {
    case kSeasonSortByAiringDate:
      text += L"Airing date";
      break;
    case kSeasonSortByEpisodes:
      text += L"Episodes";
      break;
    case kSeasonSortByPopularity:
      text += L"Popularity";
      break;
    case kSeasonSortByScore:
      text += L"Score";
      break;
    case kSeasonSortByTitle:
    default:
      text += L"Title";
      break;
  }
  toolbar_.SetButtonText(7, text.c_str());

  text = L"View: ";
  switch (taiga::settings.GetAppSeasonsViewAs()) {
    case kSeasonViewAsImages:
      text += L"Images";
      break;
    case kSeasonViewAsTiles:
    default:
      text += L"Details";
      break;
  }
  toolbar_.SetButtonText(8, text.c_str());
}

void SeasonDialog::SetViewMode(int mode) {
  const int line_count = GetLineCount();
  const int synopsis_line_count = 9 - line_count;

  win::Dc hdc = list_.GetDC();
  hdc.AttachFont(list_.GetFont());
  const int text_height = GetTextHeight(hdc.Get());
  hdc.DetachFont();
  hdc.DetachDc();

  SIZE size;
  size.cy =
      4 +  // margin
      1 +  // border
      4 +  // padding
      4 + text_height + 4 +  // title
      4 + (line_count * (text_height + 2)) +  // details
      4 + (synopsis_line_count * text_height) +  // synopsis
      4 +  // ?
      4 +  // padding
      1 +  // border
      4;   // margin
  size.cx = mode == kSeasonViewAsImages ?
      (4 + 1 + 4 + ScaleX(124) + 4 + 1 + 4) :
      static_cast<int>(size.cy * 2.5);
  list_.SetTileViewInfo(0, LVTVIF_FIXEDSIZE, nullptr, &size);

  taiga::settings.SetAppSeasonsViewAs(mode);
}

int SeasonDialog::GetLineCount() const {
  switch (sync::GetCurrentServiceId()) {
    default:
    case sync::ServiceId::MyAnimeList:
    case sync::ServiceId::AniList:
      return 6;
    case sync::ServiceId::Kitsu:
      return 5;  // missing producers
  }
}

}  // namespace ui
