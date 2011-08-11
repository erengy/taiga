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
#include "../animedb.h"
#include "../animelist.h"
#include "../common.h"
#include "dlg_season.h"
#include "../gfx.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"
#include "../win32/win_gdi.h"

CSeasonWindow SeasonWindow;

// =============================================================================

CSeasonWindow::CSeasonWindow() :
  GroupBy(SEASONBROWSER_GROUPBY_TYPE), SortBy(SEASONBROWSER_SORTBY_NAME)
{
  RegisterDlgClass(L"TaigaSeasonW");
}

BOOL CSeasonWindow::OnInitDialog() {
  // Set properties
  SetSizeMin(575, 310);
  SetIconLarge(IDI_MAIN);
  SetIconSmall(IDI_MAIN);

  // Create list
  m_List.Attach(GetDlgItem(IDC_LIST_SEASON));
  m_List.EnableGroupView(true);
  m_List.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT);
  m_List.SetTheme();
  SIZE size = {520, 200};
  m_List.SetTileViewInfo(0, LVTVIF_FIXEDSIZE, nullptr, &size);
  m_List.SetView(LV_VIEW_TILE);

  // Create main toolbar
  m_Toolbar.Attach(GetDlgItem(IDC_TOOLBAR_SEASON));
  m_Toolbar.SetImageList(UI.ImgList16.GetHandle(), 16, 16);
  m_Toolbar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  // Create search toolbar
  m_ToolbarFilter.Attach(GetDlgItem(IDC_TOOLBAR_SEARCH));
  m_ToolbarFilter.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  // Create search text
  m_EditFilter.Attach(GetDlgItem(IDC_EDIT_SEASON_FILTER));
  m_EditFilter.SetCueBannerText(L"Filter");
  m_EditFilter.SetParent(m_ToolbarFilter.GetWindowHandle());
  m_EditFilter.SetPosition(NULL, 0, 0, 160, 20);
  m_EditFilter.SetMargins(1, 16);
  CRect rect_edit; m_EditFilter.GetRect(&rect_edit);
  // Create cancel filter button
  m_CancelFilter.Attach(GetDlgItem(IDC_BUTTON_CANCELFILTER));
  m_CancelFilter.SetParent(m_EditFilter.GetWindowHandle());
  m_CancelFilter.SetPosition(NULL, rect_edit.right + 1, 0, 16, 16);

  // Insert toolbar buttons
  BYTE fsStyle1 = BTNS_AUTOSIZE | BTNS_SHOWTEXT;
  BYTE fsStyle2 = BTNS_AUTOSIZE | BTNS_SHOWTEXT | BTNS_WHOLEDROPDOWN;
  m_Toolbar.InsertButton(0, Icon16_Calendar, 100, 1, fsStyle2, 0, L"Select season", NULL);
  m_Toolbar.InsertButton(1, Icon16_Refresh,  101, 1, fsStyle1, 1, L"Refresh data", L"Download anime details and missing images");
  m_Toolbar.InsertButton(2, 0, 0, 0, BTNS_SEP, NULL, NULL, NULL);
  m_Toolbar.InsertButton(3, Icon16_Category, 103, 1, fsStyle2, 3, L"Group by", NULL);
  m_Toolbar.InsertButton(4, Icon16_Sort,     104, 1, fsStyle2, 4, L"Sort by", NULL);
  #ifdef _DEBUG
  m_Toolbar.EnableButton(1, false);
  #endif

  // Create rebar
  m_Rebar.Attach(GetDlgItem(IDC_REBAR_SEASON));
  // Insert rebar bands
  UINT fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE;
  UINT fStyle = RBBS_NOGRIPPER;
  m_Rebar.InsertBand(NULL, 0, 0, 0, 0, 0, 0, 0, 0, fMask, fStyle);
  m_Rebar.InsertBand(m_Toolbar.GetWindowHandle(), GetSystemMetrics(SM_CXSCREEN), 0, 0, 0, 0, 0, 0, 
    HIWORD(m_Toolbar.GetButtonSize()) + (HIWORD(m_Toolbar.GetPadding()) / 2), fMask, fStyle);
  m_Rebar.InsertBand(m_ToolbarFilter.GetWindowHandle(), 0, 0, 0, 170, 0, 0, 0, 20, fMask, fStyle);

  // Create status bar
  m_Status.Attach(GetDlgItem(IDC_STATUSBAR_SEASON));
  m_Status.SetImageList(UI.ImgList16.GetHandle());

  // Refresh
  RefreshData(false);
  RefreshList();
  RefreshToolbar();

  return TRUE;
}

