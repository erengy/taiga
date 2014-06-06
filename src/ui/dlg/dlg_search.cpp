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
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "sync/sync.h"
#include "taiga/resource.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "ui/dlg/dlg_search.h"
#include "ui/dialog.h"
#include "ui/list.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"
#include "win/win_taskdialog.h"

namespace ui {

SearchDialog DlgSearch;

BOOL SearchDialog::OnInitDialog() {
  // Create list control
  list_.Attach(GetDlgItem(IDC_LIST_SEARCH));
  list_.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  list_.SetImageList(ui::Theme.GetImageList16().GetHandle());
  list_.SetTheme();
  list_.InsertColumn(0, 400, 400, LVCFMT_LEFT,   L"Anime title");
  list_.InsertColumn(1,  60,  60, LVCFMT_CENTER, L"Type");
  list_.InsertColumn(2,  60,  60, LVCFMT_RIGHT,  L"Episodes");
  list_.InsertColumn(3,  60,  60, LVCFMT_RIGHT,  L"Score");
  list_.InsertColumn(4, 100, 100, LVCFMT_RIGHT,  L"Season");
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

LRESULT SearchDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // ListView control
  if (idCtrl == IDC_LIST_SEARCH) {
    switch (pnmh->code) {
      // Column click
      case LVN_COLUMNCLICK: {
        LPNMLISTVIEW lplv = (LPNMLISTVIEW)pnmh;
        int order = 1;
        if (lplv->iSubItem == list_.GetSortColumn())
          order = list_.GetSortOrder() * -1;
        switch (lplv->iSubItem) {
          // Episode
          case 2:
            list_.Sort(lplv->iSubItem, order, ui::kListSortNumber, ui::ListViewCompareProc);
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

      // Double click
      case NM_DBLCLK: {
        if (list_.GetSelectedCount() > 0) {
          LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
          if (lpnmitem->iItem == -1)
            break;
          LPARAM lParam = list_.GetItemParam(lpnmitem->iItem);
          if (lParam)
            ShowDlgAnimeInfo(lParam);
        }
        break;
      }

      // Right click
      case NM_RCLICK: {
        LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
        if (lpnmitem->iItem == -1)
          break;
        LPARAM lParam = list_.GetItemParam(lpnmitem->iItem);
        auto anime_item = AnimeDatabase.FindItem(static_cast<int>(lParam));
		ui::Menus.UpdateSearchList(!anime_item->IsInList(), anime_item);
        ExecuteAction(ui::Menus.Show(pnmh->hwndFrom, 0, 0, L"SearchList"), 0, lParam);
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
    std::wstring msg = L"No results found for \"" + search_text + L"\".";
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
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (anime_item) {
    int i = list_.GetItemCount();
    list_.InsertItem(i, anime_item->IsInList() ? 1 : 0,
                     StatusToIcon(anime_item->GetAiringStatus()), 0, nullptr,
                     anime_item->GetTitle().c_str(),
                     static_cast<LPARAM>(anime_item->GetId()));
    list_.SetItem(i, 1, anime::TranslateType(anime_item->GetType()).c_str());
    list_.SetItem(i, 2, anime::TranslateNumber(anime_item->GetEpisodeCount()).c_str());
    list_.SetItem(i, 3, anime_item->GetScore().c_str());
    list_.SetItem(i, 4, anime::TranslateDateToSeasonString(anime_item->GetDateStart()).c_str());
  }
}

void SearchDialog::RefreshList() {
  if (!IsWindow())
    return;

  // Hide and clear the list
  list_.Hide();
  list_.DeleteAllItems();

  // Add anime items to list
  foreach_(it, anime_ids_)
    AddAnimeToList(*it);

  // Show the list again
  list_.Show(SW_SHOW);
}

void SearchDialog::Search(const std::wstring& title) {
  anime_ids_.clear();
  search_text = title;

  ui::ChangeStatusText(L"Searching " + taiga::GetCurrentService()->name() +
                       L" for \"" + title + L"\"...");
  sync::SearchTitle(title, anime::ID_UNKNOWN);
}

}  // namespace ui