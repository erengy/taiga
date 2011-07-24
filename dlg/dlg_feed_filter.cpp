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
#include "dlg_feed_condition.h"
#include "dlg_feed_filter.h"
#include "../common.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"
#include "../win32/win_gdi.h"
#include "../win32/win_taskdialog.h"

CFeedFilterWindow FeedFilterWindow;

// =============================================================================

CFeedFilterWindow::CFeedFilterWindow() : 
  current_page_(0), icon_(nullptr), 
  header_font_(nullptr), main_instructions_font_(nullptr)
{
  RegisterDlgClass(L"TaigaFeedFilterW");
}

CFeedFilterWindow::~CFeedFilterWindow() {
  if (header_font_) {
    ::DeleteObject(header_font_);
    header_font_ = nullptr;
  }
  if (main_instructions_font_) {
    ::DeleteObject(main_instructions_font_);
    main_instructions_font_ = nullptr;
  }
  if (icon_) {
    ::DeleteObject(icon_);
    icon_ = nullptr;
  }
}

INT_PTR CFeedFilterWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Set main instruction text color
    case WM_CTLCOLORSTATIC: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      HWND hwnd_control = reinterpret_cast<HWND>(lParam);
      if (hwnd_control == GetDlgItem(IDC_STATIC_HEADER)) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0x00, 0x33, 0x99));
        return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
      }
      break;
    }

    // Forward mouse wheel messages to lists
    case WM_MOUSEWHEEL: {
      switch (current_page_) {
        case 0:
          return m_Page0.SendDlgItemMessage(
            IDC_LIST_FEED_FILTER_PRESETS, uMsg, wParam, lParam);
        case 1:
          return m_Page1.SendDlgItemMessage(
            IDC_LIST_FEED_FILTER_CONDITIONS, uMsg, wParam, lParam);
        case 2:
          return m_Page2.SendDlgItemMessage(
            IDC_LIST_FEED_FILTER_ANIME, uMsg, wParam, lParam);
      }
      break;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void CFeedFilterWindow::OnCancel() {
  filter_.Reset();
  EndDialog(IDCANCEL);
}

BOOL CFeedFilterWindow::OnInitDialog() {
  // Create main instructions font
  if (!main_instructions_font_) {
    LOGFONT lFont;
    HDC hdc = GetDC();
    ::GetObject(GetFont(), sizeof(LOGFONT), &lFont);
    lFont.lfHeight = -MulDiv(12, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    lFont.lfCharSet = DEFAULT_CHARSET;
    lstrcpy(lFont.lfFaceName, L"Segoe UI");
    main_instructions_font_ = ::CreateFontIndirect(&lFont);
    ReleaseDC(m_hWindow, hdc);
  }
  // Create bold header font
  if (!header_font_) {
    LOGFONT lFont;
    ::GetObject(GetFont(), sizeof(LOGFONT), &lFont);
    lFont.lfCharSet = DEFAULT_CHARSET;
    lFont.lfWeight = FW_BOLD;
    header_font_ = ::CreateFontIndirect(&lFont);
  }
  // Set icon
  if (!icon_) {
    icon_ = UI.ImgList16.GetIcon(Icon16_Funnel);
  }
  //SetIconSmall(icon_);

  // Set main instruction font
  main_instructions_label_.Attach(GetDlgItem(IDC_STATIC_HEADER));
  main_instructions_label_.SendMessage(WM_SETFONT, 
    reinterpret_cast<WPARAM>(main_instructions_font_), FALSE);
  
  // Calculate page area
  CRect page_area; main_instructions_label_.GetWindowRect(&page_area);
  MapWindowPoints(nullptr, m_hWindow, reinterpret_cast<LPPOINT>(&page_area), 2);
  page_area.top += page_area.bottom;
  page_area.left *= 2;

  // Create pages
  m_Page0.Create(IDD_FEED_FILTER_PAGE1, this, page_area);
  m_Page1.Create(IDD_FEED_FILTER_PAGE2, this, page_area);
  m_Page2.Create(IDD_FEED_FILTER_PAGE3, this, page_area);

  // Choose and display a page
  if (!filter_.Conditions.empty()) {
    SetText(L"Edit Filter");
    current_page_ = 1;
    ChoosePage(1);
  } else {
    ChoosePage(0);
  }

  return TRUE;
}

void CFeedFilterWindow::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  CDC dc = hdc;
  CRect rect;

  // Paint background
  GetClientRect(&rect);
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));

  // Paint bottom area
  CRect rect_button;
  ::GetClientRect(GetDlgItem(IDCANCEL), &rect_button);
  rect.top = rect.bottom - (rect_button.Height() * 2);
  dc.FillRect(rect, ::GetSysColor(COLOR_BTNFACE));
  
  // Paint line
  rect.bottom = rect.top + 1;
  dc.FillRect(rect, ::GetSysColor(COLOR_ACTIVEBORDER));
}

