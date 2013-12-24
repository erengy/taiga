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

#include "api.h"
#include "dummy.h"
#include "base/common.h"
#include "base/logger.h"
#include "base/process.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/history.h"
#include "resource.h"
#include "settings.h"
#include "taiga.h"
#include "taiga/announce.h"
#include "track/media.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "version.h"
#include "win/win_taskbar.h"

HINSTANCE g_hInstance;
HWND g_hMain;

taiga::App Taiga;

namespace taiga {

App::App()
    : logged_in(false),
      current_tip_type(kTipTypeDefault),
      play_status(kPlayStatusStopped),
      ticker_media(0),
      ticker_memory(0),
      ticker_new_episodes(0),
      ticker_queue(0) {
  SetVersion(VERSION_MAJOR, VERSION_MINOR, VERSION_REVISION);
}

App::~App() {
  OleUninitialize();
}

BOOL App::InitInstance() {
  // Check another instance
  if (CheckInstance(L"Taiga-33d5a63c-de90-432f-9a8b-f6f733dab258",
                    L"TaigaMainW"))
    return FALSE;
  g_hInstance = GetInstanceHandle();

  // Initialize
  InitCommonControls(ICC_STANDARD_CLASSES);
  OleInitialize(nullptr);

  // Initialize logger
  Logger.SetOutputPath(AddTrailingSlash(GetPathOnly(GetModulePath())) +
                       APP_NAME L".log");
#ifdef _DEBUG
  Logger.SetSeverityLevel(LevelDebug);
#else
  Logger.SetSeverityLevel(LevelWarning);
#endif

  // Load data
  LoadData();

  DummyAnime.Initialize();
  DummyEpisode.Initialize();

  // Create API windows
  ::Skype.Create();
  TaigaApi.Create();

  if (Settings.GetBool(kApp_Behavior_CheckForUpdates)) {
    // Create update dialog
    ExecuteAction(L"CheckUpdates");
  } else {
    // Create main dialog
    ExecuteAction(L"MainDialog");
  }

  return TRUE;
}

void App::Uninitialize() {
  // Announce
  if (play_status == kPlayStatusPlaying) {
    play_status = kPlayStatusStopped;
    ::Announcer.Do(kAnnounceToHttp);
  }
  ::Announcer.Clear(kAnnounceToMessenger | kAnnounceToSkype);

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

void App::LoadData() {
  MediaPlayers.Load();

  if (Settings.Load())
    Settings.HandleCompatibility();

  ui::Theme.Load();
  ui::Menus.Load();

  AnimeDatabase.LoadDatabase();
  AnimeDatabase.LoadList();
  AnimeDatabase.ClearInvalidItems();

  History.Load();
}

}  // namespace taiga