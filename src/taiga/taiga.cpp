/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include "base/log.h"
#include "base/process.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/history.h"
#include "taiga/announce.h"
#include "taiga/api.h"
#include "taiga/dummy.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "taiga/version.h"
#include "track/media.h"
#include "ui/dialog.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "win/win_taskbar.h"

taiga::App Taiga;

namespace taiga {

App::App()
    : debug_mode(false),
      logged_in(false),
      current_tip_type(kTipTypeDefault),
      play_status(kPlayStatusStopped) {
#ifdef _DEBUG
  debug_mode = true;
#endif

  version.major = TAIGA_VERSION_MAJOR;
  version.minor = TAIGA_VERSION_MINOR;
  version.patch = TAIGA_VERSION_PATCH;
  version.prerelease_identifiers = TAIGA_VERSION_PRE;
  if (TAIGA_VERSION_BUILD > 0)
    version.build_metadata = ToWstr(TAIGA_VERSION_BUILD);
}

App::~App() {
  OleUninitialize();
}

BOOL App::InitInstance() {
  // Check another instance
  if (CheckInstance(L"Taiga-33d5a63c-de90-432f-9a8b-f6f733dab258",
                    L"TaigaMainW"))
    return FALSE;

  // Initialize
  InitCommonControls(ICC_STANDARD_CLASSES);
  OleInitialize(nullptr);

  // Initialize logger
  Logger.SetOutputPath(AddTrailingSlash(GetPathOnly(GetModulePath())) +
                       TAIGA_APP_NAME L".log");
#ifdef _DEBUG
  Logger.SetSeverityLevel(LevelDebug);
#else
  Logger.SetSeverityLevel(LevelWarning);
#endif
  LOG(LevelInformational, L"Version " + std::wstring(version));

  // Parse command line
  ParseCommandLineArguments();

  // Load data
  LoadData();

  DummyAnime.Initialize();
  DummyEpisode.Initialize();

  // Create API windows
  ::Skype.Create();
  TaigaApi.Create();

  if (Settings.GetBool(kApp_Behavior_CheckForUpdates)) {
    ui::ShowDialog(ui::kDialogUpdate);
  } else {
    ui::ShowDialog(ui::kDialogMain);
  }

  return TRUE;
}

void App::Uninitialize() {
  // Announce
  if (play_status == kPlayStatusPlaying) {
    play_status = kPlayStatusStopped;
    ::Announcer.Do(kAnnounceToHttp);
  }
  ::Announcer.Clear(kAnnounceToSkype);

  // Cleanup
  ConnectionManager.Shutdown();
  Taskbar.Destroy();
  TaskbarList.Release();

  // Save
  Settings.Save();
  AnimeDatabase.SaveDatabase();
  Aggregator.SaveArchive();

  // Exit
  PostQuitMessage();
}

void App::ParseCommandLineArguments() {
  int argument_count = 0;
  LPWSTR* argument_list = CommandLineToArgvW(GetCommandLine(), &argument_count);

  if (!argument_count || !argument_list)
    return;

  for (int i = 1; i < argument_count; i++) {
    std::wstring argument = argument_list[i];

    if (argument == L"-debug" && !debug_mode) {
      debug_mode = true;
      Logger.SetSeverityLevel(LevelDebug);
      LOG(LevelDebug, argument);
    }
  }

  LocalFree(argument_list);
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