INT_PTR CSeasonWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL CSeasonWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
  // Toolbar
  switch (LOWORD(wParam)) {
    // Refresh data
    case 101:
      RefreshData(true);
      return TRUE;
  }

  // Filter text
  if (HIWORD(wParam) == EN_CHANGE) {
    if (LOWORD(wParam) == IDC_EDIT_SEASON_FILTER) {
      wstring filter_text;
      m_EditFilter.GetText(filter_text);
      m_CancelFilter.Show(filter_text.empty() ? SW_HIDE : SW_SHOWNORMAL);
      RefreshList();
      return TRUE;
    }
  }

  return FALSE;
}

BOOL CSeasonWindow::OnDestroy() {
  // Save database
  SeasonDatabase.Write(L"2011_summer.xml"); // TODO
  
  // Free some memory
  Images.clear();
  
  return TRUE;
}

LRESULT CSeasonWindow::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // List
  if (idCtrl == IDC_LIST_SEASON) {
    return OnListNotify(reinterpret_cast<LPARAM>(pnmh));
  // Toolbar
  } else if (idCtrl == IDC_TOOLBAR_SEASON) {
    return OnToolbarNotify(reinterpret_cast<LPARAM>(pnmh));
  // Button
  } else if (idCtrl == IDC_BUTTON_CANCELFILTER) {
    if (pnmh->code == NM_CUSTOMDRAW) {
      return OnButtonCustomDraw(reinterpret_cast<LPARAM>(pnmh));
    }
  }
  
  return 0;
}

void CSeasonWindow::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      CRect rcWindow;
      rcWindow.Set(0, 0, size.cx, size.cy);
      rcWindow.Inflate(-ScaleX(WIN_CONTROL_MARGIN), -ScaleY(WIN_CONTROL_MARGIN));
      // Resize rebar
      m_Rebar.SendMessage(WM_SIZE, 0, 0);
      rcWindow.top += m_Rebar.GetBarHeight() + ScaleY(WIN_CONTROL_MARGIN / 2);
      // Resize status bar
      CRect rcStatus;
      m_Status.GetClientRect(&rcStatus);
      m_Status.SendMessage(WM_SIZE, 0, 0);
      rcWindow.bottom -= rcStatus.Height();
      // Resize list
      m_List.SetPosition(NULL, rcWindow);
    }
  }
}

BOOL CSeasonWindow::PreTranslateMessage(MSG* pMsg) {
  switch (pMsg->message) {
    case WM_KEYDOWN: {
      if (::GetFocus() == m_EditFilter.GetWindowHandle()) {
        switch (pMsg->wParam) {
          // Clear filter text
          case VK_ESCAPE: {
            m_EditFilter.SetText(L"");
            return TRUE;
          }
        }
      }
      break;
    }
  }

  return FALSE;
}

// =============================================================================

LRESULT CSeasonWindow::OnListNotify(LPARAM lParam) {
  LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
  switch (pnmh->code) {
    // Custom draw
    case NM_CUSTOMDRAW: {
      return OnListCustomDraw(lParam);
    }

    // Double click
    case NM_DBLCLK: {
      LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
      if (lpnmitem->iItem == -1) break;
      CAnime* anime = reinterpret_cast<CAnime*>(m_List.GetItemParam(lpnmitem->iItem));
      if (anime) {
        CAnime* anime_onlist = AnimeList.FindItem(anime->Series_ID);
        ExecuteAction(L"Info", 0, reinterpret_cast<LPARAM>(
          anime_onlist ? anime_onlist : anime));
      }
      break;
    }

    // Right click
    case NM_RCLICK: {
      LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
      if (lpnmitem->iItem == -1) break;
      CAnime* anime = reinterpret_cast<CAnime*>(m_List.GetItemParam(lpnmitem->iItem));
      if (anime) {
        CAnime* anime_onlist = AnimeList.FindItem(anime->Series_ID);
        UpdateSearchListMenu(anime_onlist == nullptr);
        ExecuteAction(UI.Menus.Show(pnmh->hwndFrom, 0, 0, L"SearchList"), 0, 
          reinterpret_cast<LPARAM>(anime_onlist ? anime_onlist : anime));
        m_List.RedrawWindow();
      }
      break;
    }
  }

  return 0;
}

