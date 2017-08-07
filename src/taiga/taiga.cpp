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

#include <windows/win/taskbar.h>

#include "base/file.h"
#include "base/log.h"
#include "base/process.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/history.h"
#include "taiga/announce.h"
#include "taiga/dummy.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "taiga/version.h"
#include "track/media.h"
#include "ui/dialog.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"

taiga::App Taiga;

namespace taiga {

App::App()
    : allow_multiple_instances(false),
#ifdef _DEBUG
      debug_mode(true)
#else
      debug_mode(false)
#endif
{
  version.major = TAIGA_VERSION_MAJOR;
  version.minor = TAIGA_VERSION_MINOR;
  version.patch = TAIGA_VERSION_PATCH;
  version.prerelease_identifiers = TAIGA_VERSION_PRE;
  if (TAIGA_VERSION_BUILD > 0)
    version.build_metadata = ToStr(TAIGA_VERSION_BUILD);
}

App::~App() {
  OleUninitialize();
}

BOOL App::InitInstance() {
  // Parse command line
  ParseCommandLineArguments();

  // Initialize logger
  auto module_path = GetModulePath();
  auto path = AddTrailingSlash(GetPathOnly(module_path));
  using monolog::Level;
  monolog::log.enable_console_output(false);
  monolog::log.set_path(WstrToStr(path + TAIGA_APP_NAME L".log"));
  monolog::log.set_level(debug_mode ? Level::Debug : Level::Warning);
  LOGI(L"Version " + StrToWstr(version.str()) +
       L" (" + GetFileLastModifiedDate(module_path) + L")");

  // Check another instance
  if (!allow_multiple_instances) {
    if (CheckInstance(L"Taiga-33d5a63c-de90-432f-9a8b-f6f733dab258",
                      L"TaigaMainW")) {
      LOGD(L"Another instance of Taiga is running.");
      return FALSE;
    }
  }

  // Initialize
  InitCommonControls(ICC_STANDARD_CLASSES);
  OleInitialize(nullptr);

  // Load data
  LoadData();

  DummyAnime.Initialize();
  DummyEpisode.Initialize();

  // Create API windows
  ::Skype.Create();

  if (Settings.GetBool(kApp_Behavior_CheckForUpdates)) {
    ui::ShowDialog(ui::Dialog::Update);
  } else {
    ui::ShowDialog(ui::Dialog::Main);
  }

  return TRUE;
}

void App::Uninitialize() {
  // Announce
  if (MediaPlayers.play_status == track::recognition::PlayStatus::Playing) {
    MediaPlayers.play_status = track::recognition::PlayStatus::Stopped;
    ::Announcer.Do(kAnnounceToHttp);
  }
  ::Announcer.Clear(kAnnounceToSkype);

  // Cleanup
  ConnectionManager.Shutdown();
  ui::taskbar.Destroy();
  ui::taskbar_list.Release();

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

    if (argument == L"-debug") {
      debug_mode = true;
      LOGD(argument);
    } else if (argument == L"-allowmultipleinstances") {
      allow_multiple_instances = true;
      LOGD(argument);
    } else {
      LOGW(L"Invalid argument: " + argument);
    }
  }

  LocalFree(argument_list);
}

void App::LoadData() {
  MediaPlayers.Load();

  if (Settings.Load())
    if (Settings.HandleCompatibility())
      Settings.Save();

  ui::Theme.Load();
  ui::Menus.Load();

  AnimeDatabase.LoadDatabase();
  AnimeDatabase.LoadList();
  AnimeDatabase.ClearInvalidItems();

  History.Load();
}

}  // namespace taiga