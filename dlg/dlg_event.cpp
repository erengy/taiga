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
#include "../animelist.h"
#include "dlg_event.h"
#include "dlg_main.h"
#include "../event.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"

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
  list_.InsertColumn(0, 190, 190, LVCFMT_LEFT,   L"Anime title");
  list_.InsertColumn(1,  55,  55, LVCFMT_CENTER, L"Episode");
  list_.InsertColumn(2,  50,  50, LVCFMT_CENTER, L"Score");
  list_.InsertColumn(3, 110, 110, LVCFMT_LEFT,   L"Status");
  list_.InsertColumn(4, 110, 110, LVCFMT_LEFT,   L"Tags");
  list_.InsertColumn(5, 120, 120, LVCFMT_LEFT,   L"Last modified");
  list_.SetColumnWidth(5, LVSCW_AUTOSIZE_USEHEADER);
  
  // Refresh list
  RefreshList();
  return TRUE;
}

BOOL EventDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Clear all items
    case IDC_BUTTON_EVENT_CLEAR: {
      if (EventQueue.updating) {
        MessageBox(L"Event list can not be cleared while an update is in progress.", L"Error", MB_ICONERROR);
      } else {
        EventQueue.Clear();
        RefreshList();
        MainDialog.RefreshList();
        MainDialog.RefreshTabs();
        return TRUE;
      }
    }
  }

  return FALSE;
}

LRESULT EventDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  if (pnmh->hwndFrom == list_.GetWindowHandle()) {
    switch (pnmh->code) {
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
  EventQueue.Check();
}

void EventDialog::RefreshList() {
  if (!IsWindow()) return;
  
  // Clear list
  list_.DeleteAllItems();
  
  // Add items
  EventList* event_list = EventQueue.FindList();
  if (event_list) {
    for (auto it = event_list->items.begin(); it != event_list->items.end(); ++it) {
      int episode = it->episode; if (episode == -1) episode++;
      int score = it->score; if (score == -1) score++;
      int status = it->status;
      int rewatching = it->enable_rewatching;
      wstring tags = it->tags; if (tags == EMPTY_STR) tags.clear();
      Anime* anime = AnimeList.FindItem(it->anime_id);
      int i = list_.GetItemCount();
      if (anime) {
        list_.InsertItem(i, -1, -1, 0, nullptr, anime->series_title.c_str(), reinterpret_cast<LPARAM>(anime));
      } else {
        list_.InsertItem(i, -1, -1, 0, nullptr, L"???", 0);
      }
      list_.SetItem(i, 1, MAL.TranslateNumber(episode, L"").c_str());
      list_.SetItem(i, 2, MAL.TranslateNumber(score, L"").c_str());
      list_.SetItem(i, 3, rewatching != TRUE ? MAL.TranslateMyStatus(status, false).c_str() : L"Re-watching");
      list_.SetItem(i, 4, tags.c_str());
      list_.SetItem(i, 5, it->time.c_str());
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