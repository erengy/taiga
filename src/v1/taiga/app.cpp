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

#include <windows/win/taskbar.h>

#include "taiga/app.h"

#include "base/command_line.h"
#include "base/file.h"
#include "base/log.h"
#include "base/process.h"
#include "base/string.h"
#include "link/discord.h"
#include "media/anime_db.h"
#include "media/library/history.h"
#include "taiga/announce.h"
#include "taiga/config.h"
#include "taiga/dummy.h"
#include "taiga/http.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "taiga/version.h"
#include "track/feed_aggregator.h"
#include "track/media.h"
#include "ui/dialog.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace taiga {

namespace detail {

CommandLineOptions ParseCommandLine() {
  CommandLineOptions options;

  const auto args = base::ParseCommandLineArgs();

  for (const auto& arg : args) {
    bool found = false;

    if (arg == L"allowmultipleinstances") {
      options.allow_multiple_instances = true;
      found = true;
    } else if (arg == L"debug") {
      options.debug_mode = true;
      found = true;
    } else if (arg == L"verbose") {
      options.verbose = true;
      found = true;
    }

    if (found) {
      LOGD(arg);
    } else {
      LOGW(L"Invalid argument: {}", arg);
    }
  }

  return options;
}

void LoadData() {
  track::media_players.Load();

  if (settings.Load())
    if (settings.HandleCompatibility())
      settings.Save();

  ui::Theme.Load();
  ui::Menus.Load();

  anime::db.LoadDatabase();
  anime::db.LoadList();
  anime::db.ClearInvalidItems();

  library::history.Load();
  track::aggregator.archive.Load();
}

}  // namespace detail

App::~App() {
  OleUninitialize();
}

BOOL App::InitInstance() {
  // Parse command line
  options = detail::ParseCommandLine();
#ifdef _DEBUG
  options.debug_mode = true;
#endif

  // Initialize logger
  const auto module_path = GetModulePath();
  const auto path = AddTrailingSlash(GetPathOnly(module_path));
  using monolog::Level;
  monolog::log.enable_console_output(false);
  monolog::log.set_path(path + TAIGA_APP_NAME L".log");
  monolog::log.set_level(options.debug_mode ? Level::Debug : Level::Warning);
  LOGI(L"Version {} ({})", StrToWstr(taiga::version().to_string()),
       GetFileLastModifiedDate(module_path));

  // Check another instance
  if (!options.allow_multiple_instances) {
    if (CheckInstance(TAIGA_APP_MUTEX, L"TaigaMainW")) {
      LOGD(L"Another instance of Taiga is running.");
      return FALSE;
    }
  }

  // Initialize
  http::Init();
  InitCommonControls(ICC_STANDARD_CLASSES);
  OleInitialize(nullptr);

  // Load data
  detail::LoadData();

  InitializeDummies();

  // Initialize Discord
  if (settings.GetShareDiscordEnabled())
    link::discord::Initialize();

  if (settings.GetAppBehaviorCheckForUpdates()) {
    ui::ShowDialog(ui::Dialog::Update);
  } else {
    ui::ShowDialog(ui::Dialog::Main);
  }

  return TRUE;
}

void App::Uninitialize() {
  // Announce
  if (track::media_players.play_status ==
      track::recognition::PlayStatus::Playing) {
    track::media_players.play_status = track::recognition::PlayStatus::Stopped;
    announcer.Do(kAnnounceToHttp);
  }
  announcer.Clear(kAnnounceToDiscord);

  // Cleanup
  http::Shutdown();
  ui::taskbar.Destroy();
  ui::taskbar_list.Release();

  // Save
  settings.Save();
  anime::db.SaveDatabase();
  track::aggregator.archive.Save();

  // Exit
  PostQuitMessage();
}

}  // namespace taiga
