/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
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
