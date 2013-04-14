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

#include "std.h"

#include "taiga.h"

#include "anime_db.h"
#include "api.h"
#include "common.h"
#include "gfx.h"
#include "history.h"
#include "media.h"
#include "monitor.h"
#include "myanimelist.h"
#include "process.h"
#include "recognition.h"
#include "resource.h"
#include "settings.h"
#include "string.h"
#include "theme.h"
#include "version.h"

#include "dlg/dlg_update.h"

#include "win32/win_taskdialog.h"

HINSTANCE g_hInstance;
HWND g_hMain;
class Taiga Taiga;

// =============================================================================

Taiga::Taiga()
    : is_recognition_enabled(true), 
      logged_in(false), 
      current_tip_type(TIPTYPE_NORMAL), 
      play_status(PLAYSTATUS_STOPPED), 
      ticker_media(0), 
      ticker_new_episodes(0), 
      ticker_queue(0) {
  SetVersionInfo(VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
}

Taiga::~Taiga() {
  OleUninitialize();
}

// =============================================================================

BOOL Taiga::InitInstance() {
  // Check another instance
  if (CheckInstance(L"Taiga-33d5a63c-de90-432f-9a8b-f6f733dab258", L"TaigaMainW"))
    return FALSE;
  g_hInstance = GetInstanceHandle();

  // Initialize
  InitCommonControls(ICC_STANDARD_CLASSES);
  OleInitialize(NULL);

  // Load data
  LoadData();

  // Create API window
  TaigaApi.Create();
  
  if (Settings.Program.StartUp.check_new_version) {
    // Create update dialog
    ExecuteAction(L"CheckUpdates");
  } else {
    // Create main dialog
    ExecuteAction(L"MainDialog");
  }

  return TRUE;
}

// =============================================================================

wstring Taiga::GetDataPath() {
  // Return current working directory in debug mode
#ifdef _DEBUG
  return CheckSlash(GetCurrentDirectory()) + L"data\\";
#endif
  
  // Return current path in portable mode
#ifdef PORTABLE
  return CheckSlash(GetPathOnly(GetModulePath())) + L"data\\";
#endif
  
  // Return %APPDATA% folder
  WCHAR buffer[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, 
    NULL, SHGFP_TYPE_CURRENT, buffer))) {
      return CheckSlash(buffer) + APP_NAME + L"\\";
  }

  return L"";
}

void Taiga::LoadData() {
  // Load media player data
  MediaPlayers.Load();
  
  // Load settings
  Settings.Load();
  
  // Load theme data
  UI.Load(Settings.Program.General.theme);
  UI.LoadImages();

  // Load anime database
  AnimeDatabase.LoadDatabase();
  
  // Load anime list
  AnimeDatabase.LoadList();

  // Load history
  History.Load();
}

// =============================================================================

void Taiga::Updater::OnCheck() {
  //
}

void Taiga::Updater::OnCRCCheck(const wstring& path, wstring& crc) {
  wstring text = L"Checking file integrity... (" + GetFileName(path) + L")";
  UpdateDialog.SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS, text.c_str());
  crc = CalculateCRC(path);
  ToUpper(crc);
}

void Taiga::Updater::OnDone() {
  UpdateDialog.SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS, L"Done!");
}

void Taiga::Updater::OnProgress(int file_index) {
  wstring text = L"Downloading file... (" + files[file_index].path + L")";
  UpdateDialog.SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS, text.c_str());
}

bool Taiga::Updater::OnRestartApp() {
  if (g_hMain) {
    Settings.Save();
  }

  return true;
}

void Taiga::Updater::OnRunActions() {
  for (unsigned int i = 0; i < actions.size(); i++) {
    ExecuteAction(actions[i]);
  }
}