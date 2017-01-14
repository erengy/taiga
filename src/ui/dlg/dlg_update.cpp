/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include "base/gfx.h"
#include "taiga/resource.h"
#include "taiga/taiga.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_update.h"
#include "ui/dialog.h"
#include "ui/theme.h"

namespace ui {

UpdateDialog DlgUpdate;

UpdateDialog::UpdateDialog() {
  RegisterDlgClass(L"TaigaUpdateW");
}

BOOL UpdateDialog::OnInitDialog() {
  // Set icon
  SetIconLarge(IDI_MAIN);
  SetIconSmall(IDI_MAIN);

  // Create default fonts
  Theme.CreateFonts(GetDC());

  // Set title
  SetText(TAIGA_APP_TITLE);

  // Set progress text
  SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS, L"Checking updates...");
  // Set progress bar
  progressbar.Attach(GetDlgItem(IDC_PROGRESS_UPDATE));
  progressbar.SetMarquee(true);

  // Check updates
  Taiga.Updater.Check();

  return TRUE;
}

INT_PTR UpdateDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC: {
      win::Dc dc = reinterpret_cast<HDC>(wParam);
      dc.SetBkMode(TRANSPARENT);
      dc.DetachDc();
      return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL UpdateDialog::OnDestroy() {
  // Clean up
  Taiga.Updater.Cancel();

  if (Taiga.Updater.IsRestartRequired()) {
    if (DlgMain.IsWindow()) {
      DlgMain.PostMessage(WM_DESTROY);
    } else {
      Taiga.Uninitialize();
    }
  } else {
    // Create/activate main window
    ShowDialog(kDialogMain);
  }

  return TRUE;
}

void UpdateDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  // Paint background
  win::Dc dc = hdc;
  dc.FillRect(lpps->rcPaint, ::GetSysColor(COLOR_WINDOW));

  // Paint application icon
  win::Rect rect;
  win::Window label = GetDlgItem(IDC_STATIC_APP_ICON);
  label.GetWindowRect(GetWindowHandle(), &rect);
  label.SetWindowHandle(nullptr);
  DrawIconResource(IDI_MAIN, dc.Get(), rect, true, true);
}

}  // namespace ui
