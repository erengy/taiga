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
#include "../common.h"
#include "dlg_search.h"
#include "../http.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"
#include "../win32/win_taskdialog.h"
#include "../xml.h"

CSearchWindow SearchWindow;

// =============================================================================

CSearchWindow::CSearchWindow() {
  RegisterDlgClass(L"TaigaSearchW");
}

BOOL CSearchWindow::OnInitDialog() {
  // Create list control
  m_List.Attach(GetDlgItem(IDC_LIST_SEARCH));
  m_List.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  m_List.SetImageList(UI.ImgList16.GetHandle());
  m_List.SetTheme();
  m_List.InsertColumn(0, 280, 280, LVCFMT_LEFT,   L"Anime title");
  m_List.InsertColumn(1,  60,  60, LVCFMT_CENTER, L"Type");
  m_List.InsertColumn(2,  60,  60, LVCFMT_RIGHT,  L"Episodes");
  m_List.InsertColumn(3,  60,  60, LVCFMT_RIGHT,  L"Score");
  m_List.InsertColumn(4, 100, 100, LVCFMT_RIGHT,  L"Season");

  // Create edit control
  m_Edit.Attach(GetDlgItem(IDC_EDIT_SEARCH));
  
  // Success
  return TRUE;
}

void CSearchWindow::OnOK() {
  wstring text;
  m_Edit.GetText(text);
  Search(text);
}

