/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include <windows/win/task_dialog.h>

#include "base/gfx.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "taiga/config.h"
#include "taiga/resource.h"
#include "track/feed_filter.h"
#include "track/feed_filter_manager.h"
#include "track/feed_filter_util.h"
#include "ui/dlg/dlg_feed_condition.h"
#include "ui/dlg/dlg_feed_filter.h"
#include "ui/list.h"
#include "ui/theme.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace ui {

FeedFilterDialog DlgFeedFilter;

FeedFilterDialog::FeedFilterDialog()
    : current_page_(0),
      icon_(nullptr) {
  RegisterDlgClass(L"TaigaFeedFilterW");
}

FeedFilterDialog::~FeedFilterDialog() {
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
        SetTextColor(hdc, ui::kColorMainInstruction);
        return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
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
  filter = {};
  EndDialog(IDCANCEL);
}

BOOL FeedFilterDialog::OnInitDialog() {
  // Set icon_
  if (!icon_)
    icon_ = ui::Theme.GetImageList16().GetIcon(ui::kIcon16_Funnel);
  //SetIconSmall(icon_);

  // Set main instruction font
  main_instructions_label_.Attach(GetDlgItem(IDC_STATIC_HEADER));
  main_instructions_label_.SendMessage(WM_SETFONT,
      reinterpret_cast<WPARAM>(ui::Theme.GetHeaderFont()), FALSE);

  // Calculate page area
  win::Rect page_area; main_instructions_label_.GetWindowRect(&page_area);
  MapWindowPoints(nullptr, GetWindowHandle(), reinterpret_cast<LPPOINT>(&page_area), 2);
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
  win::Dc dc = hdc;
  win::Rect rect;

  // Paint background
  GetClientRect(&rect);
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));

  // Paint bottom area
  win::Rect rect_button;
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
      if (current_page_ == 0)
        page_0_.BuildFilter(filter);
      page_1_.name_text.SetText(filter.name);
      page_1_.RefreshConditionList();
      page_1_.match_combo.SetCurSel(filter.match);
      page_1_.action_combo.SetCurSel(filter.action);
      page_1_.option_combo.SetCurSel(filter.option);
      page_1_.ChangeAction();
      main_instructions_label_.SetText(
          L"Change filter options and add conditions");
      break;

    // Page #2
    case 2:
      if (current_page_ == 1) {
        if (page_1_.condition_list.GetItemCount() == 0) {
          win::TaskDialog dlg(TAIGA_APP_NAME, TD_ICON_ERROR);
          dlg.SetMainInstruction(
              L"There must be at least one condition in order to create a filter.");
          dlg.SetContent(
              L"You may add a condition using the Add New Condition button, "
              L"or by choosing a preset from the previous page.");
          dlg.AddButton(L"OK", IDOK);
          dlg.Show(GetWindowHandle());
          return;
        }
        page_1_.BuildFilter(filter);
      }
      main_instructions_label_.SetText(
          L"Limit this filter to one or more anime title, "
          L"or leave it blank to apply to all items");
      break;

    // Finished
    case 3:
      if (current_page_ == 2)
        page_2_.BuildFilter(filter);
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

////////////////////////////////////////////////////////////////////////////////

/* Page */

void FeedFilterDialog::DialogPage::Create(UINT uResourceID, FeedFilterDialog* parent, const RECT& rect) {
  this->parent = parent;
  win::Dialog::Create(uResourceID, parent->GetWindowHandle(), false);
  SetPosition(nullptr, rect, SWP_NOSIZE);
}

INT_PTR FeedFilterDialog::DialogPage::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
      win::Dc dc = reinterpret_cast<HDC>(wParam);
      dc.SetBkMode(TRANSPARENT);
      dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
      dc.DetachDc();
      return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////////////

/* Page 0 */

