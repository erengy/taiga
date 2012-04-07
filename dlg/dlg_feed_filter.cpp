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

class FeedFilterDialog FeedFilterDialog;

// =============================================================================

FeedFilterDialog::FeedFilterDialog() : 
  current_page_(0), icon_(nullptr), 
  header_font_(nullptr), main_instructions_font_(nullptr)
{
  RegisterDlgClass(L"TaigaFeedFilterW");
}

FeedFilterDialog::~FeedFilterDialog() {
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

INT_PTR FeedFilterDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
          return page_0_.SendDlgItemMessage(
            IDC_LIST_FEED_FILTER_PRESETS, uMsg, wParam, lParam);
        case 1:
          return page_1_.SendDlgItemMessage(
            IDC_LIST_FEED_FILTER_CONDITIONS, uMsg, wParam, lParam);
        case 2:
          return page_2_.SendDlgItemMessage(
            IDC_LIST_FEED_FILTER_ANIME, uMsg, wParam, lParam);
      }
      break;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void FeedFilterDialog::OnCancel() {
  filter.Reset();
  EndDialog(IDCANCEL);
}

BOOL FeedFilterDialog::OnInitDialog() {
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
  // Set icon_
  if (!icon_) {
    icon_ = UI.ImgList16.GetIcon(ICON16_FUNNEL);
  }
  //SetIconSmall(icon_);

  // Set main instruction font
  main_instructions_label_.Attach(GetDlgItem(IDC_STATIC_HEADER));
  main_instructions_label_.SendMessage(WM_SETFONT, 
    reinterpret_cast<WPARAM>(main_instructions_font_), FALSE);
  
  // Calculate page area
  win32::Rect page_area; main_instructions_label_.GetWindowRect(&page_area);
  MapWindowPoints(nullptr, m_hWindow, reinterpret_cast<LPPOINT>(&page_area), 2);
  page_area.top += page_area.bottom;
  page_area.left *= 2;

  // Create pages
  page_0_.Create(IDD_FEED_FILTER_PAGE1, this, page_area);
  page_1_.Create(IDD_FEED_FILTER_PAGE2, this, page_area);
  page_2_.Create(IDD_FEED_FILTER_PAGE3, this, page_area);

  // Choose and display a page
  if (!filter.conditions.empty()) {
    SetText(L"Edit Filter");
    current_page_ = 1;
    ChoosePage(1);
  } else {
    ChoosePage(0);
  }

  return TRUE;
}

void FeedFilterDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  win32::Dc dc = hdc;
  win32::Rect rect;

  // Paint background
  GetClientRect(&rect);
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));

  // Paint bottom area
  win32::Rect rect_button;
  ::GetClientRect(GetDlgItem(IDCANCEL), &rect_button);
  rect.top = rect.bottom - (rect_button.Height() * 2);
  dc.FillRect(rect, ::GetSysColor(COLOR_BTNFACE));
  
  // Paint line
  rect.bottom = rect.top + 1;
  dc.FillRect(rect, ::GetSysColor(COLOR_ACTIVEBORDER));
}

BOOL FeedFilterDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
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

