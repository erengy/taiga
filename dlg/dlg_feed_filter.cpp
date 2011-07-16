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
#include "../common.h"
#include "dlg_feed_condition.h"
#include "dlg_feed_filter.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"
#include "../theme.h"

CFeedFilterWindow FeedFilterWindow;

// =============================================================================

CFeedFilterWindow::CFeedFilterWindow() : 
  m_hfHeader(NULL)
{
  RegisterDlgClass(L"TaigaFeedFilterW");
}

CFeedFilterWindow::~CFeedFilterWindow() {
  if (m_hfHeader) {
    ::DeleteObject(m_hfHeader);
    m_hfHeader = NULL;
  }
}

BOOL CFeedFilterWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void CFeedFilterWindow::OnCancel() {
  m_Filter.Conditions.clear();

  // Exit
  EndDialog(IDOK);
}

BOOL CFeedFilterWindow::OnInitDialog() {
  // Create bold header font
  if (!m_hfHeader) {
    LOGFONT lFont;
    ::GetObject(GetFont(), sizeof(LOGFONT), &lFont);
    lFont.lfCharSet = DEFAULT_CHARSET;
    lFont.lfWeight = FW_BOLD;
    m_hfHeader = ::CreateFontIndirect(&lFont);
  }

  // Create basic page
  if (m_Filter.Conditions.empty()) {
    m_PageBasic.m_Parent = this;
    m_PageBasic.Create(IDD_FEED_FILTER_BASIC, GetWindowHandle(), false);
    EnableThemeDialogTexture(m_PageBasic.GetWindowHandle(), ETDT_ENABLETAB);
  }
  // Create advanced page
  m_PageAdvanced.m_Parent = this;
  m_PageAdvanced.Create(IDD_FEED_FILTER_ADVANCED, GetWindowHandle(), false);
  EnableThemeDialogTexture(m_PageAdvanced.GetWindowHandle(), ETDT_ENABLETAB);

  // Initialize tab
  m_Tab.Attach(GetDlgItem(IDC_TAB_FEED_FILTER));
  // Insert tab items
  if (m_Filter.Conditions.empty()) {
    m_Tab.InsertItem(0, L"Basic", 0);
    m_Tab.InsertItem(1, L"Advanced", 1);
    m_Tab.SetCurrentlySelected(0);
    m_PageBasic.Show();
  } else {
    SetText(L"Edit Filter");
    m_PageAdvanced.Show();
  }

  // Set pages position
  RECT rect; m_Tab.AdjustRect(m_hWindow, FALSE, &rect);
  m_PageBasic.SetPosition(NULL, rect, 0);
  m_PageAdvanced.SetPosition(NULL, rect, 0);

  return TRUE;
}

LRESULT CFeedFilterWindow::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (idCtrl) {
    // Tabs
    case IDC_TAB_FEED_FILTER:
      switch (pnmh->code) {
        // Tab change
        case TCN_SELCHANGE: {
          int index = static_cast<int>(m_Tab.GetItemParam(m_Tab.GetCurrentlySelected()));
          m_PageBasic.Show(index == 0);
          m_PageAdvanced.Show(index == 1);
          break;
        }
      }
      break;
  }

  return 0;
}

void CFeedFilterWindow::OnOK() {
  // Set values
  if (m_Tab.GetItemCount() > 0 && m_Tab.GetItemParam(m_Tab.GetCurrentlySelected()) == 0) {
    // Basic
    if (!m_PageBasic.BuildFilter(m_Filter)) return;
  } else {
    // Advanced
    if (!m_PageAdvanced.BuildFilter(m_Filter)) return;
  }

  // Exit
  EndDialog(IDOK);
}

// =============================================================================

/* Basic */

