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
#include "../common.h"
#include "dlg_main.h"
#include "dlg_update.h"
#include "../http.h"
#include "../resource.h"
#include "../taiga.h"
#include "../win32/win_gdi.h"
#include "../win32/win_taskdialog.h"

CUpdateDialog UpdateDialog;

// =============================================================================

CUpdateDialog::CUpdateDialog() :
  m_hfHeader(NULL)
{
  RegisterDlgClass(L"TaigaUpdateW");
}

BOOL CUpdateDialog::OnInitDialog() {
  // Set icon
  SetIconLarge(IDI_MAIN);
  SetIconSmall(IDI_MAIN);

  // Set title
  SetText(APP_TITLE);

  // Set header text and font
  SetDlgItemText(IDC_STATIC_UPDATE_TITLE, APP_NAME);
  if (!m_hfHeader) {
    LOGFONT lFont;
    ::GetObject(GetFont(), sizeof(LOGFONT), &lFont);
    CDC dc = GetDC();
    lFont.lfCharSet = DEFAULT_CHARSET;
    lFont.lfHeight = -MulDiv(14, GetDeviceCaps(dc.Get(), LOGPIXELSY), 72);
    lFont.lfWeight = FW_BOLD;
    m_hfHeader = ::CreateFontIndirect(&lFont);
  }
  SendDlgItemMessage(IDC_STATIC_UPDATE_TITLE, WM_SETFONT, 
    reinterpret_cast<WPARAM>(m_hfHeader), FALSE);

  // Set progress text
  SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS, L"Checking updates...");
  // Set progress bar
  m_ProgressBar.Attach(GetDlgItem(IDC_PROGRESS_UPDATE));
  m_ProgressBar.SetMarquee(true);

  // Check updates
  #ifdef _DEBUG
  PostMessage(WM_CLOSE);
  #else
  Taiga.UpdateHelper.Check(
    L"dl.dropbox.com/u/2298899/Taiga/update.xml", // TODO: Move to L"taiga.erengy.com/update.xml"
    Taiga, HTTP_UpdateCheck);
  #endif

  // Success
  return TRUE;
}

INT_PTR CUpdateDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC: {
      return ::GetSysColor(COLOR_WINDOW);
      break;
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL CUpdateDialog::OnDestroy() {
  // Clean up
  Taiga.UpdateHelper.Client.Cleanup();
  
  if (Taiga.UpdateHelper.UpdateAvailable) {
    // Restart application
    if (Taiga.UpdateHelper.RestartApplication(L"UpdateHelper.exe", L"Taiga.exe", L"Taiga.exe.!uh")) {
      return TRUE;
    } else {
      // Read data again
      Taiga.ReadData();
    }
  } else {
    if (g_hMain) {
      CTaskDialog dlg(APP_TITLE, TD_ICON_INFORMATION);
      dlg.SetMainInstruction(L"No updates available. Taiga is up to date!");
      dlg.SetExpandedInformation(L"Current version: " APP_VERSION);
      dlg.AddButton(L"OK", IDOK);
      dlg.Show(g_hMain);
    }
  }

  // Create/activate main window
  ExecuteAction(L"MainWindow");
  return TRUE;
}

void CUpdateDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  // Paint background
  CDC dc = hdc;
  dc.FillRect(lpps->rcPaint, ::GetSysColor(COLOR_WINDOW));
}