INT_PTR CSearchWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL: {
      return m_List.SendMessage(uMsg, wParam, lParam);
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT CSearchWindow::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // ListView control
  if (idCtrl == IDC_LIST_SEARCH) {
    switch (pnmh->code) {
      // Column click
      case LVN_COLUMNCLICK: {
        LPNMLISTVIEW lplv = (LPNMLISTVIEW)pnmh;
        int iOrder = m_List.GetSortOrder() * -1;
        if (iOrder == 0) iOrder = 1;
        switch (lplv->iSubItem) {
          // Episode
          case 2:
            m_List.Sort(lplv->iSubItem, iOrder, LISTSORTTYPE_NUMBER, ListViewCompareProc);
            break;
          // Season
          case 4:
            m_List.Sort(lplv->iSubItem, iOrder, LISTSORTTYPE_SEASON, ListViewCompareProc);
            break;
          // Other columns
          default:
            m_List.Sort(lplv->iSubItem, iOrder, LISTSORTTYPE_DEFAULT, ListViewCompareProc);
            break;
        }
        break;
      }

      // Double click
      case NM_DBLCLK: {
        if (m_List.GetSelectedCount() > 0) {
          LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
          if (lpnmitem->iItem == -1) break;
          LPARAM lParam = m_List.GetItemParam(lpnmitem->iItem);
          if (lParam) ExecuteAction(L"Info", 0, lParam);
        }
        break;
      }

      // Right click
      case NM_RCLICK: {
        LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
        if (lpnmitem->iItem == -1) break;
        LPARAM lParam = m_List.GetItemParam(lpnmitem->iItem);
        if (!lParam) break;
        UpdateSearchListMenu(reinterpret_cast<CAnime*>(lParam)->Index <= 0);
        ExecuteAction(UI.Menus.Show(pnmh->hwndFrom, 0, 0, L"SearchList"), 0, lParam);
        break;
      }

      // Custom draw
      case NM_CUSTOMDRAW: {
        LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmh);
        switch (pCD->nmcd.dwDrawStage) {
          case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;
          case CDDS_ITEMPREPAINT:
            return CDRF_NOTIFYSUBITEMDRAW;
          case CDDS_PREERASE:
          case CDDS_ITEMPREERASE:
            return CDRF_NOTIFYPOSTERASE;
          case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
            // Alternate background color
            if ((pCD->nmcd.dwItemSpec % 2) && !m_List.IsGroupViewEnabled()) {
              pCD->clrTextBk = RGB(248, 248, 248);
            }
            // Change text color
            CAnime* pAnimeItem = reinterpret_cast<CAnime*>(pCD->nmcd.lItemlParam);
            if (pAnimeItem) {
              if (pAnimeItem->Index > 0) {
                pCD->clrText = RGB(180, 180, 180);
              } else {
                pCD->clrText = GetSysColor(COLOR_WINDOWTEXT);
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

BOOL CSearchWindow::PreTranslateMessage(MSG* pMsg) {
  if (pMsg->message == WM_KEYDOWN) {
    switch (pMsg->wParam) {
      // Close window
      case VK_ESCAPE: {
        Destroy();
        return TRUE;
      }
      // Search
      case VK_RETURN: {
        if (::GetFocus() == m_Edit.GetWindowHandle()) {
          OnOK();
          return TRUE;
        }
        break;
      }
    }
  }
  return FALSE;
}

// =============================================================================

BOOL CSearchWindow::Search(const wstring& title) {
  if (MAL.SearchAnime(title)) {
    m_Edit.SetText(title.c_str());
    EnableDlgItem(IDOK, FALSE);
    SetDlgItemText(IDOK, L"Searching...");
    m_List.DeleteAllItems();
    m_Anime.clear();
    return TRUE;
  } else {
    return FALSE;
  }
}

void CSearchWindow::ParseResults(const wstring& data) {
  if (data.empty()) {
    wstring msg;
    m_Edit.GetText(msg);
    msg = L"No results found for \"" + msg + L"\".";
    CTaskDialog dlg;
    dlg.SetWindowTitle(L"Search Anime");
    dlg.SetMainIcon(TD_INFORMATION_ICON);
    dlg.SetMainInstruction(msg.c_str());
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(GetWindowHandle());
    return;
  }
  if (data == L"Invalid credentials") {
    CTaskDialog dlg;
    dlg.SetWindowTitle(L"Search Anime");
    dlg.SetMainIcon(TD_ERROR_ICON);
    dlg.SetMainInstruction(L"Invalid user name or password.");
    dlg.SetContent(L"Anime search requires authentication, which means, "
      L"you need to enter a valid user name and password to search MyAnimeList.");
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(GetWindowHandle());
    return;
  }
  m_Anime.clear();
  
  xml_document doc;
  xml_parse_result result = doc.load(data.c_str());
  if (result.status == status_ok) {
    xml_node anime = doc.child(L"anime");
    for (xml_node entry = anime.child(L"entry"); entry; entry = entry.next_sibling(L"entry")) {
      int i = m_Anime.size(); m_Anime.resize(i + 1);
      int anime_id = XML_ReadIntValue(entry, L"id");
      m_Anime[i].Index = AnimeList.FindItemByID(anime_id);
      
      if (m_Anime[i].Index > -1) {
        AnimeList.Item[m_Anime[i].Index].Score = XML_ReadStrValue(entry, L"score");
        AnimeList.Item[m_Anime[i].Index].Synopsis = XML_ReadStrValue(entry, L"synopsis");
        MAL.DecodeText(AnimeList.Item[m_Anime[i].Index].Synopsis);
      } else {
        m_Anime[i].Series_ID = anime_id;
        m_Anime[i].Series_Title = XML_ReadStrValue(entry, L"title");
        MAL.DecodeText(m_Anime[i].Series_Title);
        m_Anime[i].Series_Synonyms = XML_ReadStrValue(entry, L"synonyms");
        m_Anime[i].Series_Episodes = XML_ReadIntValue(entry, L"episodes");
        m_Anime[i].Score = XML_ReadStrValue(entry, L"score");
        m_Anime[i].Series_Type = MAL.TranslateType(XML_ReadStrValue(entry, L"type"));
        m_Anime[i].Series_Status = MAL.TranslateStatus(XML_ReadStrValue(entry, L"status"));
        m_Anime[i].Series_Start = XML_ReadStrValue(entry, L"start_date");
        m_Anime[i].Series_End = XML_ReadStrValue(entry, L"end_date");
        m_Anime[i].Synopsis = XML_ReadStrValue(entry, L"synopsis");
        MAL.DecodeText(m_Anime[i].Synopsis);
        m_Anime[i].Series_Image = XML_ReadStrValue(entry, L"image");
      }
    }
  }

  RefreshList();
}

void CSearchWindow::RefreshList() {
  if (!IsWindow()) return;

  // Hide and clear the list
  m_List.Show(SW_HIDE);
  m_List.DeleteAllItems();
  
  // Add anime items to list
  for (size_t i = 0; i < m_Anime.size(); i++) {
    if (m_Anime[i].Index == -1) {
      m_Anime[i].Index = AnimeList.FindItemByID(m_Anime[i].Series_ID);
    }
    CAnime* item = m_Anime[i].Index > -1 ? &AnimeList.Item[m_Anime[i].Index] : &m_Anime[i];
    m_List.InsertItem(i, -1, StatusToIcon(item->Series_Status), item->Series_Title.c_str(), 
      reinterpret_cast<LPARAM>(item));
    m_List.SetItem(i, 1, MAL.TranslateType(item->Series_Type).c_str());
    m_List.SetItem(i, 2, MAL.TranslateNumber(item->Series_Episodes).c_str());
    m_List.SetItem(i, 3, item->Score.c_str());
    m_List.SetItem(i, 4, MAL.TranslateDate(item->Series_Start).c_str());
  }

  // Sort and show the list again
  m_List.Sort(0, 1, LISTSORTTYPE_DEFAULT, ListViewCompareProc);
  m_List.Show();
}