BOOL CFeedFilterWindow::CDialogPageBasic::OnInitDialog() {
  // Set new font for headers
  for (int i = 0; i < 2; i++) {
    SendDlgItemMessage(IDC_STATIC_HEADER1 + i, WM_SETFONT, 
      reinterpret_cast<WPARAM>(m_Parent->m_hfHeader), FALSE);
  }

  //
  m_ComboOption1.Attach(GetDlgItem(IDC_COMBO_FEED_FILTER_OPTION1));
  m_ComboOption2.Attach(GetDlgItem(IDC_COMBO_FEED_FILTER_OPTION2));
  m_ComboOption1.SetCurSel(m_Parent->m_Filter.Match);

  //
  CheckRadioButton(IDC_RADIO_FEED_FILTER_BASIC1, IDC_RADIO_FEED_FILTER_BASIC2, 
    IDC_RADIO_FEED_FILTER_BASIC1);
  BuildOptions(0);

  return TRUE;
}

BOOL CFeedFilterWindow::CDialogPageBasic::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (HIWORD(wParam)) {
    case BN_CLICKED: {
      switch (LOWORD(wParam)) {
        case IDC_RADIO_FEED_FILTER_BASIC1:
        case IDC_RADIO_FEED_FILTER_BASIC2: {
          CheckRadioButton(IDC_RADIO_FEED_FILTER_BASIC1, IDC_RADIO_FEED_FILTER_BASIC2, LOWORD(wParam));
          BuildOptions(LOWORD(wParam) - IDC_RADIO_FEED_FILTER_BASIC1);
          return TRUE;
        }
      }
      break;
    }
  }

  return FALSE;
}

bool CFeedFilterWindow::CDialogPageBasic::BuildFilter(CFeedFilter& filter) {
  int mode = GetCheckedRadioButton(IDC_RADIO_FEED_FILTER_BASIC1, IDC_RADIO_FEED_FILTER_BASIC2);

  switch (mode) {
    // Default filters  
    case 0: {
      int index = m_ComboOption1.GetCurSel();
      m_Parent->m_Filter = Aggregator.FilterManager.DefaultFilters.at(index);
      break;
    }

    // Fansub group
    case 1: {
      wstring group_name;
      m_ComboOption2.GetText(group_name);
      Trim(group_name);
      if (group_name.empty()) {
        HWND hEdit = ::FindWindowEx(m_ComboOption2.GetWindowHandle(), NULL, NULL, NULL);
        if (hEdit) {
          CEdit edit = hEdit;
          edit.ShowBalloonTip(L"You must enter a fansub group name.", L"Required field", TTI_ERROR);
          edit.SetWindowHandle(NULL);
        }
        return false;
      }
      CAnime* anime = reinterpret_cast<CAnime*>(m_ComboOption1.GetItemData(m_ComboOption1.GetCurSel()));
      if (anime) {
        // Set properties
        m_Parent->m_Filter.Name = L"[Fansub] " + anime->Series_Title;
        m_Parent->m_Filter.Match = FEED_FILTER_MATCH_ALL;
        m_Parent->m_Filter.Action = FEED_FILTER_ACTION_DISCARD;
        // Add conditions
        m_Parent->m_Filter.AddCondition(FEED_FILTER_ELEMENT_ANIME_ID, 
          FEED_FILTER_OPERATOR_IS, ToWSTR(anime->Series_ID));
        m_Parent->m_Filter.AddCondition(FEED_FILTER_ELEMENT_ANIME_GROUP, 
          FEED_FILTER_OPERATOR_ISNOT, group_name);
      }
      break;
    }
  }

  return true;
}

