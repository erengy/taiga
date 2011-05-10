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

#include "std.h"
#include "animelist.h"
#include "common.h"
#include "dlg/dlg_update.h"
#include "gfx.h"
#include "media.h"
#include "monitor.h"
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
  TickerMedia(0), TickerNewEpisodes(0), TickerQueue(0)
{
  SetVersionInfo(APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_REVISION);
}

CTaiga::~CTaiga() {
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
  OleInitialize(NULL);

  // Read data
  ReadData();
  
  if (Settings.Program.StartUp.CheckNewVersion) {
    // Create update dialog
    ExecuteAction(L"CheckUpdates");
  } else {
    // Create main dialog
    ExecuteAction(L"MainWindow");
  }

  return TRUE;
}

// =============================================================================

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

void CTaiga::ReadData() {
  // Read media player data
  MediaPlayers.Read();
  
  // Read settings
  Settings.Read();
  
  // Read theme data
  UI.Read(Settings.Program.General.Theme);
  UI.LoadImages();
  
  // Read anime list
  AnimeList.Read();
}

// =============================================================================

void CTaiga::CUpdate::OnCheck() {
  //
}

void CTaiga::CUpdate::OnCRCCheck(const wstring& path, wstring& crc) {
  wstring text = L"Checking file integrity... (" + GetFileName(path) + L")";
  UpdateDialog.SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS, text.c_str());
  crc = CalculateCRC(path);
  ToUpper(crc);
}

void CTaiga::CUpdate::OnDone() {
  UpdateDialog.SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS, L"Done!");
}

void CTaiga::CUpdate::OnProgress(int file_index) {
  wstring text = L"Downloading file... (" + Files[file_index].Path + L")";
  UpdateDialog.SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS, text.c_str());
}

bool CTaiga::CUpdate::OnRestartApp() {
  if (g_hMain) {
    Settings.Write();
  }

  return true;
}

void CTaiga::CUpdate::OnRunActions() {
  for (unsigned int i = 0; i < Actions.size(); i++) {
    ExecuteAction(Actions[i]);
  }
}