void FeedFilterDialog::ChoosePage(int index) {
  switch (index) {
    // Page #0
    case 0:
      main_instructions_label_.SetText( 
        L"Choose one of the preset filters, or create a custom one");
      break;

    // Page #1
    case 1:
      if (current_page_ == 0) {
        page_0_.BuildFilter(filter);
      }
      page_1_.name_text.SetText(filter.name);
      page_1_.RefreshConditionList();
      page_1_.match_combo.SetCurSel(filter.match);
      page_1_.action_combo.SetCurSel(filter.action);
      main_instructions_label_.SetText(
        L"Change filter options and add conditions");
      break;

    // Page #2
    case 2:
      if (current_page_ == 1) {
        if (page_1_.condition_list.GetItemCount() == 0) {
          win32::TaskDialog dlg(APP_TITLE, TD_ICON_ERROR);
          dlg.SetMainInstruction(
            L"There must be at least one condition in order to create a filter.");
          dlg.SetContent(
            L"You may add a condition using the Add New Condition button, "
            L"or choose a preset from the previous page.");
          dlg.AddButton(L"OK", IDOK);
          dlg.Show(GetWindowHandle());
          return;
        }
        page_1_.BuildFilter(filter);
      }
      main_instructions_label_.SetText(
        L"Limit this filter to one or more anime title, or leave it blank to apply to all items");
      break;

    // Finished
    case 3:
      if (current_page_ == 2) {
        page_2_.BuildFilter(filter);
      }
      EndDialog(IDOK);
      return;
  }

  page_0_.Show(index == 0 ? 1 : 0);
  page_1_.Show(index == 1 ? 1 : 0);
  page_2_.Show(index == 2 ? 1 : 0);

  EnableDlgItem(IDNO, index > 0);
  SetDlgItemText(IDYES, index < 2 ? L"Next >" : L"Finish");

  current_page_ = index;
}

// =============================================================================

/* Page */

void FeedFilterDialog::DialogPage::Create(UINT uResourceID, FeedFilterDialog* parent, const RECT& rect) {
  this->parent = parent;
  win32::Dialog::Create(uResourceID, parent->GetWindowHandle(), false);
  SetPosition(nullptr, rect, SWP_NOSIZE);
}

INT_PTR FeedFilterDialog::DialogPage::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
      return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

// =============================================================================

/* Page 0 */

BOOL FeedFilterDialog::DialogPage0::OnInitDialog() {
  // Initialize presets list
  preset_list.Attach(GetDlgItem(IDC_LIST_FEED_FILTER_PRESETS));
  preset_list.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
  preset_list.SetImageList(UI.ImgList16.GetHandle(), LVSIL_NORMAL);
  preset_list.SetTheme();
  
  // Insert list groups
  //preset_list.EnableGroupView(true);
  preset_list.InsertGroup(0, L"Presets", true, false);
  
  // Insert list columns
  preset_list.InsertColumn(0, 200, 200, 0, L"Title");
  preset_list.InsertColumn(1, 350, 350, 0, L"Details");
  
  // Enable tile view
  SIZE size = {580, 40};
  RECT rect = {5, 0, 5, 0};
  preset_list.SetTileViewInfo(1, LVTVIF_FIXEDSIZE, &rect, &size);
  preset_list.SetView(LV_VIEW_TILE);

  // Insert presets
  for (auto it = Aggregator.filter_manager.presets.begin(); 
       it != Aggregator.filter_manager.presets.end(); ++it) {
    if (it->is_default) continue;
    int icon_ = ICON16_FUNNEL;
    switch (it->filter.action) {
      case FEED_FILTER_ACTION_DISCARD: icon_ = ICON16_FUNNEL_CROSS; break;
      case FEED_FILTER_ACTION_SELECT:  icon_ = ICON16_FUNNEL_TICK;  break;
      case FEED_FILTER_ACTION_PREFER:  icon_ = ICON16_FUNNEL_PLUS;  break;
    }
    if (it->filter.conditions.empty()) icon_ = ICON16_FUNNEL_PENCIL;
    preset_list.InsertItem(it - Aggregator.filter_manager.presets.begin(), 
      0, icon_, I_COLUMNSCALLBACK, nullptr, LPSTR_TEXTCALLBACK, 
      reinterpret_cast<LPARAM>(&(*it)));
  }
  preset_list.SetSelectedItem(0);

  return TRUE;
}

