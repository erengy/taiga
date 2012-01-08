/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include <algorithm>
#include "../animelist.h"
#include "../common.h"
#include "dlg_event.h"
#include "dlg_main.h"
#include "../event.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../taiga.h"

class EventDialog EventDialog;

// =============================================================================

EventDialog::EventDialog() {
  RegisterDlgClass(L"TaigaEventW");
}

BOOL EventDialog::OnInitDialog() {
  // Create list
  list_.Attach(GetDlgItem(IDC_LIST_EVENT));
  list_.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  list_.SetTheme();
  
  // Insert list columns
  list_.InsertColumn(0, 190, 190, LVCFMT_LEFT, L"Anime title");
  list_.InsertColumn(1, 325, 325, LVCFMT_LEFT, L"Details");
  list_.InsertColumn(2, 120, 120, LVCFMT_LEFT, L"Last modified");
  list_.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);
  
  // Refresh list
  RefreshList();
  return TRUE;
}

BOOL EventDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Clear all items
    case IDC_BUTTON_EVENT_CLEAR: {
      if (RemoveItems()) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

LRESULT EventDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  if (pnmh->hwndFrom == list_.GetWindowHandle()) {
    switch (pnmh->code) {
      // List item select
      case LVN_ITEMCHANGED: {
        SetDlgItemText(IDC_BUTTON_EVENT_CLEAR, 
          list_.GetSelectedCount() > 0 ? L"Remove selected items" : L"Remove all items");
        break;
      }
      // Custom draw
      case NM_CUSTOMDRAW:
        LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmh);
        switch (pCD->nmcd.dwDrawStage) {
          case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;
          case CDDS_ITEMPREPAINT:
            return CDRF_NOTIFYSUBITEMDRAW;
          case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
            // Alternate background color
            if (pCD->nmcd.dwItemSpec % 2) pCD->clrTextBk = RGB(248, 248, 248);
            return CDRF_DODEFAULT;
        }
    }
  }
  
  return 0;
}

void EventDialog::OnOK() {
  if (!Taiga.logged_in) {
    ExecuteAction(L"Login");
  } else {
    EventQueue.Check();
  }
}

BOOL EventDialog::PreTranslateMessage(MSG* pMsg) {
  switch (pMsg->message) {
    case WM_KEYDOWN: {
      if (::GetFocus() == list_.GetWindowHandle()) {
        switch (pMsg->wParam) {
          // Delete selected items
          case VK_DELETE: {
            if (RemoveItems()) {
              return TRUE;
            }
          }
          // Move selected items
          case VK_UP:
          case VK_DOWN: {
            if (::GetKeyState(VK_CONTROL) & 0xFF80) {
              if (MoveItems(pMsg->wParam == VK_UP ? -1 : 1)) {
                return TRUE;
              }
            }
            break;
          }
        }
      }
      break;
    }
  }

  return FALSE;
}

// =============================================================================

void EventDialog::RefreshList() {
  if (!IsWindow()) return;
  
  // Clear list
  list_.DeleteAllItems();
  
  // Add items
  EventList* event_list = EventQueue.FindList();
  if (event_list) {
    for (auto it = event_list->items.begin(); it != event_list->items.end(); ++it) {
      Anime* anime = AnimeList.FindItem(it->anime_id);
      int i = list_.GetItemCount();
      if (anime) {
        list_.InsertItem(i, -1, -1, 0, nullptr, anime->series_title.c_str(), reinterpret_cast<LPARAM>(anime));
      } else {
        list_.InsertItem(i, -1, -1, 0, nullptr, L"???", 0);
      }
      wstring details;
      if (it->mode == HTTP_MAL_AnimeAdd)
        AppendString(details, L"Add to list");
      if (it->mode == HTTP_MAL_AnimeDelete)
        AppendString(details, L"Remove from list");
      if (it->episode > -1)
        AppendString(details, L"Episode: " + MAL.TranslateNumber(it->episode));
      if (it->score > -1)
        AppendString(details, L"Score: " + MAL.TranslateNumber(it->score));
      if (it->status > -1)
        AppendString(details, it->enable_rewatching != TRUE ? 
          L"Status: " + MAL.TranslateMyStatus(it->status, false) : L"Re-watching");
      if (it->tags != EMPTY_STR)
        AppendString(details, L"Tags: \"" + it->tags + L"\""); 
      list_.SetItem(i, 1, details.c_str());
      list_.SetItem(i, 2, it->time.c_str());
    }
  }

  // Set title
  switch (EventQueue.GetItemCount()) {
    case 0:
      SetText(L"Event Queue");
      break;
    case 1:
      SetText(L"Event Queue (1 item)");
      break;
    default:
      SetText(L"Event Queue (" + ToWSTR(EventQueue.GetItemCount()) + L" items)");
      break;
  }
}

bool EventDialog::MoveItems(int pos) {
  if (EventQueue.updating) {
    MessageBox(L"Event queue cannot be modified while an update is in progress.", L"Error", MB_ICONERROR);
    return false;
  }

  EventList* event_list = EventQueue.FindList();
  if (event_list == nullptr) return false;
  
  int index = -1;
  vector<bool> item_selected(list_.GetItemCount());
  vector<bool> item_selected_new(list_.GetItemCount());
  while ((index = list_.GetNextItem(index, LVNI_SELECTED)) > -1) {
    item_selected.at(index) = true;
  }

  for (size_t i = 0; i < item_selected.size(); i++) {
    size_t j = (pos < 0 ? i : item_selected.size() - 1 - i);
    if (!item_selected.at(j)) continue;
    if (j == (pos < 0 ? 0 : item_selected.size() - 1)) { item_selected_new.at(j) = true; continue; }
    if (item_selected_new.at(j + pos)) { item_selected_new.at(j) = true; continue; }
    std::iter_swap(event_list->items.begin() + j, event_list->items.begin() + j + pos);
    item_selected_new.at(j + pos) = true;
  }

  RefreshList();
  for (size_t i = 0; i < item_selected_new.size(); i++) {
    if (item_selected_new.at(i)) list_.SetSelectedItem(i);
  }

  return true;
}

bool EventDialog::RemoveItems() {
  if (EventQueue.updating) {
    MessageBox(L"Event queue cannot be modified while an update is in progress.", L"Error", MB_ICONERROR);
    return false;
  }

  if (list_.GetSelectedCount() > 0) {
    while (list_.GetSelectedCount() > 0) {
      int item_index = list_.GetNextItem(-1, LVNI_SELECTED);
      EventQueue.Remove(item_index, false, false);
      list_.DeleteItem(item_index);
    }
    Settings.Save();
  } else {
    EventQueue.Clear();
  }
  
  RefreshList();
  SetDlgItemText(IDC_BUTTON_EVENT_CLEAR, L"Remove all items");
  MainDialog.RefreshList();
  MainDialog.RefreshTabs();

  return true;
}