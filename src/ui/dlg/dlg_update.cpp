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

#include "base/std.h"

#include "dlg_main.h"
#include "dlg_update.h"

#include "base/common.h"
#include "taiga/http.h"
#include "taiga/resource.h"
#include "taiga/taiga.h"

#include "win/win_gdi.h"
#include "win/win_taskdialog.h"

class UpdateDialog UpdateDialog;

// =============================================================================

UpdateDialog::UpdateDialog() {
  RegisterDlgClass(L"TaigaUpdateW");
}

BOOL UpdateDialog::OnInitDialog() {
  // Set icon
  SetIconLarge(IDI_MAIN);
  SetIconSmall(IDI_MAIN);

  // Set title
  SetText(APP_TITLE);

  // Set progress text
  SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS, L"Checking updates...");
  // Set progress bar
  progressbar.Attach(GetDlgItem(IDC_PROGRESS_UPDATE));
  progressbar.SetMarquee(true);

  // Check updates
  Taiga.Updater.Check(Taiga);

  return TRUE;
}

INT_PTR UpdateDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC: {
      return ::GetSysColor(COLOR_WINDOW);
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL UpdateDialog::OnDestroy() {
  // Clean up
  Taiga.Updater.Cancel();

  if (Taiga.Updater.IsRestartRequired()) {
    if (MainDialog.IsWindow()) {
      MainDialog.PostMessage(WM_CLOSE);
    } else {
      Taiga.Uninitialize();
    }
  } else {
    // Create/activate main window
    ExecuteAction(L"MainDialog");
  }

  return TRUE;
}

void UpdateDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  // Paint background
  win::Dc dc = hdc;
  dc.FillRect(lpps->rcPaint, ::GetSysColor(COLOR_WINDOW));
}