LRESULT FeedFilterDialog::DialogPage0::OnNotify(int idCtrl, LPNMHDR pnmh) {
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
            FeedFilterPreset* preset = reinterpret_cast<FeedFilterPreset*>(plvdi->item.lParam);
            if (!preset) break;
            switch (plvdi->item.iSubItem) {
              case 0: // Filter name
                plvdi->item.pszText = const_cast<LPWSTR>(preset->filter.name.c_str());
                break;
              case 1: // Preset description
                plvdi->item.pszText = const_cast<LPWSTR>(preset->description.c_str());
                break;
            }
          }
          return TRUE;
        }

        // Double click
        case NM_DBLCLK: {
          if (preset_list.GetSelectedCount() > 0) {
            parent->ChoosePage(1);
          }
          return TRUE;
        }
      }
      break;
  }

  return 0;
}

bool FeedFilterDialog::DialogPage0::BuildFilter(FeedFilter& filter) {
  int selected_item = preset_list.GetNextItem(-1, LVIS_SELECTED);
  
  if (selected_item > -1) {
    // Build filter from preset
    FeedFilterPreset* preset = 
      reinterpret_cast<FeedFilterPreset*>(preset_list.GetItemParam(selected_item));
    if (!preset || preset->filter.conditions.empty()) {
      parent->filter.Reset();
      return false;
    }
    parent->filter = preset->filter;
    // Clear selection
    ListView_SetItemState(preset_list.GetWindowHandle(), selected_item, 0, LVIS_SELECTED);
  }

  return true;
}

// =============================================================================

/* Page #1 */

BOOL FeedFilterDialog::DialogPage1::OnInitDialog() {
  // Set new font for headers
  for (int i = 0; i < 3; i++) {
    SendDlgItemMessage(IDC_STATIC_HEADER1 + i, WM_SETFONT, 
      reinterpret_cast<WPARAM>(parent->header_font_), FALSE);
  }
  
  // Initialize name
  name_text.Attach(GetDlgItem(IDC_EDIT_FEED_NAME));
  name_text.SetCueBannerText(L"Type something to identify this filter");

  // Initialize condition list
  condition_list.Attach(GetDlgItem(IDC_LIST_FEED_FILTER_CONDITIONS));
  condition_list.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  condition_list.SetImageList(UI.ImgList16.GetHandle());
  condition_list.SetTheme();
  // Insert list columns
  condition_list.InsertColumn(0, 170, 170, 0, L"Element");
  condition_list.InsertColumn(1, 170, 170, 0, L"Operator");
  condition_list.InsertColumn(2, 170, 170, 0, L"Value");

  // Initialize toolbar
  condition_toolbar.Attach(GetDlgItem(IDC_TOOLBAR_FEED_FILTER));
  condition_toolbar.SetImageList(UI.ImgList16.GetHandle(), 16, 16);
  condition_toolbar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
  // Add toolbar items
  condition_toolbar.InsertButton(0, ICON16_PLUS,      100, true,  0, 0, nullptr, L"Add new condition...");
  condition_toolbar.InsertButton(1, ICON16_MINUS,     101, false, 0, 1, nullptr, L"Delete condition");
  condition_toolbar.InsertButton(2, 0, 0, 0, BTNS_SEP, 0, nullptr, nullptr);
  condition_toolbar.InsertButton(3, ICON16_ARROW_UP,   103, false, 0, 3, nullptr, L"Move up");
  condition_toolbar.InsertButton(4, ICON16_ARROW_DOWN, 104, false, 0, 4, nullptr, L"Move down");
  
  // Initialize options
  match_combo.Attach(GetDlgItem(IDC_COMBO_FEED_FILTER_MATCH));
  match_combo.AddString(Aggregator.filter_manager.TranslateMatching(FEED_FILTER_MATCH_ALL).c_str());
  match_combo.AddString(Aggregator.filter_manager.TranslateMatching(FEED_FILTER_MATCH_ANY).c_str());
  action_combo.Attach(GetDlgItem(IDC_COMBO_FEED_FILTER_ACTION));
  action_combo.AddString(Aggregator.filter_manager.TranslateAction(FEED_FILTER_ACTION_DISCARD).c_str());
  action_combo.AddString(Aggregator.filter_manager.TranslateAction(FEED_FILTER_ACTION_SELECT).c_str());
  action_combo.AddString(Aggregator.filter_manager.TranslateAction(FEED_FILTER_ACTION_PREFER).c_str());

  // Display current filter
  name_text.SetText(parent->filter.name);
  RefreshConditionList();
  match_combo.SetCurSel(parent->filter.match);
  action_combo.SetCurSel(parent->filter.action);

  return TRUE;
}

