/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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
#include "../event.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"

CEventWindow EventWindow;

// =============================================================================

CEventWindow::CEventWindow() {
  RegisterDlgClass(L"TaigaEventW");
}

BOOL CEventWindow::OnInitDialog() {
  // Create list
  m_List.Attach(GetDlgItem(IDC_LIST_EVENT));
  m_List.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  m_List.SetTheme();
  
  // Insert list columns
  m_List.InsertColumn(0, 190, 190, LVCFMT_LEFT,   L"Anime Title");
  m_List.InsertColumn(1,  55,  55, LVCFMT_CENTER, L"Episode");
  m_List.InsertColumn(2,  50,  50, LVCFMT_CENTER, L"Score");
  m_List.InsertColumn(3, 110, 110, LVCFMT_LEFT,   L"Status");
  m_List.InsertColumn(4, 110, 110, LVCFMT_LEFT,   L"Tags");
  m_List.InsertColumn(5, 120, 120, LVCFMT_LEFT,   L"Time");
  m_List.SetColumnWidth(5, LVSCW_AUTOSIZE_USEHEADER);
  
  // Refresh list
  RefreshList();
  return TRUE;
}

BOOL CEventWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Clear all items
    case IDC_BUTTON_EVENT_CLEAR: {
      EventBuffer.Clear();
      RefreshList();
      return TRUE;
    }
  }

  return FALSE;
}

LRESULT CEventWindow::OnNotify(int idCtrl, LPNMHDR pnmh) {
  if (pnmh->hwndFrom == m_List.GetWindowHandle()) {
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

void CEventWindow::OnOK() {
  EventBuffer.Check();
}

void CEventWindow::RefreshList() {
  if (!IsWindow()) return;
  
  // Clear list
  m_List.DeleteAllItems();
  
  // Add items
  int user_index = EventBuffer.GetUserIndex();
  if (user_index > -1) {
    for (size_t i = 0; i < EventBuffer.List[user_index].Item.size(); i++) {
      int episode  = EventBuffer.List[user_index].Item[i].Episode; if (episode == -1) episode++;
      int score    = EventBuffer.List[user_index].Item[i].Score;   if (score == -1) score++;
      int status   = EventBuffer.List[user_index].Item[i].Status;  if (status == -1) status++;
      wstring tags = EventBuffer.List[user_index].Item[i].Tags;    if (tags == L"%empty%") tags.clear();

      if (EventBuffer.List[user_index].Item[i].Index <= AnimeList.Count) {
        m_List.InsertItem(i, -1, -1, AnimeList.Item[EventBuffer.List[user_index].Item[i].Index].Series_Title.c_str(), 0);
        m_List.SetItem(i, 1, MAL.TranslateNumber(episode, L"").c_str());
        m_List.SetItem(i, 2, MAL.TranslateNumber(score, L"").c_str());
        m_List.SetItem(i, 3, MAL.TranslateMyStatus(status, false).c_str());
        m_List.SetItem(i, 4, tags.c_str());
        m_List.SetItem(i, 5, EventBuffer.List[user_index].Item[i].Time.c_str());
      }
    }
  }

  // Set title
  switch (EventBuffer.GetItemCount()) {
    case 0:
      SetText(L"Event queue is empty.");
      break;
    case 1:
      SetText(L"There is 1 item in the event queue.");
      break;
    default:
      SetText(WSTR(L"There are " + WSTR(EventBuffer.GetItemCount()) + L" items in the event queue.").c_str());
      break;
  }
}