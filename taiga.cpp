/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

#include "std.h"
#include "animelist.h"
#include "common.h"
#include "dlg/dlg_main.h"
#include "dlg/dlg_settings.h"
#include "gfx.h"
#include "http.h"
#include "media.h"
#include "myanimelist.h"
#include "process.h"
#include "recognition.h"
#include "resource.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "theme.h"
#include "win32/win_taskdialog.h"

HINSTANCE g_hInstance;
HWND g_hMain;
CTaiga Taiga;

// =============================================================================

CTaiga::CTaiga() : 
  LoggedIn(false), UpdatesEnabled(true), 
  CurrentTipType(TIPTYPE_NORMAL), PlayStatus(PLAYSTATUS_STOPPED), 
  TickerMedia(0), TickerNewEpisodes(0), TickerQueue(0), TickerTorrent(0)
{
}

CTaiga::~CTaiga() {
  Gdiplus_Shutdown();
  OleUninitialize();
}

// =============================================================================

BOOL CTaiga::InitInstance() {
  // Check another instance
  if (CheckInstance(L"Taiga-33d5a63c-de90-432f-9a8b-f6f733dab258", L"TaigaMainW"))
    return FALSE;
  g_hInstance = GetInstanceHandle();

  // Initialize
  InitCommonControls(ICC_STANDARD_CLASSES);
  Gdiplus_Initialize();
  OleInitialize(NULL);

  // Read data
  MediaPlayers.Read();
  Settings.Read();
  UI.Read(Settings.Program.General.Theme);
  UI.LoadImages();

  // Read anime list
  if (!Settings.Account.MAL.User.empty()) {
    AnimeList.Read();
  }

  // Create main window
  MainWindow.Create(IDD_MAIN, NULL, false);

  // Start-up settings
  if (Settings.Account.MAL.AutoLogin) {
    ExecuteAction(L"Login");
  }
  if (Settings.Program.StartUp.CheckNewEpisodes) {
    ExecuteAction(L"CheckNewEpisodes()", TRUE);
  }
  if (!Settings.Program.StartUp.Minimize) {
    MainWindow.Show();
  }
  if (Settings.Account.MAL.User.empty()) {
    CTaskDialog dlg;
    dlg.SetWindowTitle(APP_TITLE);
    dlg.SetMainIcon(TD_ICON_INFORMATION);
    dlg.SetMainInstruction(L"Welcome to Taiga!");
    dlg.SetContent(L"User name is not set. Would you like to open settings window to set it now?");
    dlg.AddButton(L"Yes", IDYES);
    dlg.AddButton(L"No", IDNO);
    dlg.Show(g_hMain);
    if (dlg.GetSelectedButtonID() == IDYES) {
      ExecuteAction(L"Settings", 0, PAGE_ACCOUNT);
    }
  }
  if (Settings.Program.StartUp.CheckNewVersion) {
    CheckNewVersion();
  }

  return TRUE;
}

// =============================================================================

BOOL CTaiga::CheckNewVersion() {
  return VersionClient.Get(L"dl.dropbox.com", 
    L"/u/2298899/Taiga/Version.txt", 
    L"", HTTP_VersionCheck);
}

wstring CTaiga::GetDataPath() {
  // Return current working directory in debug mode  
  #ifdef _DEBUG
  return CheckSlash(GetCurrentDirectory()) + L"Data\\";
  #endif
  
  // Return current path in portable mode
  #ifdef PORTABLE
  return CheckSlash(GetPathOnly(GetModulePath())) + L"Data\\";
  #endif
  
  // Return %APPDATA% folder
  WCHAR buffer[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, 
    NULL, SHGFP_TYPE_CURRENT, buffer))) {
      return CheckSlash(buffer) + APP_NAME + L"\\";
  }

  return L"";
}