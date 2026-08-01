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

#include "base/format.h"
#include "base/gfx.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "sync/service.h"
#include "sync/sync.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "track/recognition.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_search.h"
#include "ui/dialog.h"
#include "ui/list.h"
#include "ui/command.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace ui {

SearchDialog DlgSearch;

BOOL SearchDialog::OnInitDialog() {
  // Create list control
  list_.Attach(GetDlgItem(IDC_LIST_SEARCH));
  list_.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  list_.SetImageList(ui::Theme.GetImageList16().GetHandle());
  list_.SetTheme();
  list_.InsertColumn(0, ScaleX(400), ScaleX(400), LVCFMT_LEFT,   L"Anime title");
  list_.InsertColumn(1,  ScaleX(60),  ScaleX(60), LVCFMT_CENTER, L"Type");
  list_.InsertColumn(2,  ScaleX(60),  ScaleX(60), LVCFMT_RIGHT,  L"Episodes");
  list_.InsertColumn(3,  ScaleX(60),  ScaleX(60), LVCFMT_RIGHT,  L"Score");
  list_.InsertColumn(4, ScaleX(100), ScaleX(100), LVCFMT_RIGHT,  L"Season");
  list_.EnableGroupView(true);
  list_.InsertGroup(0, L"Not in list");
  list_.InsertGroup(1, L"In list");

  // Success
  return TRUE;
}

INT_PTR SearchDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL: {
      return list_.SendMessage(uMsg, wParam, lParam);
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void SearchDialog::OnContextMenu(HWND hwnd, POINT pt) {
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
      ui::Menus.UpdateSearchList(!is_in_list);
      const auto command = ui::Menus.Show(DlgMain.GetWindowHandle(), pt.x, pt.y, L"SearchList");
      ExecuteCommand(command, TRUE, StartsWith(command, L"AddToList") ?
                     reinterpret_cast<LPARAM>(&anime_ids) : anime_id);
    }
  }
}

LRESULT SearchDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // ListView control
  if (idCtrl == IDC_LIST_SEARCH) {
    switch (pnmh->code) {
      // Column click
      case LVN_COLUMNCLICK: {
        auto lplv = reinterpret_cast<LPNMLISTVIEW>(pnmh);
        int order = 1;
        switch (lplv->iSubItem) {
          case 1:  // Type
          case 2:  // Episodes
          case 3:  // Score
          case 4:  // Season
            order = -1;
            break;
        }
        if (lplv->iSubItem == list_.GetSortColumn())
          order = list_.GetSortOrder() * -1;
        switch (lplv->iSubItem) {
          // Type
          case 1:
            list_.Sort(lplv->iSubItem, order, ui::kListSortDefault, ui::ListViewCompareProc);
            break;
          // Episodes
          case 2:
            list_.Sort(lplv->iSubItem, order, ui::kListSortNumber, ui::ListViewCompareProc);
            break;
          // Score
          case 3:
            list_.Sort(lplv->iSubItem, order, ui::kListSortScore, ui::ListViewCompareProc);
            break;
          // Season
          case 4:
            list_.Sort(lplv->iSubItem, order, ui::kListSortDateStart, ui::ListViewCompareProc);
            break;
          // Other columns
          default:
            list_.Sort(lplv->iSubItem, order, ui::kListSortDefault, ui::ListViewCompareProc);
            break;
        }
        break;
      }

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

      // Double click
      case NM_DBLCLK: {
        LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
        if (lpnmitem->iItem > -1) {
          int anime_id = list_.GetItemParam(lpnmitem->iItem);
          if (anime::IsValidId(anime_id))
            ShowDlgAnimeInfo(anime_id);
        }
        break;
      }
    }
  }

  return 0;
}

void SearchDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      win::Rect rcWindow;
      rcWindow.Set(0, 0, size.cx, size.cy);
      // Resize list
      list_.SetPosition(nullptr, rcWindow);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void SearchDialog::ParseResults(const std::vector<int>& ids) {
  if (!IsWindow())
    return;

  if (ids.empty()) {
    std::wstring msg = L"No results found for \"{}\"."_format(search_text);
    win::TaskDialog dlg(L"Search Anime", TD_INFORMATION_ICON);
    dlg.SetMainInstruction(msg.c_str());
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(GetWindowHandle());
    return;
  }

  anime_ids_ = ids;
  RefreshList();
}

void SearchDialog::AddAnimeToList(int anime_id) {
  auto anime_item = anime::db.Find(anime_id);

  if (anime_item) {
    int i = list_.GetItemCount();
    list_.InsertItem(i, anime_item->IsInList() ? 1 : 0,
                     StatusToIcon(anime_item->GetAiringStatus()), 0, nullptr,
                     anime::GetPreferredTitle(*anime_item).c_str(),
                     static_cast<LPARAM>(anime_item->GetId()));
    list_.SetItem(i, 1, ui::TranslateType(anime_item->GetType()).c_str());
    list_.SetItem(i, 2, ui::TranslateNumber(anime_item->GetEpisodeCount()).c_str());
    list_.SetItem(i, 3, ui::TranslateScore(anime_item->GetScore()).c_str());
    list_.SetItem(i, 4, ui::TranslateDateToSeasonString(anime_item->GetDateStart()).c_str());
  }
}

void SearchDialog::RefreshList() {
  if (!IsWindow())
    return;

  // Disable drawing
  list_.SetRedraw(FALSE);

  // Clear list
  list_.DeleteAllItems();

  // Add anime items to list
  for (const auto& anime_id : anime_ids_)
    AddAnimeToList(anime_id);

  // Redraw
  list_.SetRedraw(TRUE);
  list_.RedrawWindow(nullptr, nullptr,
                     RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void SearchDialog::Search(const std::wstring& title, bool local) {
  anime_ids_.clear();
  search_text = title;

  if (local) {
    Meow.Search(title, anime_ids_);
    RefreshList();
  } else {
    sync::SearchTitle(title);
  }
}

}  // namespace ui