BOOL FeedFilterDialog::DialogPage1::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Add new condition
    case 100:
      FeedConditionDialog.condition.Reset();
      FeedConditionDialog.Create(IDD_FEED_CONDITION, GetWindowHandle());
      if (FeedConditionDialog.condition.element > -1) {
        parent->filter.AddCondition(
          FeedConditionDialog.condition.element,
          FeedConditionDialog.condition.op,
          FeedConditionDialog.condition.value);
        RefreshConditionList();
        condition_list.SetSelectedItem(condition_list.GetItemCount() - 1);
      }
      return TRUE;
    // Delete condition
    case 101: {
      int index = condition_list.GetNextItem(-1, LVNI_SELECTED);
      if (index > -1) {
        condition_list.DeleteItem(index);
        parent->filter.conditions.erase(parent->filter.conditions.begin() + index);
      }
      return TRUE;
    }
    // Move condition up
    case 103: {
      int index = condition_list.GetNextItem(-1, LVNI_SELECTED);
      if (index > 0) {
        iter_swap(parent->filter.conditions.begin() + index, 
          parent->filter.conditions.begin() + index - 1);
        RefreshConditionList();
        condition_list.SetSelectedItem(index - 1);
      }
      return TRUE;
    }
    // Move condition down
    case 104:
      int index = condition_list.GetNextItem(-1, LVNI_SELECTED);
      if (index > -1 && index < condition_list.GetItemCount() - 1) {
        iter_swap(parent->filter.conditions.begin() + index, 
          parent->filter.conditions.begin() + index + 1);
        RefreshConditionList();
        condition_list.SetSelectedItem(index + 1);
      }
      return TRUE;
  }

  return FALSE;
}