void CFeedFilterWindow::CDialogPageBasic::BuildOptions(int mode) {
  m_ComboOption1.ResetContent();
  
  switch (mode) {
    // Default filters  
    case 0:
      SetDlgItemText(IDC_STATIC_FEED_FILTER_OPTION1, L"Filter:");
      for (auto it = Aggregator.FilterManager.DefaultFilters.begin(); it != Aggregator.FilterManager.DefaultFilters.end(); ++it) {
        m_ComboOption1.AddString(it->Name.c_str());
      }
      ShowDlgItem(IDC_STATIC_FEED_FILTER_OPTION2, SW_HIDE);
      m_ComboOption2.Hide();
      break;

    // Fansub group
    case 1: {
      SetDlgItemText(IDC_STATIC_FEED_FILTER_OPTION1, L"Anime title:");
      SetDlgItemText(IDC_STATIC_FEED_FILTER_OPTION2, L"Fansub group name:");
      ShowDlgItem(IDC_STATIC_FEED_FILTER_OPTION2);
      m_ComboOption2.Show();
      typedef std::pair<LPARAM, wstring> anime_pair;
      vector<anime_pair> title_list;
      for (auto it = AnimeList.Item.begin() + 1; it != AnimeList.Item.end(); ++it) {
        title_list.push_back(std::make_pair((LPARAM)&(*it), it->Series_Title));
      }
      std::sort(title_list.begin(), title_list.end(), 
        [](const anime_pair& a1, const anime_pair& a2) {
          return CompareStrings(a1.second, a2.second) < 0;
        });
      for (auto it = title_list.begin(); it != title_list.end(); ++it) {
        m_ComboOption1.AddString(it->second.c_str());
        m_ComboOption1.SetItemData(it - title_list.begin(), it->first);
      }
      break;
    }
  }

  m_ComboOption1.SetCurSel(0);
}

void CFeedFilterWindow::CDialogPageBasic::BuildOptions2(int mode) {
  m_ComboOption2.ResetContent();

  switch (mode) {
    // Default filters  
    case 0:
      //m_ComboOption2.SetCurSel(0);
      break;

    // Fansub group
    case 1: {
      //m_ComboOption2.AddString(L"gg");
      break;
    }
  }
}

// =============================================================================

/* Advanced */

BOOL CFeedFilterWindow::CDialogPageAdvanced::OnInitDialog() {
  // Set new font for headers
  for (int i = 0; i < 3; i++) {
    SendDlgItemMessage(IDC_STATIC_HEADER1 + i, WM_SETFONT, 
      reinterpret_cast<WPARAM>(m_Parent->m_hfHeader), FALSE);
  }
  
  // Initialize name
  m_Edit.Attach(GetDlgItem(IDC_EDIT_FEED_NAME));
  m_Edit.SetCueBannerText(L"Type something to identify this filter");
  m_Edit.SetText(m_Parent->m_Filter.Name);

  // Initialize condition list
  m_ListCondition.Attach(GetDlgItem(IDC_LIST_FEED_CONDITIONS));
  m_ListCondition.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  m_ListCondition.SetImageList(UI.ImgList16.GetHandle());
  m_ListCondition.SetTheme();
  // Insert list columns
  m_ListCondition.InsertColumn(0, 150, 150, 0, L"Element");
  m_ListCondition.InsertColumn(1, 150, 150, 0, L"Operator");
  m_ListCondition.InsertColumn(2, 175, 175, 0, L"Value");
  // Add conditions to list
  for (auto it = m_Parent->m_Filter.Conditions.begin(); it != m_Parent->m_Filter.Conditions.end(); ++it) {
    AddConditionToList(*it);
  }

  // Initialize toolbar
  m_Toolbar.Attach(GetDlgItem(IDC_TOOLBAR_FEED_FILTER));
  m_Toolbar.SetImageList(UI.ImgList16.GetHandle(), 16, 16);
  m_Toolbar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
  // Add toolbar items
  m_Toolbar.InsertButton(0, Icon16_Plus,      100, true,  0, 0, NULL, L"Add new condition...");
  m_Toolbar.InsertButton(1, Icon16_Minus,     101, false, 0, 1, NULL, L"Delete condition");
  m_Toolbar.InsertButton(2, 0, 0, 0, BTNS_SEP, NULL, NULL, NULL);
  m_Toolbar.InsertButton(3, Icon16_ArrowUp,   103, false, 0, 3, NULL, L"Move up");
  m_Toolbar.InsertButton(4, Icon16_ArrowDown, 104, false, 0, 4, NULL, L"Move down");
  
  // Initialize options
  m_ComboMatch.Attach(GetDlgItem(IDC_COMBO_FEED_FILTER_MATCH));
  m_ComboMatch.AddString(Aggregator.FilterManager.TranslateMatching(FEED_FILTER_MATCH_ALL).c_str());
  m_ComboMatch.AddString(Aggregator.FilterManager.TranslateMatching(FEED_FILTER_MATCH_ANY).c_str());
  m_ComboMatch.SetCurSel(m_Parent->m_Filter.Match);
  m_ComboAction.Attach(GetDlgItem(IDC_COMBO_FEED_FILTER_ACTION));
  m_ComboAction.AddString(Aggregator.FilterManager.TranslateAction(FEED_FILTER_ACTION_DISCARD).c_str());
  m_ComboAction.AddString(Aggregator.FilterManager.TranslateAction(FEED_FILTER_ACTION_SELECT).c_str());
  m_ComboAction.AddString(Aggregator.FilterManager.TranslateAction(FEED_FILTER_ACTION_PREFER).c_str());
  m_ComboAction.SetCurSel(m_Parent->m_Filter.Action);

  // Initialize anime list
  m_ListAnime.Attach(GetDlgItem(IDC_LIST_FEED_FILTER_ANIME));
  m_ListAnime.EnableGroupView(true);
  m_ListAnime.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
  m_ListAnime.SetImageList(UI.ImgList16.GetHandle());
  m_ListAnime.SetTheme();
  // Insert list columns
  m_ListAnime.InsertColumn(0, 460, 460, 0, L"Title");
  // Insert list groups
  for (int i = MAL_WATCHING; i <= MAL_PLANTOWATCH; i++) {
    if (i != MAL_UNKNOWN) {
      m_ListAnime.InsertGroup(i, MAL.TranslateMyStatus(i, false).c_str(), true, i != MAL_WATCHING);
    }
  }
  // Add anime to list
  for (int i = 1; i <= AnimeList.Count; i++) {
    m_ListAnime.InsertItem(i - 1, AnimeList.Item[i].GetStatus(), 
      StatusToIcon(AnimeList.Item[i].GetAiringStatus()), 
      0, NULL, LPSTR_TEXTCALLBACK, 
      reinterpret_cast<LPARAM>(&AnimeList.Item[i]));
    for (auto it = m_Parent->m_Filter.AnimeID.begin(); it != m_Parent->m_Filter.AnimeID.end(); ++it) {
      if (*it == AnimeList.Item[i].Series_ID) {
        m_ListAnime.SetCheckState(i - 1, TRUE);
        break;
      }
    }
  }
  m_ListAnime.Sort(0, 1, 0, ListViewCompareProc);

  return TRUE;
}

