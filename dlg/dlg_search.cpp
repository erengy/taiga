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

#include "dlg_search.h"
#include "dlg_main.h"

#include "../anime_db.h"
#include "../common.h"
#include "../gfx.h"
#include "../http.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"
#include "../xml.h"

#include "../win32/win_taskdialog.h"

class SearchDialog SearchDialog;

// =============================================================================

BOOL SearchDialog::OnInitDialog() {
  // Create list control
  list_.Attach(GetDlgItem(IDC_LIST_SEARCH));
  list_.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  list_.SetImageList(UI.ImgList16.GetHandle());
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
        if (lplv->iSubItem == list_.GetSortColumn()) order = list_.GetSortOrder() * -1;
        switch (lplv->iSubItem) {
          // Episode
          case 2:
            list_.Sort(lplv->iSubItem, order, LIST_SORTTYPE_NUMBER, ListViewCompareProc);
            break;
          // Season
          case 4:
            list_.Sort(lplv->iSubItem, order, LIST_SORTTYPE_STARTDATE, ListViewCompareProc);
            break;
          // Other columns
          default:
            list_.Sort(lplv->iSubItem, order, LIST_SORTTYPE_DEFAULT, ListViewCompareProc);
            break;
        }
        break;
      }

      // Double click
      case NM_DBLCLK: {
        if (list_.GetSelectedCount() > 0) {
          LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
          if (lpnmitem->iItem == -1) break;
          LPARAM lParam = list_.GetItemParam(lpnmitem->iItem);
          if (lParam) ExecuteAction(L"Info", 0, lParam);
        }
        break;
      }

      // Right click
      case NM_RCLICK: {
        LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
        if (lpnmitem->iItem == -1) break;
        LPARAM lParam = list_.GetItemParam(lpnmitem->iItem);
        auto anime_item = AnimeDatabase.FindItem(static_cast<int>(lParam));
        UpdateSearchListMenu(!anime_item->IsInList());
        ExecuteAction(UI.Menus.Show(pnmh->hwndFrom, 0, 0, L"SearchList"), 0, lParam);
        break;
      }
    }
  }

  return 0;
}

void SearchDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      win32::Rect rcWindow;
      rcWindow.Set(0, 0, size.cx, size.cy);
      // Resize list
      list_.SetPosition(nullptr, rcWindow);
    }
  }
}

// =============================================================================

void SearchDialog::ParseResults(const wstring& data) {
  if (data.empty()) {
    wstring msg = L"No results found for \"" + search_text + L"\".";
    win32::TaskDialog dlg(L"Search Anime", TD_INFORMATION_ICON);
    dlg.SetMainInstruction(msg.c_str());
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(GetWindowHandle());
    return;
  }

  if (data == L"Invalid credentials") {
    win32::TaskDialog dlg(L"Search Anime", TD_ERROR_ICON);
    dlg.SetMainInstruction(L"Invalid username or password.");
    dlg.SetContent(L"Anime search requires authentication, which means, you need to "
                   L"enter a valid username and password to search MyAnimeList.");
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(GetWindowHandle());
    return;
  }

  anime_ids_.clear();
  xml_document doc;
  xml_parse_result result = doc.load(data.c_str());
  if (result.status == status_ok) {
    xml_node anime = doc.child(L"anime");
    for (xml_node entry = anime.child(L"entry"); entry; entry = entry.next_sibling(L"entry")) {
      anime_ids_.push_back(XML_ReadIntValue(entry, L"id"));
    }
  }

  mal::ParseSearchResult(data);

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
    list_.SetItem(i, 1, mal::TranslateType(anime_item->GetType()).c_str());
    list_.SetItem(i, 2, mal::TranslateNumber(anime_item->GetEpisodeCount()).c_str());
    list_.SetItem(i, 3, anime_item->GetScore().c_str());
    list_.SetItem(i, 4, mal::TranslateDateToSeason(anime_item->GetDate(anime::DATE_START)).c_str());
  }
}

void SearchDialog::RefreshList() {
  if (!IsWindow()) return;

  // Hide and clear the list
  list_.Hide();
  list_.DeleteAllItems();
  
  // Add anime items to list
  for (size_t i = 0; i < anime_ids_.size(); i++) {
    AddAnimeToList(anime_ids_.at(i));
  }
  /*for (auto it = AnimeDatabase.items.begin(); it != AnimeDatabase.items.end(); ++it) {
    if (std::find(anime_ids_.begin(), anime_ids_.end(), it->second.GetId()) == anime_ids_.end())
      if (filters_.CheckItem(it->second))
        AddAnimeToList(it->second.GetId());
  }*/

  // Sort and show the list again
  //list_.Sort(0, 1, LIST_SORTTYPE_DEFAULT, ListViewCompareProc);
  list_.Show(SW_SHOW);
}

bool SearchDialog::Search(const wstring& title) {
  anime_ids_.clear();
  search_text = title;
  filters_.text = title;
  //RefreshList();
  
  if (mal::SearchAnime(anime::ID_UNKNOWN, title)) {
    MainDialog.ChangeStatus(L"Searching MyAnimeList for \"" + title + L"\"...");
    return true;
  } else {
    MainDialog.ChangeStatus(L"An error occured while searching MyAnimeList for \"" + title + L"\"...");
    return false;
  }
}