BOOL CFeedFilterWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Back
    case IDNO:
      ChoosePage(current_page_ - 1);
      return TRUE;
    // Next
    case IDYES:
      ChoosePage(current_page_ + 1);
      return TRUE;
  }

  return FALSE;
}

void CFeedFilterWindow::ChoosePage(int index) {
  switch (index) {
    // Page #0
    case 0:
      main_instructions_label_.SetText( 
        L"Choose one of the preset filters, or create a custom one");
      break;

    // Page #1
    case 1:
      if (current_page_ == 0) {
        m_Page0.BuildFilter(filter_);
      }
      m_Page1.name_text_.SetText(filter_.Name);
      m_Page1.RefreshConditionList();
      m_Page1.match_combo_.SetCurSel(filter_.Match);
      m_Page1.action_combo_.SetCurSel(filter_.Action);
      main_instructions_label_.SetText(
        L"Change filter options and add conditions");
      break;

    // Page #2
    case 2:
      if (current_page_ == 1) {
        if (m_Page1.condition_list_.GetItemCount() == 0) {
          CTaskDialog dlg(APP_TITLE, TD_ICON_ERROR);
          dlg.SetMainInstruction(
            L"There must be at least one condition in order to create a filter.");
          dlg.SetContent(
            L"You may add a condition using the Add New Condition button, "
            L"or choose a preset from the previous page.");
          dlg.AddButton(L"OK", IDOK);
          dlg.Show(GetWindowHandle());
          return;
        }
        m_Page1.BuildFilter(filter_);
      }
      main_instructions_label_.SetText(
        L"Limit this filter to one or more anime title, or leave it blank to apply to all items");
      break;

    // Finished
    case 3:
      if (current_page_ == 2) {
        m_Page2.BuildFilter(filter_);
      }
      EndDialog(IDOK);
      return;
  }

  m_Page0.Show(index == 0 ? 1 : 0);
  m_Page1.Show(index == 1 ? 1 : 0);
  m_Page2.Show(index == 2 ? 1 : 0);

  EnableDlgItem(IDNO, index > 0);
  SetDlgItemText(IDYES, index < 2 ? L"Next >" : L"Finish");

  current_page_ = index;
}

// =============================================================================

/* Page */

void CFeedFilterWindow::CDialogPage::Create(UINT uResourceID, CFeedFilterWindow* parent, const RECT& rect) {
  parent_ = parent;
  CDialog::Create(uResourceID, parent->GetWindowHandle(), false);
  SetPosition(nullptr, rect, SWP_NOSIZE);
}

INT_PTR CFeedFilterWindow::CDialogPage::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
      return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

// =============================================================================

/* Page 0 */

BOOL CFeedFilterWindow::CDialogPage0::OnInitDialog() {
  // Initialize presets list
  preset_list_.Attach(GetDlgItem(IDC_LIST_FEED_FILTER_PRESETS));
  preset_list_.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
  preset_list_.SetImageList(UI.ImgList16.GetHandle(), LVSIL_NORMAL);
  preset_list_.SetTheme();
  
  // Insert list groups
  //preset_list_.EnableGroupView(true);
  preset_list_.InsertGroup(0, L"Presets", true, false);
  
  // Insert list columns
  preset_list_.InsertColumn(0, 200, 200, 0, L"Title");
  preset_list_.InsertColumn(1, 350, 350, 0, L"Details");
  
  // Enable tile view
  SIZE size = {580, 40};
  RECT rect = {5, 0, 5, 0};
  preset_list_.SetTileViewInfo(1, LVTVIF_FIXEDSIZE, &rect, &size);
  preset_list_.SetView(LV_VIEW_TILE);

  // Insert presets
  for (auto it = Aggregator.FilterManager.Presets.begin(); 
       it != Aggregator.FilterManager.Presets.end(); ++it) {
    if (it->Default) continue;
    int icon = Icon16_Funnel;
    switch (it->Filter.Action) {
      case FEED_FILTER_ACTION_DISCARD: icon = Icon16_FunnelCross; break;
      case FEED_FILTER_ACTION_SELECT:  icon = Icon16_FunnelTick;  break;
      case FEED_FILTER_ACTION_PREFER:  icon = Icon16_FunnelPlus;  break;
    }
    if (it->Filter.Conditions.empty()) icon = Icon16_FunnelPencil;
    preset_list_.InsertItem(it - Aggregator.FilterManager.Presets.begin(), 
      0, icon, I_COLUMNSCALLBACK, nullptr, LPSTR_TEXTCALLBACK, 
      reinterpret_cast<LPARAM>(&(*it)));
  }
  preset_list_.SetSelectedItem(0);

  return TRUE;
}