LRESULT CSeasonWindow::OnListCustomDraw(LPARAM lParam) {
  LRESULT result = CDRF_DODEFAULT;
  LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

  CDC hdc = pCD->nmcd.hdc;
  CRect rect = pCD->nmcd.rc;

  switch (pCD->nmcd.dwDrawStage) {
    case CDDS_PREPAINT: {
      result = CDRF_NOTIFYITEMDRAW;
      break;
    }
    case CDDS_ITEMPREPAINT: {
      result = CDRF_NOTIFYPOSTPAINT;
      break;
    }
    case CDDS_ITEMPOSTPAINT: {
      CAnime* anime = reinterpret_cast<CAnime*>(pCD->nmcd.lItemlParam);
      if (!anime) break;

      // Draw border
      rect.Inflate(-4, -4);
      hdc.FillRect(rect, RGB(230, 230, 230));
      // Draw background
      rect.Inflate(-1, -1);
      hdc.FillRect(rect, RGB(250, 250, 250));

      // Draw image
      rect.Inflate(-4, -4);
      CRect rect_image = rect;
      rect_image.right = rect_image.left + 120;
      int image_index = -1;
      for (size_t i = 0; i < Images.size(); i++) {
        if (Images.at(i).Data == anime->Series_ID) {
          image_index = static_cast<int>(i);
          break;
        }
      }
      if (image_index > -1 && Images.at(image_index).DC.Get()) {
        rect_image = ResizeRect(rect_image, 
          Images.at(image_index).Width,
          Images.at(image_index).Height,
          true, true, false);
        hdc.SetStretchBltMode(HALFTONE);
        hdc.StretchBlt(rect_image.left, rect_image.top, 
          rect_image.Width(), rect_image.Height(), 
          Images.at(image_index).DC.Get(), 0, 0, 
          Images.at(image_index).Width, 
          Images.at(image_index).Height, 
          SRCCOPY);
      }
      
      // Draw title background
      rect.left += 120 + 4;
      CRect rect_text = rect;
      rect_text.bottom = rect_text.top + 20;
      CAnime* anime_onlist = AnimeList.FindItem(anime->Series_ID);
      hdc.FillRect(rect_text, anime_onlist ? RGB(225, 245, 231) : MAL_LIGHTBLUE);
      // Draw title
      rect_text.Inflate(-4, 0);
      hdc.EditFont(NULL, -1, TRUE);
      hdc.SetBkMode(TRANSPARENT);
      hdc.DrawText(anime->Series_Title.c_str(), anime->Series_Title.length(), rect_text, 
        DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
      DeleteObject(hdc.DetachFont());

      // Draw details
      rect_text.left += 2;
      rect_text.top = rect_text.bottom + 8;
      rect_text.bottom = rect.bottom;
      wstring text = 
        L"Aired: \t\t" + MAL.TranslateDate(anime->Series_Start, L"MMM dd',' yyyy") 
        + L" (" + MAL.TranslateStatus(anime->GetAiringStatus()) + L")\n"
        L"Episodes: \t" + MAL.TranslateNumber(anime->Series_Episodes, L"Unknown") + L"\n"
        L"Genres: \t" + anime->Genres + L"\n"
        L"Producers: \t" + anime->Producers + L"\n"
        L"Score: \t\t" + (anime->Score.empty() ? L"0.00" : anime->Score) + L"\n"
        L"Rank: \t\t" + (anime->Rank.empty() ? L"#0" : anime->Rank) + L"\n"
        L"Popularity: \t" + (anime->Popularity.empty() ? L"#0" : anime->Popularity);
      hdc.DrawText(text.c_str(), text.length(), rect_text, 
        DT_END_ELLIPSIS | DT_EXPANDTABS | DT_NOPREFIX);
      // Draw synopsis
      rect_text.top += 98;
      text = anime->Synopsis;
      hdc.DrawText(text.c_str(), text.length(), rect_text, 
        DT_END_ELLIPSIS | DT_NOPREFIX | DT_WORDBREAK);
      break;
    }
  }

  hdc.DetachDC();
  return result;
}

LRESULT CSeasonWindow::OnToolbarNotify(LPARAM lParam) {
  switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
    // Dropdown button click
    case TBN_DROPDOWN: {
      RECT rect; LPNMTOOLBAR nmt = reinterpret_cast<LPNMTOOLBAR>(lParam);
      ::SendMessage(nmt->hdr.hwndFrom, TB_GETRECT, static_cast<WPARAM>(nmt->iItem), reinterpret_cast<LPARAM>(&rect));          
      MapWindowPoints(nmt->hdr.hwndFrom, HWND_DESKTOP, reinterpret_cast<LPPOINT>(&rect), 2);
      UpdateSeasonMenu();
      wstring action;
      switch (LOWORD(nmt->iItem)) {
        // Select season
        case 100:
          action = UI.Menus.Show(m_hWindow, rect.left, rect.bottom, L"SeasonSelect");
          break;
        // Group by
        case 103:
          action = UI.Menus.Show(m_hWindow, rect.left, rect.bottom, L"SeasonGroup");
          break;
        // Sort by
        case 104:
          action = UI.Menus.Show(m_hWindow, rect.left, rect.bottom, L"SeasonSort");
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
      if (git->hdr.hwndFrom == m_Toolbar.GetWindowHandle()) {
        git->pszText = const_cast<LPWSTR>(m_Toolbar.GetButtonTooltip(git->lParam));
      }
      break;
    }
  }

  return 0L;
}

LRESULT CSeasonWindow::OnButtonCustomDraw(LPARAM lParam) {
  LPNMCUSTOMDRAW pCD = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);

  switch (pCD->dwDrawStage) {
    case CDDS_PREPAINT: {
      CDC dc = pCD->hdc;
      dc.FillRect(pCD->rc, ::GetSysColor(COLOR_WINDOW));
      UI.ImgList16.Draw(Icon16_Cross, dc.Get(), 0, 0);
      dc.DetachDC();
      return CDRF_SKIPDEFAULT;
    }
  }

  return 0;
}