LRESULT FeedFilterDialog::DialogPage1::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (idCtrl) {
    // Condition list
    case IDC_LIST_FEED_FILTER_CONDITIONS:
      switch (pnmh->code) {
        // List item change
        case LVN_ITEMCHANGED: {
          int index = condition_list.GetNextItem(-1, LVNI_SELECTED);
          int count = condition_list.GetItemCount();
          condition_toolbar.EnableButton(1, index > -1);
          condition_toolbar.EnableButton(3, index > 0);
          condition_toolbar.EnableButton(4, index > -1 && index < count - 1);
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
          int selected_item = condition_list.GetNextItem(-1, LVIS_SELECTED);
          if (selected_item == -1) break;
          FeedFilterCondition* condition = reinterpret_cast<FeedFilterCondition*>(
            condition_list.GetItemParam(selected_item));
          if (condition) {
            FeedConditionDialog.condition = *condition;
            FeedConditionDialog.Create(IDD_FEED_CONDITION, GetWindowHandle());
            if (FeedConditionDialog.condition.element > -1) {
              *condition = FeedConditionDialog.condition;
              RefreshConditionList();
              condition_list.SetSelectedItem(selected_item);
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
          git->pszText = (LPWSTR)(condition_toolbar.GetButtonTooltip(git->lParam));
          break;
      }
      break;
  }

  return 0;
}

bool FeedFilterDialog::DialogPage1::BuildFilter(FeedFilter& filter) {
  name_text.GetText(filter.name);
  filter.match = match_combo.GetCurSel();
  filter.action = action_combo.GetCurSel();
  
  return true;
}

void FeedFilterDialog::DialogPage1::AddConditionToList(const FeedFilterCondition& condition, int index) {
  if (index == -1) index = condition_list.GetItemCount();
  condition_list.InsertItem(index, -1, ICON16_FUNNEL, 0, nullptr, 
    Aggregator.filter_manager.TranslateElement(condition.element).c_str(), 
    reinterpret_cast<LPARAM>(&condition));
  condition_list.SetItem(index, 1, Aggregator.filter_manager.TranslateOperator(condition.op).c_str());
  condition_list.SetItem(index, 2, Aggregator.filter_manager.TranslateValue(condition).c_str());
}

void FeedFilterDialog::DialogPage1::RefreshConditionList() {
  condition_list.DeleteAllItems();
  for (auto it = parent->filter.conditions.begin(); it != parent->filter.conditions.end(); ++it) {
    AddConditionToList(*it);
  }
}

// =============================================================================

/* Page 2 */

BOOL FeedFilterDialog::DialogPage2::OnInitDialog() {
  // Initialize anime list
  anime_list.Attach(GetDlgItem(IDC_LIST_FEED_FILTER_ANIME));
  anime_list.EnableGroupView(win32::GetWinVersion() > win32::VERSION_XP);
  anime_list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
  anime_list.SetImageList(UI.ImgList16.GetHandle());
  anime_list.SetTheme();
  
  // Insert list columns
  anime_list.InsertColumn(0, 543, 543, 0, L"Title");
  
  // Insert list groups
  for (int i = mal::MYSTATUS_WATCHING; i <= mal::MYSTATUS_PLANTOWATCH; i++) {
    if (i != mal::MYSTATUS_UNKNOWN) {
      anime_list.InsertGroup(i, mal::TranslateMyStatus(i, false).c_str(), true, i != mal::MYSTATUS_WATCHING);
    }
  }
  
  // Add anime to list
  for (int i = 1; i <= AnimeList.count; i++) {
    anime_list.InsertItem(i - 1, AnimeList.items[i].GetStatus(), 
      StatusToIcon(AnimeList.items[i].GetAiringStatus()), 0, nullptr, 
      LPSTR_TEXTCALLBACK, reinterpret_cast<LPARAM>(&AnimeList.items[i]));
    for (auto it = parent->filter.anime_ids.begin(); it != parent->filter.anime_ids.end(); ++it) {
      if (*it == AnimeList.items[i].series_id) {
        anime_list.SetCheckState(i - 1, TRUE);
        break;
      }
    }
  }
  
  // Sort items
  anime_list.Sort(0, 1, 0, ListViewCompareProc);

  return TRUE;
}

LRESULT FeedFilterDialog::DialogPage2::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (idCtrl) {
    // Anime list
    case IDC_LIST_FEED_FILTER_ANIME:
      switch (pnmh->code) {
        // Text callback
        case LVN_GETDISPINFO: {
          NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(pnmh);
          Anime* anime = reinterpret_cast<Anime*>(plvdi->item.lParam);
          if (!anime) break;
          switch (plvdi->item.iSubItem) {
            case 0: // Anime title
              plvdi->item.pszText = const_cast<LPWSTR>(anime->series_title.data());
              break;
          }
          break;
        }
        // Check/uncheck
        case LVN_ITEMCHANGED: {
          LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(pnmh);
          if (pnmv->uOldState != 0 && (pnmv->uNewState == 0x1000 || pnmv->uNewState == 0x2000)) {
            wstring text;
            for (int i = 0; i < anime_list.GetItemCount(); i++) {
              Anime* anime = reinterpret_cast<Anime*>(anime_list.GetItemParam(i));
              if (anime && anime_list.GetCheckState(i)) {
                AppendString(text, anime->series_title);
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

bool FeedFilterDialog::DialogPage2::BuildFilter(FeedFilter& filter) {
  filter.anime_ids.clear();

  for (int i = 0; i < anime_list.GetItemCount(); i++) {
    if (anime_list.GetCheckState(i)) {
      Anime* anime = reinterpret_cast<Anime*>(anime_list.GetItemParam(i));
      if (anime) filter.anime_ids.push_back(anime->series_id);
    }
  }
  
  return true;
}