LRESULT CFeedFilterWindow::CDialogPage0::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (idCtrl) {
    case IDC_LIST_FEED_FILTER_PRESETS:
      switch (pnmh->code) {
        // Text callback
        case LVN_GETDISPINFO: {
          NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(pnmh);
          if (plvdi->item.mask & LVIF_COLUMNS) {
            plvdi->item.cColumns = 1;
            plvdi->item.puColumns[0] = 1;
          }
          if (plvdi->item.mask & LVIF_TEXT) {
            CFeedFilterPreset* preset = reinterpret_cast<CFeedFilterPreset*>(plvdi->item.lParam);
            if (!preset) break;
            switch (plvdi->item.iSubItem) {
              case 0: // Filter name
                plvdi->item.pszText = const_cast<LPWSTR>(preset->Filter.Name.c_str());
                break;
              case 1: // Preset description
                plvdi->item.pszText = const_cast<LPWSTR>(preset->Description.c_str());
                break;
            }
          }
          return TRUE;
        }

        // Double click
        case NM_DBLCLK: {
          if (preset_list_.GetSelectedCount() > 0) {
            parent_->ChoosePage(1);
          }
          return TRUE;
        }
      }
      break;
  }

  return 0;
}

bool CFeedFilterWindow::CDialogPage0::BuildFilter(CFeedFilter& filter) {
  int selected_item = preset_list_.GetNextItem(-1, LVIS_SELECTED);
  
  if (selected_item > -1) {
    // Build filter from preset
    CFeedFilterPreset* preset = 
      reinterpret_cast<CFeedFilterPreset*>(preset_list_.GetItemParam(selected_item));
    if (!preset || preset->Filter.Conditions.empty()) {
      parent_->filter_.Reset();
      return false;
    }
    parent_->filter_ = preset->Filter;
    // Clear selection
    ListView_SetItemState(preset_list_.GetWindowHandle(), selected_item, 0, LVIS_SELECTED);
  }

  return true;
}

// =============================================================================

/* Page #1 */