LRESULT CSeasonWindow::CEditFilter::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_COMMAND: {
      if (HIWORD(wParam) == BN_CLICKED) {
        // Clear filter text
        if (LOWORD(wParam) == IDC_BUTTON_CANCELFILTER) {
          SetText(L"");
          return TRUE;
        }
      }
      break;
    }
  }
  
  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

// =============================================================================

void CSeasonWindow::RefreshData(bool connect) {
  size_t size = SeasonDatabase.Items.size();
  if (Images.size() != size) Images.resize(size);
  if (ImageClients.size() != size) ImageClients.resize(size);
  if (InfoClients.size() != size) InfoClients.resize(size);

  for (auto i = SeasonDatabase.Items.begin(); i != SeasonDatabase.Items.end(); ++i) {
    size_t index = i - SeasonDatabase.Items.begin();
    // Load available image
    Images.at(index).Data = i->Series_ID;
    Images.at(index).Load(i->GetImagePath());
    // Download missing image
    if (connect && Images.at(index).DC.Get() == nullptr) {
      MAL.DownloadImage(&(*i), &ImageClients.at(index));
    }
    // Get details
    if (connect) {
      MAL.SearchAnime(i->Series_Title, &(*i), &InfoClients.at(index));
    }
  }

  m_List.RedrawWindow();
}