BOOL FeedFilterDialog::DialogPage0::OnInitDialog() {
  // Initialize presets list
  preset_list.Attach(GetDlgItem(IDC_LIST_FEED_FILTER_PRESETS));
  preset_list.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
  preset_list.SetImageList(ui::Theme.GetImageList16().GetHandle(), LVSIL_NORMAL);
  preset_list.SetTheme();

  // Insert list groups
  //preset_list.EnableGroupView(true);
  preset_list.InsertGroup(0, L"Presets", true, false);

  // Insert list columns
  preset_list.InsertColumn(0, ScaleX(200), ScaleX(200), 0, L"Title");
  preset_list.InsertColumn(1, ScaleX(350), ScaleX(350), 0, L"Details");

  // Enable tile view
  SIZE size = {660, 40};
  RECT rect = {5, 0, 5, 0};
  preset_list.SetTileViewInfo(1, LVTVIF_FIXEDSIZE, &rect, &size);
  preset_list.SetView(LV_VIEW_TILE);

  // Insert presets
  for (const auto& preset : track::feed_filter_manager.GetPresets()) {
    if (preset.is_default)
      continue;
    int icon_ = ui::kIcon16_Funnel;
    switch (preset.filter.action) {
      case track::kFeedFilterActionDiscard: icon_ = ui::kIcon16_FunnelCross; break;
      case track::kFeedFilterActionSelect:  icon_ = ui::kIcon16_FunnelTick;  break;
      case track::kFeedFilterActionPrefer:  icon_ = ui::kIcon16_FunnelPlus;  break;
    }
    if (preset.filter.conditions.empty())
      icon_ = ui::kIcon16_FunnelPencil;
    preset_list.InsertItem(preset_list.GetItemCount(),
                           0, icon_, I_COLUMNSCALLBACK, nullptr, LPSTR_TEXTCALLBACK,
                           reinterpret_cast<LPARAM>(&preset));
  }
  preset_list.SelectItem(0);

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
            track::FeedFilterPreset* preset = reinterpret_cast<track::FeedFilterPreset*>(plvdi->item.lParam);
            if (!preset) break;
            switch (plvdi->item.iSubItem) {
              case 0:  // Filter name
                plvdi->item.pszText = const_cast<LPWSTR>(preset->filter.name.c_str());
                break;
              case 1:  // Preset description
                plvdi->item.pszText = const_cast<LPWSTR>(preset->description.c_str());
                break;
            }
          }
          return TRUE;
        }

        // Double click
        case NM_DBLCLK: {
          if (preset_list.GetSelectedCount() > 0)
            parent->ChoosePage(1);
          return TRUE;
        }
      }
      break;
  }

  return 0;
}

bool FeedFilterDialog::DialogPage0::BuildFilter(track::FeedFilter& filter) {
  int selected_item = preset_list.GetNextItem(-1, LVIS_SELECTED);

  if (selected_item > -1) {
    // Build filter from preset
    auto preset = reinterpret_cast<track::FeedFilterPreset*>(
        preset_list.GetItemParam(selected_item));
    if (!preset || preset->filter.conditions.empty()) {
      parent->filter = {};
      return false;
    }
    parent->filter = preset->filter;
    // Clear selection
    ListView_SetItemState(preset_list.GetWindowHandle(), selected_item, 0, LVIS_SELECTED);
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

/* Page #1 */

BOOL FeedFilterDialog::DialogPage1::OnInitDialog() {
  // Set new font for headers
  for (int i = 0; i < 3; i++)
    SendDlgItemMessage(IDC_STATIC_HEADER1 + i, WM_SETFONT,
                       reinterpret_cast<WPARAM>(ui::Theme.GetBoldFont()), FALSE);

  // Initialize name
  name_text.Attach(GetDlgItem(IDC_EDIT_FEED_NAME));
  name_text.SetCueBannerText(L"Type something to identify this filter");

  // Initialize condition list
  condition_list.Attach(GetDlgItem(IDC_LIST_FEED_FILTER_CONDITIONS));
  condition_list.SetExtendedStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  condition_list.SetImageList(ui::Theme.GetImageList16().GetHandle());
  condition_list.SetTheme();
  // Insert list columns
  condition_list.InsertColumn(0, ScaleX(170), ScaleX(170), 0, L"Element");
  condition_list.InsertColumn(1, ScaleX(170), ScaleX(170), 0, L"Operator");
  condition_list.InsertColumn(2, ScaleX(170), ScaleX(170), 0, L"Value");
  condition_list.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);

  // Initialize toolbar
  condition_toolbar.Attach(GetDlgItem(IDC_TOOLBAR_FEED_FILTER));
  condition_toolbar.SetImageList(ui::Theme.GetImageList16().GetHandle(), ScaleX(16), ScaleY(16));
  condition_toolbar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_MIXEDBUTTONS);
  // Add toolbar items
  BYTE fsState1 = TBSTATE_ENABLED | TBSTATE_WRAP;
  BYTE fsState2 = TBSTATE_INDETERMINATE | TBSTATE_WRAP;
  condition_toolbar.InsertButton(0, ui::kIcon16_Plus,      100, fsState1, 0, 0, nullptr, L"Add new condition...");
  condition_toolbar.InsertButton(1, ui::kIcon16_Minus,     101, fsState2, 0, 1, nullptr, L"Delete condition");
  condition_toolbar.InsertButton(2, 0, 0, TBSTATE_WRAP, BTNS_SEP, 0, nullptr, nullptr);
  condition_toolbar.InsertButton(3, ui::kIcon16_ArrowUp,   103, fsState2, 0, 3, nullptr, L"Move up");
  condition_toolbar.InsertButton(4, ui::kIcon16_ArrowDown, 104, fsState2, 0, 4, nullptr, L"Move down");

  // Initialize options
  match_combo.Attach(GetDlgItem(IDC_COMBO_FEED_FILTER_MATCH));
  match_combo.AddString(track::util::TranslateMatching(track::kFeedFilterMatchAll).c_str());
  match_combo.AddString(track::util::TranslateMatching(track::kFeedFilterMatchAny).c_str());
  action_combo.Attach(GetDlgItem(IDC_COMBO_FEED_FILTER_ACTION));
  action_combo.AddString(track::util::TranslateAction(track::kFeedFilterActionDiscard).c_str());
  action_combo.AddString(track::util::TranslateAction(track::kFeedFilterActionSelect).c_str());
  action_combo.AddString(track::util::TranslateAction(track::kFeedFilterActionPrefer).c_str());
  option_combo.Attach(GetDlgItem(IDC_COMBO_FEED_FILTER_OPTION));
  option_combo.AddString(track::util::TranslateOption(track::kFeedFilterOptionDefault).c_str());
  option_combo.AddString(track::util::TranslateOption(track::kFeedFilterOptionDeactivate).c_str());
  option_combo.AddString(track::util::TranslateOption(track::kFeedFilterOptionHide).c_str());

  // Display current filter
  name_text.SetText(parent->filter.name);
  RefreshConditionList();
  match_combo.SetCurSel(parent->filter.match);
  action_combo.SetCurSel(parent->filter.action);
  option_combo.SetCurSel(parent->filter.option);
  ChangeAction();

  return TRUE;
}

