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

#include <discord_rpc.h>

#include "link/discord.h"

#include "base/format.h"
#include "base/log.h"
#include "base/string.h"
#include "sync/service.h"
#include "taiga/settings.h"

namespace link::discord {

static void OnReady(const DiscordUser* user) {
  if (user) {
    LOGD(L"Discord: ready ({}#{})",
         StrToWstr(user->username), StrToWstr(user->discriminator));
  } else {
    LOGD(L"Discord: ready");
  }
}

static void OnDisconnected(int errcode, const char* message) {
  LOGD(L"Discord: disconnected ({}: {})", errcode, StrToWstr(message));
}

static void OnError(int errcode, const char* message) {
  LOGD(L"Discord: error ({}: {})", errcode, StrToWstr(message));
}

static void RunCallbacks() {
#ifdef DISCORD_DISABLE_IO_THREAD
  Discord_UpdateConnection();
#endif
  Discord_RunCallbacks();
}

void Initialize() {
  DiscordEventHandlers handlers = {0};
  handlers.ready = OnReady;
  handlers.disconnected = OnDisconnected;
  handlers.errored = OnError;

  const auto application_id =
      WstrToStr(taiga::settings.GetShareDiscordApplicationId());

  Discord_Initialize(application_id.c_str(), &handlers, FALSE, nullptr);
}

void Shutdown() {
  Discord_ClearPresence();
  Discord_Shutdown();
}

void ClearPresence() {
  Discord_ClearPresence();
  RunCallbacks();
}

void UpdatePresence(const std::string& details, const std::string& state,
                    const std::string& large_image,
                    const std::string& button_label,
                    const std::string& button_url, const time_t timestamp) {
  const std::string small_image_key = WstrToStr(sync::GetCurrentServiceSlug());

  const std::string small_image_text =
      taiga::settings.GetShareDiscordUsernameEnabled()
          ? WstrToStr(L"{} at {}"_format(taiga::GetCurrentUserDisplayName(),
                                         sync::GetCurrentServiceName()))
          : WstrToStr(sync::GetCurrentServiceName());

  DiscordRichPresence presence = {0};
  presence.state = state.c_str();
  presence.details = details.c_str();
  if (taiga::settings.GetShareDiscordTimeEnabled()) {
    presence.startTimestamp = timestamp;
  }
  presence.largeImageKey =
      !large_image.empty() ? large_image.c_str() : "default";
  presence.largeImageText = details.c_str();
  presence.smallImageKey = small_image_key.c_str();
  presence.smallImageText = small_image_text.c_str();
  if (!button_label.empty() && !button_url.empty()) {
    presence.buttons[0].label = button_label.c_str();
    presence.buttons[0].url = button_url.c_str();
  }

  Discord_UpdatePresence(&presence);
  RunCallbacks();
}

}  // namespace link::discord