BOOL CFeedFilterWindow::CDialogPage1::OnInitDialog() {
  // Set new font for headers
  for (int i = 0; i < 3; i++) {
    SendDlgItemMessage(IDC_STATIC_HEADER1 + i, WM_SETFONT, 
      reinterpret_cast<WPARAM>(parent_->header_font_), FALSE);
  }
  
  // Initialize name
  name_text_.Attach(GetDlgItem(IDC_EDIT_FEED_NAME));
  name_text_.SetCueBannerText(L"Type something to identify this filter");

  // Initialize condition list
  condition_list_.Attach(GetDlgItem(IDC_LIST_FEED_FILTER_CONDITIONS));
  condition_list_.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  condition_list_.SetImageList(UI.ImgList16.GetHandle());
  condition_list_.SetTheme();
  // Insert list columns
  condition_list_.InsertColumn(0, 170, 170, 0, L"Element");
  condition_list_.InsertColumn(1, 170, 170, 0, L"Operator");
  condition_list_.InsertColumn(2, 170, 170, 0, L"Value");

  // Initialize toolbar
  condition_toolbar_.Attach(GetDlgItem(IDC_TOOLBAR_FEED_FILTER));
  condition_toolbar_.SetImageList(UI.ImgList16.GetHandle(), 16, 16);
  condition_toolbar_.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
  // Add toolbar items
  condition_toolbar_.InsertButton(0, Icon16_Plus,      100, true,  0, 0, nullptr, L"Add new condition...");
  condition_toolbar_.InsertButton(1, Icon16_Minus,     101, false, 0, 1, nullptr, L"Delete condition");
  condition_toolbar_.InsertButton(2, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  condition_toolbar_.InsertButton(3, Icon16_ArrowUp,   103, false, 0, 3, nullptr, L"Move up");
  condition_toolbar_.InsertButton(4, Icon16_ArrowDown, 104, false, 0, 4, nullptr, L"Move down");
  
  // Initialize options
  match_combo_.Attach(GetDlgItem(IDC_COMBO_FEED_FILTER_MATCH));
  match_combo_.AddString(Aggregator.FilterManager.TranslateMatching(FEED_FILTER_MATCH_ALL).c_str());
  match_combo_.AddString(Aggregator.FilterManager.TranslateMatching(FEED_FILTER_MATCH_ANY).c_str());
  action_combo_.Attach(GetDlgItem(IDC_COMBO_FEED_FILTER_ACTION));
  action_combo_.AddString(Aggregator.FilterManager.TranslateAction(FEED_FILTER_ACTION_DISCARD).c_str());
  action_combo_.AddString(Aggregator.FilterManager.TranslateAction(FEED_FILTER_ACTION_SELECT).c_str());
  action_combo_.AddString(Aggregator.FilterManager.TranslateAction(FEED_FILTER_ACTION_PREFER).c_str());

  // Display current filter
  name_text_.SetText(parent_->filter_.Name);
  RefreshConditionList();
  match_combo_.SetCurSel(parent_->filter_.Match);
  action_combo_.SetCurSel(parent_->filter_.Action);

  return TRUE;
}

BOOL CFeedFilterWindow::CDialogPage1::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Add new condition
    case 100:
      FeedConditionWindow.condition_.Reset();
      FeedConditionWindow.Create(IDD_FEED_CONDITION, GetWindowHandle());
      if (FeedConditionWindow.condition_.Element > -1) {
        parent_->filter_.AddCondition(
          FeedConditionWindow.condition_.Element,
          FeedConditionWindow.condition_.Operator,
          FeedConditionWindow.condition_.Value);
        RefreshConditionList();
        condition_list_.SetSelectedItem(condition_list_.GetItemCount() - 1);
      }
      return TRUE;
    // Delete condition
    case 101: {
      int index = condition_list_.GetNextItem(-1, LVNI_SELECTED);
      if (index > -1) {
        condition_list_.DeleteItem(index);
        parent_->filter_.Conditions.erase(parent_->filter_.Conditions.begin() + index);
      }
      return TRUE;
    }
    // Move condition up
    case 103: {
      int index = condition_list_.GetNextItem(-1, LVNI_SELECTED);
      if (index > 0) {
        iter_swap(parent_->filter_.Conditions.begin() + index, 
          parent_->filter_.Conditions.begin() + index - 1);
        RefreshConditionList();
        condition_list_.SetSelectedItem(index - 1);
      }
      return TRUE;
    }
    // Move condition down
    case 104:
      int index = condition_list_.GetNextItem(-1, LVNI_SELECTED);
      if (index > -1 && index < condition_list_.GetItemCount() - 1) {
        iter_swap(parent_->filter_.Conditions.begin() + index, 
          parent_->filter_.Conditions.begin() + index + 1);
        RefreshConditionList();
        condition_list_.SetSelectedItem(index + 1);
      }
      return TRUE;
  }

  return FALSE;
}

