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

#include "base/gfx.h"
#include "taiga/app.h"
#include "taiga/config.h"
#include "taiga/resource.h"
#include "taiga/update.h"
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
  SetText(TAIGA_APP_NAME);

  // Set progress text
  SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS, L"Checking updates...");
  // Set progress bar
  progressbar.Attach(GetDlgItem(IDC_PROGRESS_UPDATE));
  progressbar.SetMarquee(true);

  // Check updates
  taiga::updater.Check();

  return TRUE;
}

INT_PTR UpdateDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
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

BOOL UpdateDialog::OnClose() {
  taiga::updater.Cancel();

  return FALSE;
}

BOOL UpdateDialog::OnDestroy() {
  if (taiga::updater.IsRestartRequired()) {
    if (DlgMain.IsWindow()) {
      DlgMain.PostMessage(WM_DESTROY);
    } else {
      taiga::app.Uninitialize();
    }
  } else {
    // Create/activate main window
    ShowDialog(ui::Dialog::Main);
  }

  return TRUE;
}

void UpdateDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  win::Dc dc = hdc;
  win::Rect rect;

  if (!lpps)
    return;

  // Paint background
  dc.FillRect(lpps->rcPaint, ::GetSysColor(COLOR_WINDOW));

  // Paint application icon
  win::Window label = GetDlgItem(IDC_STATIC_APP_ICON);
  label.GetWindowRect(GetWindowHandle(), &rect);
  label.SetWindowHandle(nullptr);
  DrawIconResource(IDI_MAIN, dc.Get(), rect, true, true);
}

}  // namespace ui