BOOL FeedFilterDialog::DialogPage1::OnCommand(WPARAM wParam, LPARAM lParam) {
  switch (LOWORD(wParam)) {
    // Add new condition
    case 100: {
      DlgFeedCondition.condition = {};
      DlgFeedCondition.Create(IDD_FEED_CONDITION, GetWindowHandle());
      if (DlgFeedCondition.condition.element != track::kFeedFilterElement_None) {
        parent->filter.conditions.push_back({
            DlgFeedCondition.condition.element,
            DlgFeedCondition.condition.op,
            DlgFeedCondition.condition.value});
        RefreshConditionList();
        condition_list.SelectItem(condition_list.GetItemCount() - 1);
      }
      return TRUE;
    }
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
        condition_list.SelectItem(index - 1);
      }
      return TRUE;
    }
    // Move condition down
    case 104: {
      int index = condition_list.GetNextItem(-1, LVNI_SELECTED);
      if (index > -1 && index < condition_list.GetItemCount() - 1) {
        iter_swap(parent->filter.conditions.begin() + index,
                  parent->filter.conditions.begin() + index + 1);
        RefreshConditionList();
        condition_list.SelectItem(index + 1);
      }
      return TRUE;
    }

    // Change action
    case IDC_COMBO_FEED_FILTER_ACTION: {
      if (HIWORD(wParam) == CBN_SELENDOK) {
        ChangeAction();
        return TRUE;
      }
      break;
    }
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
          condition_toolbar.EnableButton(101, index > -1);
          condition_toolbar.EnableButton(103, index > 0);
          condition_toolbar.EnableButton(104, index > -1 && index < count - 1);
          break;
        }
        // Key press
        case LVN_KEYDOWN: {
          auto pnkd = reinterpret_cast<LPNMLVKEYDOWN>(pnmh);
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
          if (selected_item == -1)
            break;
          auto condition = reinterpret_cast<track::FeedFilterCondition*>(
              condition_list.GetItemParam(selected_item));
          if (condition) {
            DlgFeedCondition.condition = *condition;
            DlgFeedCondition.Create(IDD_FEED_CONDITION, GetWindowHandle());
            if (DlgFeedCondition.condition.element != track::kFeedFilterElement_None) {
              *condition = DlgFeedCondition.condition;
              RefreshConditionList();
              condition_list.SelectItem(selected_item);
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
          auto git = reinterpret_cast<NMTBGETINFOTIP*>(pnmh);
          git->cchTextMax = INFOTIPSIZE;
          git->pszText = (LPWSTR)(condition_toolbar.GetButtonTooltip(git->lParam));
          break;
      }
      break;
  }

  return 0;
}

bool FeedFilterDialog::DialogPage1::BuildFilter(track::FeedFilter& filter) {
  name_text.GetText(filter.name);
  filter.match = static_cast<track::FeedFilterMatch>(match_combo.GetCurSel());
  filter.action = static_cast<track::FeedFilterAction>(action_combo.GetCurSel());
  filter.option = static_cast<track::FeedFilterOption>(option_combo.GetCurSel());

  return true;
}