LRESULT CFeedFilterWindow::CDialogPage1::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (idCtrl) {
    // Condition list
    case IDC_LIST_FEED_FILTER_CONDITIONS:
      switch (pnmh->code) {
        // List item change
        case LVN_ITEMCHANGED: {
          int index = condition_list_.GetNextItem(-1, LVNI_SELECTED);
          int count = condition_list_.GetItemCount();
          condition_toolbar_.EnableButton(1, index > -1);
          condition_toolbar_.EnableButton(3, index > 0);
          condition_toolbar_.EnableButton(4, index > -1 && index < count - 1);
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
        // Double click
        case NM_DBLCLK: {
          int selected_item = condition_list_.GetNextItem(-1, LVIS_SELECTED);
          if (selected_item == -1) break;
          CFeedFilterCondition* condition = reinterpret_cast<CFeedFilterCondition*>(
            condition_list_.GetItemParam(selected_item));
          if (condition) {
            FeedConditionWindow.condition_ = *condition;
            FeedConditionWindow.Create(IDD_FEED_CONDITION, GetWindowHandle());
            if (FeedConditionWindow.condition_.Element > -1) {
              *condition = FeedConditionWindow.condition_;
              RefreshConditionList();
              condition_list_.SetSelectedItem(selected_item);
            }
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
          git->pszText = (LPWSTR)(condition_toolbar_.GetButtonTooltip(git->lParam));
          break;
      }
      break;
  }

  return 0;
}

bool CFeedFilterWindow::CDialogPage1::BuildFilter(CFeedFilter& filter) {
  name_text_.GetText(filter.Name);
  filter.Match = match_combo_.GetCurSel();
  filter.Action = action_combo_.GetCurSel();
  
  return true;
}

void CFeedFilterWindow::CDialogPage1::AddConditionToList(const CFeedFilterCondition& condition, int index) {
  if (index == -1) index = condition_list_.GetItemCount();
  condition_list_.InsertItem(index, -1, Icon16_Funnel, 0, nullptr, 
    Aggregator.FilterManager.TranslateElement(condition.Element).c_str(), 
    reinterpret_cast<LPARAM>(&condition));
  condition_list_.SetItem(index, 1, Aggregator.FilterManager.TranslateOperator(condition.Operator).c_str());
  condition_list_.SetItem(index, 2, Aggregator.FilterManager.TranslateValue(condition).c_str());
}

void CFeedFilterWindow::CDialogPage1::RefreshConditionList() {
  condition_list_.DeleteAllItems();
  for (auto it = parent_->filter_.Conditions.begin(); it != parent_->filter_.Conditions.end(); ++it) {
    AddConditionToList(*it);
  }
}

// =============================================================================

/* Page 2 */

BOOL CFeedFilterWindow::CDialogPage2::OnInitDialog() {
  // Initialize anime list
  anime_list_.Attach(GetDlgItem(IDC_LIST_FEED_FILTER_ANIME));
  anime_list_.EnableGroupView(GetWinVersion() > WINVERSION_XP);
  anime_list_.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
  anime_list_.SetImageList(UI.ImgList16.GetHandle());
  anime_list_.SetTheme();
  
  // Insert list columns
  anime_list_.InsertColumn(0, 543, 543, 0, L"Title");
  
  // Insert list groups
  for (int i = MAL_WATCHING; i <= MAL_PLANTOWATCH; i++) {
    if (i != MAL_UNKNOWN) {
      anime_list_.InsertGroup(i, MAL.TranslateMyStatus(i, false).c_str(), true, i != MAL_WATCHING);
    }
  }
  
  // Add anime to list
  for (int i = 1; i <= AnimeList.Count; i++) {
    anime_list_.InsertItem(i - 1, AnimeList.Item[i].GetStatus(), 
      StatusToIcon(AnimeList.Item[i].GetAiringStatus()), 0, nullptr, 
      LPSTR_TEXTCALLBACK, reinterpret_cast<LPARAM>(&AnimeList.Item[i]));
    for (auto it = parent_->filter_.AnimeID.begin(); it != parent_->filter_.AnimeID.end(); ++it) {
      if (*it == AnimeList.Item[i].Series_ID) {
        anime_list_.SetCheckState(i - 1, TRUE);
        break;
      }
    }
  }
  
  // Sort items
  anime_list_.Sort(0, 1, 0, ListViewCompareProc);

  return TRUE;
}

LRESULT CFeedFilterWindow::CDialogPage2::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (idCtrl) {
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
        // Check/uncheck
        case LVN_ITEMCHANGED: {
          LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(pnmh);
          if (pnmv->uOldState != 0 && (pnmv->uNewState == 0x1000 || pnmv->uNewState == 0x2000)) {
            wstring text;
            for (int i = 0; i < anime_list_.GetItemCount(); i++) {
              CAnime* anime = reinterpret_cast<CAnime*>(anime_list_.GetItemParam(i));
              if (anime && anime_list_.GetCheckState(i)) {
                if (!text.empty()) text += L", ";
                text += anime->Series_Title;
              }
            }
            if (text.empty()) text = L"(nothing)";
            text = L"Currently limited to: " + text;
            SetDlgItemText(IDC_STATIC_FEED_FILTER_LIMIT, text.c_str());
          }
          break;
        }
      }
      break;
  }

  return 0;
}

bool CFeedFilterWindow::CDialogPage2::BuildFilter(CFeedFilter& filter) {
  filter.AnimeID.clear();

  for (int i = 0; i < anime_list_.GetItemCount(); i++) {
    if (anime_list_.GetCheckState(i)) {
      CAnime* anime = reinterpret_cast<CAnime*>(anime_list_.GetItemParam(i));
      if (anime) filter.AnimeID.push_back(anime->Series_ID);
    }
  }
  
  return true;
}