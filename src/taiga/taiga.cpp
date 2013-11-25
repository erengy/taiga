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

#include "taiga.h"

#include "library/anime_db.h"
#include "taiga/announce.h"
#include "api.h"
#include "base/common.h"
#include "base/gfx.h"
#include "library/history.h"
#include "base/logger.h"
#include "track/media.h"
#include "track/monitor.h"
#include "sync/myanimelist.h"
#include "base/process.h"
#include "track/recognition.h"
#include "resource.h"
#include "settings.h"
#include "base/string.h"
#include "ui/theme.h"
#include "version.h"

#include "ui/dlg/dlg_update.h"

#include "win/win_taskbar.h"
#include "win/win_taskdialog.h"

HINSTANCE g_hInstance;
HWND g_hMain;
class Taiga Taiga;

// =============================================================================

Taiga::Taiga()
    : logged_in(false),
      current_tip_type(TIPTYPE_DEFAULT),
      play_status(PLAYSTATUS_STOPPED),
      ticker_media(0),
      ticker_memory(0),
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

  // Initialize logger
  Logger.SetOutputPath(AddTrailingSlash(GetPathOnly(GetModulePath())) + APP_NAME L".log");
#ifdef _DEBUG
  Logger.SetSeverityLevel(LevelDebug);
#else
  Logger.SetSeverityLevel(LevelWarning);
#endif

  // Load data
  LoadData();

  // Create API windows
  Skype.Create();
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

void Taiga::Uninitialize() {
  // Announce
  if (play_status == PLAYSTATUS_PLAYING) {
    play_status = PLAYSTATUS_STOPPED;
    Announcer.Do(ANNOUNCE_TO_HTTP);
  }
  Announcer.Clear(ANNOUNCE_TO_MESSENGER | ANNOUNCE_TO_SKYPE);

  // Cleanup
  Taskbar.Destroy();
  TaskbarList.Release();

  // Save
  Settings.Save();
  AnimeDatabase.SaveDatabase();
  Aggregator.SaveArchive();

  // Exit
  PostQuitMessage();
}

// =============================================================================

wstring Taiga::GetDataPath() {
  // Return current path in portable mode
#ifdef PORTABLE
  return AddTrailingSlash(GetPathOnly(GetModulePath())) + L"data\\";
#endif
  
  // Return %AppData% folder
  WCHAR buffer[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE,
                                NULL, SHGFP_TYPE_CURRENT, buffer))) {
    return AddTrailingSlash(buffer) + APP_NAME + L"\\";
  }

  return L"";
}

void Taiga::LoadData() {
  MediaPlayers.Load();

  if (Settings.Load())
    Settings.HandleCompatibility();

  UI.Load(Settings.Program.General.theme);
  UI.LoadImages();

  AnimeDatabase.LoadDatabase();
  AnimeDatabase.LoadList();
  AnimeDatabase.ClearInvalidItems();

  History.Load();
}