void FeedFilterDialog::DialogPage1::AddConditionToList(const track::FeedFilterCondition& condition, int index) {
  if (index == -1)
    index = condition_list.GetItemCount();

  condition_list.InsertItem(index, -1, ui::kIcon16_Funnel, 0, nullptr,
                            track::util::TranslateElement(condition.element).c_str(),
                            reinterpret_cast<LPARAM>(&condition));
  condition_list.SetItem(index, 1, track::util::TranslateOperator(condition.op).c_str());
  condition_list.SetItem(index, 2, track::util::TranslateValue(condition).c_str());
}

void FeedFilterDialog::DialogPage1::RefreshConditionList() {
  condition_list.DeleteAllItems();

  for (const auto& condition : parent->filter.conditions)
    AddConditionToList(condition);
}

void FeedFilterDialog::DialogPage1::ChangeAction() {
  const bool enabled = [this]() {
    switch (action_combo.GetCurSel()) {
      case track::kFeedFilterActionDiscard:
      case track::kFeedFilterActionPrefer:
        return true;
      default:
        return false;
    }
  }();

  if (!enabled)
    option_combo.SetCurSel(track::kFeedFilterOptionDefault);
  option_combo.Enable(enabled);
  option_combo.Show(enabled);

  win::Window label = GetDlgItem(IDC_STATIC_FEED_FILTER_DISCARDTYPE);
  label.Show(enabled);
  label.SetWindowHandle(nullptr);
}

////////////////////////////////////////////////////////////////////////////////

/* Page 2 */

BOOL FeedFilterDialog::DialogPage2::OnInitDialog() {
  // Initialize anime list
  anime_list.Attach(GetDlgItem(IDC_LIST_FEED_FILTER_ANIME));
  anime_list.EnableGroupView(true);
  anime_list.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
  anime_list.SetImageList(ui::Theme.GetImageList16().GetHandle());
  anime_list.SetTheme();

  // Insert list columns
  anime_list.InsertColumn(0, 0, 0, 0, L"Title");

  // Insert list groups
  for (const auto status : anime::kMyStatuses) {
    anime_list.InsertGroup(static_cast<int>(status),
                           ui::TranslateMyStatus(status, false).c_str(), true,
                           status != anime::MyStatus::Watching);
  }

  // Add anime to list
  int list_index = 0;
  for (const auto& [id, anime_item] : anime::db.items) {
    if (!anime_item.IsInList())
      continue;
    anime_list.InsertItem(list_index, static_cast<int>(anime_item.GetMyStatus()),
                          StatusToIcon(anime_item.GetAiringStatus()), 0, nullptr,
                          LPSTR_TEXTCALLBACK, static_cast<int>(anime_item.GetId()));
    for (const auto& anime_id : parent->filter.anime_ids) {
      if (anime_id == anime_item.GetId()) {
        anime_list.SetCheckState(list_index, TRUE);
        break;
      }
    }
    ++list_index;
  }

  // Sort items
  anime_list.Sort(0, 1, 0, ui::ListViewCompareProc);

  // Resize header
  anime_list.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);

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
          auto anime_item = anime::db.Find(static_cast<int>(plvdi->item.lParam));
          if (!anime_item)
            break;
          switch (plvdi->item.iSubItem) {
            case 0:  // Anime title
              plvdi->item.pszText = const_cast<LPWSTR>(anime::GetPreferredTitle(*anime_item).data());
              break;
          }
          break;
        }
        // Check/uncheck
        case LVN_ITEMCHANGED: {
          LPNMLISTVIEW pnmv = reinterpret_cast<LPNMLISTVIEW>(pnmh);
          if (pnmv->uOldState != 0 && (pnmv->uNewState == 0x1000 || pnmv->uNewState == 0x2000)) {
            std::wstring text;
            for (int i = 0; i < anime_list.GetItemCount(); i++) {
              auto anime_item = anime::db.Find(static_cast<int>(anime_list.GetItemParam(i)));
              if (anime_item && anime_list.GetCheckState(i))
                AppendString(text, anime::GetPreferredTitle(*anime_item));
            }
            if (text.empty())
              text = L"(nothing)";
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

bool FeedFilterDialog::DialogPage2::BuildFilter(track::FeedFilter& filter) {
  filter.anime_ids.clear();

  for (int i = 0; i < anime_list.GetItemCount(); i++) {
    if (anime_list.GetCheckState(i)) {
      auto anime_item = anime::db.Find(static_cast<int>(anime_list.GetItemParam(i)));
      if (anime_item)
        filter.anime_ids.push_back(anime_item->GetId());
    }
  }

  return true;
}

}  // namespace ui