BOOL CFeedFilterWindow::CDialogPageAdvanced::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Add new condition
    case 100:
      FeedConditionWindow.Create(IDD_FEED_CONDITION, GetWindowHandle());
      if (FeedConditionWindow.m_Condition.Element > -1) {
        m_Parent->m_Filter.AddCondition(
          FeedConditionWindow.m_Condition.Element,
          FeedConditionWindow.m_Condition.Operator,
          FeedConditionWindow.m_Condition.Value);
        AddConditionToList(m_Parent->m_Filter.Conditions.back());
        m_ListCondition.SetSelectedItem(m_ListCondition.GetItemCount() - 1);
      }
      return TRUE;
    // Delete condition
    case 101: {
      int index = m_ListCondition.GetNextItem(-1, LVNI_SELECTED);
      if (index > -1) {
        m_ListCondition.DeleteItem(index);
        m_Parent->m_Filter.Conditions.erase(m_Parent->m_Filter.Conditions.begin() + index);
      }
      return TRUE;
    }
    // Move condition up
    case 103: {
      int index = m_ListCondition.GetNextItem(-1, LVNI_SELECTED);
      if (index > 0) {
        iter_swap(m_Parent->m_Filter.Conditions.begin() + index, 
          m_Parent->m_Filter.Conditions.begin() + index - 1);
        RefreshConditionsList();
        m_ListCondition.SetSelectedItem(index - 1);
      }
      return TRUE;
    }
    // Move condition down
    case 104:
      int index = m_ListCondition.GetNextItem(-1, LVNI_SELECTED);
      if (index > -1 && index < m_ListCondition.GetItemCount() - 1) {
        iter_swap(m_Parent->m_Filter.Conditions.begin() + index, 
          m_Parent->m_Filter.Conditions.begin() + index + 1);
        RefreshConditionsList();
        m_ListCondition.SetSelectedItem(index + 1);
      }
      return TRUE;
  }

  return FALSE;
}