void CSeasonWindow::RefreshList() {
  if (!IsWindow()) return;

  // Set title
  if (SeasonDatabase.Name.empty()) {
    SetText(L"Season Browser");
  } else {
    SetText(L"Season Browser - " + SeasonDatabase.Name);
  }

  // Disable drawing
  m_List.SetRedraw(FALSE);

  // Insert list groups
  m_List.RemoveAllGroups();
  switch (GroupBy) {
    case SEASONBROWSER_GROUPBY_AIRINGSTATUS:
      for (int i = MAL_AIRING; i <= MAL_NOTYETAIRED; i++) {
        m_List.InsertGroup(i, MAL.TranslateStatus(i).c_str(), true, false);
      }
      break;
    case SEASONBROWSER_GROUPBY_LISTSTATUS:
      for (int i = MAL_NOTINLIST; i <= MAL_PLANTOWATCH; i++) {
        m_List.InsertGroup(i, MAL.TranslateMyStatus(i, false).c_str(), true, false);
      }
      break;
    case SEASONBROWSER_GROUPBY_TYPE:
      for (int i = MAL_TV; i <= MAL_MUSIC; i++) {
        m_List.InsertGroup(i, MAL.TranslateType(i).c_str(), true, false);
      }
      break;
  }

  // Filter
  wstring filter_text;
  m_EditFilter.GetText(filter_text);
  vector<wstring> filters;
  Split(filter_text, L" ", filters);

  // Add items
  m_List.DeleteAllItems();
  for (auto i = SeasonDatabase.Items.begin(); i != SeasonDatabase.Items.end(); ++i) {
    bool passed_filters = true;
    for (auto j = filters.begin(); j != filters.end(); ++j) {
      if (InStr(i->Genres, *j, 0, true) == -1 && 
          InStr(i->Producers, *j, 0, true) == -1 && 
          InStr(i->Series_Title, *j, 0, true) == -1) {
            passed_filters = false;
            break;
      }
    }
    if (!passed_filters) continue;
    int group = -1;
    switch (GroupBy) {
      case SEASONBROWSER_GROUPBY_AIRINGSTATUS:
        group = i->GetAiringStatus();
        break;
      case SEASONBROWSER_GROUPBY_LISTSTATUS: {
        CAnime* anime_onlist = AnimeList.FindItem(i->Series_ID);
        group = anime_onlist ? anime_onlist->GetStatus() : MAL_NOTINLIST;
        break;
      }
      case SEASONBROWSER_GROUPBY_TYPE:
        group = i->Series_Type;
        break;
    }
    m_List.InsertItem(i - SeasonDatabase.Items.begin(), 
      group, -1, 0, nullptr, i->Series_Title.c_str(), 
      reinterpret_cast<LPARAM>(&(*i)));
  }
  
  // Sort items
  switch (SortBy) {
    case SEASONBROWSER_SORTBY_EPISODES:
      m_List.Sort(0, -1, LISTSORTTYPE_EPISODES, ListViewCompareProc);
      break;
    case SEASONBROWSER_SORTBY_NAME:
      m_List.Sort(0, 1, LISTSORTTYPE_DEFAULT, ListViewCompareProc);
      break;
    case SEASONBROWSER_SORTBY_SCORE:
      m_List.Sort(0, -1, LISTSORTTYPE_SCORE, ListViewCompareProc);
      break;
    case SEASONBROWSER_SORTBY_STARTDATE:
      m_List.Sort(0, -1, LISTSORTTYPE_STARTDATE, ListViewCompareProc);
      break;
  }

  // Redraw
  m_List.SetRedraw(TRUE);
  m_List.RedrawWindow(nullptr, nullptr, 
    RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
}

void CSeasonWindow::RefreshToolbar() {
  wstring text;
  
  text = L"Group by: ";
  switch (GroupBy) {
    case SEASONBROWSER_GROUPBY_AIRINGSTATUS:
      text += L"Airing status";
      break;
    case SEASONBROWSER_GROUPBY_LISTSTATUS:
      text += L"List status";
      break;
    case SEASONBROWSER_GROUPBY_TYPE:
      text += L"Type";
      break;
  }
  m_Toolbar.SetButtonText(3, text.c_str());

  text = L"Sort by: ";
  switch (SortBy) {
    case SEASONBROWSER_SORTBY_EPISODES:
      text += L"Episodes";
      break;
    case SEASONBROWSER_SORTBY_NAME:
      text += L"Name";
      break;
    case SEASONBROWSER_SORTBY_SCORE:
      text += L"Score";
      break;
    case SEASONBROWSER_SORTBY_STARTDATE:
      text += L"Start date";
      break;
  }
  m_Toolbar.SetButtonText(4, text.c_str());
}