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
#include "../common.h"
#include "dlg_filter.h"
#include "dlg_main.h"
#include "../resource.h"

class FilterDialog FilterDialog;

// =============================================================================

FilterDialog::FilterDialog() {
  RegisterDlgClass(L"TaigaFilterW");
}

BOOL FilterDialog::OnInitDialog() {
  RefreshFilters();
  return TRUE;
}

BOOL FilterDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  // Status
  if (LOWORD(wParam) >= IDC_CHECK_FILTER_STATUS1 && 
    LOWORD(wParam) <= IDC_CHECK_FILTER_STATUS3) {
      AnimeList.filters.status[2 - (IDC_CHECK_FILTER_STATUS3 - LOWORD(wParam))] = 
        IsDlgButtonChecked(LOWORD(wParam)) == TRUE;
  // Type
  } else if (LOWORD(wParam) >= IDC_CHECK_FILTER_TYPE1 && 
    LOWORD(wParam) <= IDC_CHECK_FILTER_TYPE6) {
      AnimeList.filters.type[5 - (IDC_CHECK_FILTER_TYPE6 - LOWORD(wParam))] = 
        IsDlgButtonChecked(LOWORD(wParam)) == TRUE;
  }
  
  UpdateFilterMenu();
  MainDialog.RefreshMenubar();
  MainDialog.RefreshList();
  
  return FALSE;
}

BOOL FilterDialog::PreTranslateMessage(MSG* pMsg) {
  if (pMsg->message == WM_KEYDOWN) {
    // Close window
    if (pMsg->wParam == VK_ESCAPE) {
      Destroy();
      return TRUE;
    }
  }
  return FALSE;
}

// =============================================================================

void FilterDialog::RefreshFilters() {
  if (!IsWindow()) return;

  // Status
  for (int i = 0; i < 3; i++) {
    CheckDlgButton(IDC_CHECK_FILTER_STATUS1 + i, AnimeList.filters.status[i]);
  }
  // Type
  for (int i = 0; i < 6; i++) {
    CheckDlgButton(IDC_CHECK_FILTER_TYPE1 + i, AnimeList.filters.type[i]);
  }
}