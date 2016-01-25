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

#include "base/foreach.h"
#include "base/gfx.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/discover.h"
#include "library/resource.h"
#include "sync/sync.h"
#include "taiga/resource.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_season.h"
#include "ui/dialog.h"
#include "ui/list.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace ui {

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
  SetViewMode(Settings.GetInt(taiga::kApp_Seasons_ViewAs));

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
  BYTE fsStyle1 = BTNS_AUTOSIZE | BTNS_SHOWTEXT;
  BYTE fsStyle2 = BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_WHOLEDROPDOWN;
  toolbar_.InsertButton(0, ui::kIcon16_Calendar, 100, fsState, fsStyle2, 0, L"Select season", L"Select season");
  toolbar_.InsertButton(1, ui::kIcon16_Refresh,  101, fsState, fsStyle1, 1, L"Refresh data", L"Download anime details and missing images");
  toolbar_.InsertButton(2, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  toolbar_.InsertButton(3, ui::kIcon16_Category, 103, fsState, fsStyle2, 3, L"Group by", nullptr);
  toolbar_.InsertButton(4, ui::kIcon16_Sort,     104, fsState, fsStyle2, 4, L"Sort by", nullptr);
  toolbar_.InsertButton(5, ui::kIcon16_Details,  105, fsState, fsStyle2, 5, L"View", nullptr);

  // Create rebar
  rebar_.Attach(GetDlgItem(IDC_REBAR_SEASON));
  // Insert rebar bands
  UINT fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
  UINT fStyle = RBBS_NOGRIPPER;
  rebar_.InsertBand(nullptr, 0, 0, 0, 0, 0, 0, 0, 0, fMask, fStyle);
  rebar_.InsertBand(toolbar_.GetWindowHandle(), GetSystemMetrics(SM_CXSCREEN), 0, 0, 0, 0, 0, 0,
                    HIWORD(toolbar_.GetButtonSize()) + (HIWORD(toolbar_.GetPadding()) / 2), fMask, fStyle);

  // Load the last selected season
  const auto last_season = Settings[taiga::kApp_Seasons_LastSeason];
  if (!last_season.empty()) {
    if (SeasonDatabase.LoadSeason(last_season)) {
      SeasonDatabase.Review();
    } else {
      Settings.Set(taiga::kApp_Seasons_LastSeason, std::wstring());
    }
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
    // Refresh data
    case 101:
      RefreshData();
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
        auto anime_item = AnimeDatabase.FindItem(anime_id);
        if (anime_item && !anime_item->IsInList()) {
          is_in_list = false;
          break;
        }
      }
      ui::Menus.UpdateSeasonList(!is_in_list);
      auto action = ui::Menus.Show(DlgMain.GetWindowHandle(), pt.x, pt.y, L"SeasonList");
      bool multi_id = StartsWith(action, L"AddToList") ||
                      StartsWith(action, L"Season_RefreshItemData");
      ExecuteAction(action, TRUE, multi_id ? reinterpret_cast<LPARAM>(&anime_ids) : anime_id);
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
      rcWindow.top += rebar_.GetBarHeight() + ScaleY(win::kControlMargin / 2);
      // Resize list
      list_.SetPosition(nullptr, rcWindow);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

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
      if (Settings.GetInt(taiga::kApp_Seasons_ViewAs) != kSeasonViewAsImages) {
        tooltips_.NewToolRect(0, nullptr);
        break;
      }
      if (hot_item_ != lplv->iItem) {
        hot_item_ = lplv->iItem;
        win::Rect rect_item;
        list_.GetSubItemRect(lplv->iItem, 0, &rect_item);
        tooltips_.NewToolRect(0, &rect_item);
      }
      auto anime_item = AnimeDatabase.FindItem(list_.GetItemParam(lplv->iItem));
      if (anime_item) {
        tooltips_.UpdateTitle(anime_item->GetTitle().c_str());
        std::wstring text;
        const std::wstring separator = L" \u2022 ";
        text += anime::TranslateType(anime_item->GetType()) + separator +
                anime::TranslateNumber(anime_item->GetEpisodeCount(), L"?") + L" eps." + separator +
                anime::TranslateScore(anime_item->GetScore());
        if (taiga::GetCurrentServiceId() == sync::kMyAnimeList)
          text += separator + L"#" + ToWstr(anime_item->GetPopularity());
        if (!anime_item->GetGenres().empty())
          text += L"\n" + Join(anime_item->GetGenres(), L", ");
        if (!anime_item->GetProducers().empty())
          text += L"\n" + Join(anime_item->GetProducers(), L", ");
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

  if (win::GetVersion() < win::kVersionVista)
    list_.GetSubItemRect(pCD->nmcd.dwItemSpec, pCD->iSubItem, &rect);

  switch (pCD->nmcd.dwDrawStage) {
    case CDDS_PREPAINT: {
      // LVN_GETEMPTYMARKUP notification is sent only once, so we paint our own
      // markup text when the control has no items.
      if (list_.GetItemCount() == 0) {
        std::wstring text;
        if (SeasonDatabase.items.empty()) {
          text = L"No season selected. Please choose one from above.";
        } else {
          text = L"No matching items for \"" + DlgMain.search_bar.filters.text + L"\".";
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
      auto anime_item = AnimeDatabase.FindItem(static_cast<int>(pCD->nmcd.lItemlParam));
      if (!anime_item)
        break;

      // Draw border
      if (win::GetVersion() > win::kVersionXp) {
        rect.Inflate(-4, -4);
      }
      if (win::GetVersion() < win::kVersionVista &&
          pCD->nmcd.uItemState & CDIS_SELECTED) {
        hdc.FillRect(rect, GetSysColor(COLOR_HIGHLIGHT));
      } else {
        hdc.FillRect(rect, ui::kColorGray);
      }

      // Draw background
      rect.Inflate(-1, -1);
      hdc.FillRect(rect, ui::kColorLightGray);

      // Calculate text height
      int text_height = GetTextHeight(hdc.Get());

      // Calculate line count
      int line_count = 6;
      auto current_service = taiga::GetCurrentServiceId();
      switch (current_service) {
        case sync::kMyAnimeList:
          line_count = 6;
          break;
        case sync::kHummingbird:
          line_count = 5;
          break;
      }

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
      if (ImageDatabase.Load(anime_item->GetId(), false, false)) {
        auto image = ImageDatabase.GetImage(anime_item->GetId());
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
        case anime::kAiring:
          color = ui::kColorLightGreen;
          break;
        case anime::kFinishedAiring:
        default:
          color = ui::kColorLightBlue;
          break;
        case anime::kNotYetAired:
          color = ui::kColorLightRed;
          break;
      }

      auto view_as = Settings.GetInt(taiga::kApp_Seasons_ViewAs);
      
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
      std::wstring text = anime_item->GetTitle();
      if (view_as == kSeasonViewAsImages) {
        switch (Settings.GetInt(taiga::kApp_Seasons_SortBy)) {
          case kSeasonSortByAiringDate:
            text = anime::TranslateDate(anime_item->GetDateStart());
            break;
          case kSeasonSortByEpisodes:
            text = anime::TranslateNumber(anime_item->GetEpisodeCount(), L"");
            if (text.empty()) {
              text = L"Unknown";
            } else {
              text += text == L"1" ? L" episode" : L" episodes";
            }
            break;
          case kSeasonSortByPopularity:
            text = L"#" + ToWstr(anime_item->GetPopularity());
            break;
          case kSeasonSortByScore:
            text = anime::TranslateScore(anime_item->GetScore());
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
      DRAWLINE(L"Genres:");
      DRAWLINE(L"Producers:");
      DRAWLINE(L"Score:");
      if (current_service == sync::kMyAnimeList) {
        DRAWLINE(L"Popularity:");
      }

      rect_details.Set(rect_details.left + ScaleX(75), text_top,
                       rect_details.right, rect_details.top + text_height);
      DeleteObject(hdc.DetachFont());

      text = anime::TranslateDate(anime_item->GetDateStart());
      text += anime_item->GetDateEnd() != anime_item->GetDateStart() ?
              L" to " + anime::TranslateDate(anime_item->GetDateEnd()) : L"";
      text += L" (" + anime::TranslateStatus(anime_item->GetAiringStatus()) + L")";
      DRAWLINE(text);
      DRAWLINE(anime::TranslateNumber(anime_item->GetEpisodeCount(), L"Unknown"));
      DRAWLINE(anime_item->GetGenres().empty() ? L"?" : Join(anime_item->GetGenres(), L", "));
      DRAWLINE(anime_item->GetProducers().empty() ? L"?" : Join(anime_item->GetProducers(), L", "));
      DRAWLINE(anime::TranslateScore(anime_item->GetScore()));
      if (current_service == sync::kMyAnimeList) {
        DRAWLINE(L"#" + ToWstr(anime_item->GetPopularity()));
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
      std::wstring action;
      switch (LOWORD(nmt->iItem)) {
        // Select season
        case 100:
          action = ui::Menus.Show(GetWindowHandle(), rect.left, rect.bottom, L"SeasonSelect");
          break;
        // Group by
        case 103:
          action = ui::Menus.Show(GetWindowHandle(), rect.left, rect.bottom, L"SeasonGroup");
          break;
        // Sort by
        case 104:
          action = ui::Menus.Show(GetWindowHandle(), rect.left, rect.bottom, L"SeasonSort");
          break;
        // View as
        case 105:
          action = ui::Menus.Show(GetWindowHandle(), rect.left, rect.bottom, L"SeasonView");
          break;
      }
      if (!action.empty()) {
        ExecuteAction(action);
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

void SeasonDialog::RefreshData(int anime_id) {
  ui::SetSharedCursor(IDC_WAIT);

  foreach_(id, SeasonDatabase.items) {
    if (anime_id > 0 && anime_id != *id)
      continue;

    auto anime_item = AnimeDatabase.FindItem(*id);
    if (!anime_item)
      continue;

    // Download missing image
    if (anime_id > 0) {
      sync::DownloadImage(anime_id, anime_item->GetImageUrl());
    } else {
      ImageDatabase.Load(*id, true, true);
    }

    // Get details
    if (anime_id > 0 || anime::MetadataNeedsRefresh(*anime_item))
      sync::GetMetadataById(*id);
  }

  ui::SetSharedCursor(IDC_ARROW);
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

  // Insert list groups
  list_.RemoveAllGroups();
  list_.EnableGroupView(true);  // Required for XP
  switch (Settings.GetInt(taiga::kApp_Seasons_GroupBy)) {
    case kSeasonGroupByAiringStatus:
      for (int i = anime::kFinishedAiring; i <= anime::kNotYetAired; i++) {
        list_.InsertGroup(i, anime::TranslateStatus(i).c_str(), true, false);
      }
      break;
    case kSeasonGroupByListStatus:
      for (int i = anime::kNotInList; i <= anime::kPlanToWatch; i++) {
        list_.InsertGroup(i, anime::TranslateMyStatus(i, false).c_str(), true, false);
      }
      break;
    case kSeasonGroupByType:
    default:
      for (int i = anime::kTv; i <= anime::kMusic; i++) {
        list_.InsertGroup(i, anime::TranslateType(i).c_str(), true, false);
      }
      break;
  }

  // Filter
  std::vector<std::wstring> filters;
  Split(DlgMain.search_bar.filters.text, L" ", filters);
  RemoveEmptyStrings(filters);

  // Add items
  list_.DeleteAllItems();
  for (auto i = SeasonDatabase.items.begin(); i != SeasonDatabase.items.end(); ++i) {
    auto anime_item = AnimeDatabase.FindItem(*i);
    if (!anime_item)
      continue;
    bool passed_filters = true;
    std::wstring genres = Join(anime_item->GetGenres(), L", ");
    std::wstring producers = Join(anime_item->GetProducers(), L", ");
    for (auto j = filters.begin(); passed_filters && j != filters.end(); ++j) {
      if (InStr(genres, *j, 0, true) == -1 &&
          InStr(producers, *j, 0, true) == -1 &&
          InStr(anime_item->GetTitle(), *j, 0, true) == -1) {
        passed_filters = false;
        break;
      }
    }
    if (!passed_filters)
      continue;
    int group = -1;
    switch (Settings.GetInt(taiga::kApp_Seasons_GroupBy)) {
      case kSeasonGroupByAiringStatus:
        group = anime_item->GetAiringStatus();
        break;
      case kSeasonGroupByListStatus: {
        group = anime_item->GetMyStatus();
        break;
      }
      case kSeasonGroupByType:
      default:
        group = anime_item->GetType();
        break;
    }
    list_.InsertItem(i - SeasonDatabase.items.begin(),
                     group, -1, 0, nullptr, LPSTR_TEXTCALLBACK,
                     static_cast<LPARAM>(anime_item->GetId()));
  }

  // Sort items
  switch (Settings.GetInt(taiga::kApp_Seasons_SortBy)) {
    case kSeasonSortByAiringDate:
      list_.Sort(0, 1, ui::kListSortDateStart, ui::ListViewCompareProc);
      break;
    case kSeasonSortByEpisodes:
      list_.Sort(0, -1, ui::kListSortEpisodeCount, ui::ListViewCompareProc);
      break;
    case kSeasonSortByPopularity:
      list_.Sort(0, 1, ui::kListSortPopularity, ui::ListViewCompareProc);
      break;
    case kSeasonSortByScore:
      list_.Sort(0, -1, ui::kListSortScore, ui::ListViewCompareProc);
      break;
    case kSeasonSortByTitle:
    default:
      list_.Sort(0, 1, ui::kListSortTitle, ui::ListViewCompareProc);
      break;
  }

  // Redraw
  list_.SetRedraw(TRUE);
  list_.RedrawWindow(nullptr, nullptr,
                     RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void SeasonDialog::RefreshStatus() {
  if (SeasonDatabase.items.empty())
    return;

  std::wstring text = SeasonDatabase.current_season.GetString() + L", from " +
                      anime::TranslateSeasonToMonths(SeasonDatabase.current_season);

  ui::ChangeStatusText(text);
}

void SeasonDialog::RefreshToolbar() {
  toolbar_.SetButtonText(0, SeasonDatabase.current_season ?
      SeasonDatabase.current_season.GetString().c_str() :
      L"Select season");

  toolbar_.EnableButton(101, !SeasonDatabase.items.empty());

  std::wstring text = L"Group by: ";
  switch (Settings.GetInt(taiga::kApp_Seasons_GroupBy)) {
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
  toolbar_.SetButtonText(3, text.c_str());

  text = L"Sort by: ";
  switch (Settings.GetInt(taiga::kApp_Seasons_SortBy)) {
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
  toolbar_.SetButtonText(4, text.c_str());

  text = L"View: ";
  switch (Settings.GetInt(taiga::kApp_Seasons_ViewAs)) {
    case kSeasonViewAsImages:
      text += L"Images";
      break;
    case kSeasonViewAsTiles:
    default:
      text += L"Details";
      break;
  }
  toolbar_.SetButtonText(5, text.c_str());
}

void SeasonDialog::SetViewMode(int mode) {
  int line_count = 6;
  auto current_service = taiga::GetCurrentServiceId();
  switch (current_service) {
    case sync::kMyAnimeList:
      line_count = 6;
      break;
    case sync::kHummingbird:
      line_count = 5;
      break;
  }
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

  Settings.Set(taiga::kApp_Seasons_ViewAs, mode);
}

}  // namespace ui