LRESULT CFeedFilterWindow::CDialogPageAdvanced::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (idCtrl) {
    // Condition list
    case IDC_LIST_FEED_CONDITIONS:
      switch (pnmh->code) {
        // List item change
        case LVN_ITEMCHANGED: {
          int index = m_ListCondition.GetNextItem(-1, LVNI_SELECTED);
          int count = m_ListCondition.GetItemCount();
          m_Toolbar.EnableButton(1, index > -1);
          m_Toolbar.EnableButton(3, index > 0);
          m_Toolbar.EnableButton(4, index > -1 && index < count - 1);
          break;
        }
        // Key press
        case LVN_KEYDOWN: {
          LPNMLVKEYDOWN pnkd = reinterpret_cast<LPNMLVKEYDOWN>(pnmh);
          switch (pnkd->wVKey) {
            // Delete condition
            case VK_DELETE:
              OnCommand(MAKEWPARAM(101, 0), 0);
              break;
            // Move condition up
            case VK_SUBTRACT:
              OnCommand(MAKEWPARAM(103, 0), 0);
              break;
            // Move condition down
            case VK_ADD:
              OnCommand(MAKEWPARAM(104, 0), 0);
              break;
          }
          break;
        }
      }
      break;

    // Anime list
    case IDC_LIST_FEED_FILTER_ANIME:
      switch (pnmh->code) {
        // Text callback
        case LVN_GETDISPINFO: {
          NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(pnmh);
          CAnime* anime = reinterpret_cast<CAnime*>(plvdi->item.lParam);
          if (!anime) break;
          switch (plvdi->item.iSubItem) {
            case 0: // Anime title
              plvdi->item.pszText = const_cast<LPWSTR>(anime->Series_Title.data());
              break;
          }
          break;
        }
      }
      break;

    // Toolbar
    case IDC_TOOLBAR_FEED_FILTER:
      switch (pnmh->code) {
        // Show tooltip text
        case TBN_GETINFOTIP:
          NMTBGETINFOTIP* git = reinterpret_cast<NMTBGETINFOTIP*>(pnmh);
          git->cchTextMax = INFOTIPSIZE;
          git->pszText = (LPWSTR)(m_Toolbar.GetButtonTooltip(git->lParam));
          break;
      }
      break;
  }

  return 0;
}

bool CFeedFilterWindow::CDialogPageAdvanced::BuildFilter(CFeedFilter& filter) {
  m_Edit.GetText(filter.Name);
  filter.Match = m_ComboMatch.GetCurSel();
  filter.Action = m_ComboAction.GetCurSel();

  filter.AnimeID.clear();
  int count = m_ListAnime.GetItemCount();
  for (int i = 0; i < count; i++) {
    if (m_ListAnime.GetCheckState(i)) {
      CAnime* anime = reinterpret_cast<CAnime*>(m_ListAnime.GetItemParam(i));
      if (anime) filter.AnimeID.push_back(anime->Series_ID);
    }
  }
  
  return true;
}

void CFeedFilterWindow::CDialogPageAdvanced::AddConditionToList(const CFeedFilterCondition& condition, int index) {
  if (index == -1) index = m_ListCondition.GetItemCount();
  m_ListCondition.InsertItem(index, -1, Icon16_Funnel, 0, NULL, 
    Aggregator.FilterManager.TranslateElement(condition.Element).c_str(), NULL);
  m_ListCondition.SetItem(index, 1, Aggregator.FilterManager.TranslateOperator(condition.Operator).c_str());
  m_ListCondition.SetItem(index, 2, Aggregator.FilterManager.TranslateValue(condition).c_str());
}

void CFeedFilterWindow::CDialogPageAdvanced::RefreshConditionsList() {
  m_ListCondition.DeleteAllItems();
  for (auto it = m_Parent->m_Filter.Conditions.begin(); it != m_Parent->m_Filter.Conditions.end(); ++it) {
    AddConditionToList(*it);
  }
}