/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#include "taiga/announce.h"

#include "base/log.h"
#include "base/string.h"
#include "track/episode.h"
#include "media/anime_util.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "ui/ui.h"

namespace taiga {

Announcer::~Announcer() {
  link::discord::Shutdown();
}

void Announcer::Clear(int modes, bool force) {
  if (modes & kAnnounceToDiscord)
    if (Settings.GetBool(kShare_Discord_Enabled) || force)
      link::discord::ClearPresence();

  if (modes & kAnnounceToHttp)
    if (Settings.GetBool(kShare_Http_Enabled) || force)
      link::http::Send(Settings[kShare_Http_Url], L"");
}

void Announcer::Do(int modes, anime::Episode* episode, bool force) {
  if (!force && !Settings.GetBool(kApp_Option_EnableSharing))
    return;

  if (!episode)
    episode = &CurrentEpisode;

  if (modes & kAnnounceToHttp) {
    if (Settings.GetBool(kShare_Http_Enabled) || force) {
      LOGD(L"HTTP");
      const auto data = ReplaceVariables(Settings[kShare_Http_Format],
                                         *episode, true, force);
      link::http::Send(Settings[kShare_Http_Url], data);
    }
  }

  if (!anime::IsValidId(episode->anime_id))
    return;

  if (modes & kAnnounceToDiscord) {
    if (Settings.GetBool(kShare_Discord_Enabled) || force) {
      LOGD(L"Discord");
      const std::wstring details = LimitText(ReplaceVariables(
          Settings[kShare_Discord_Format_Details], *episode, false, force), 64);
      const std::wstring state = LimitText(ReplaceVariables(
          Settings[kShare_Discord_Format_State], *episode, false, force), 64);
      auto timestamp = std::time(nullptr);
      if (!force)
        timestamp -= Settings.GetInt(taiga::kSync_Update_Delay);
      link::discord::UpdatePresence(WstrToStr(details), WstrToStr(state), timestamp);
    }
  }

  if (modes & kAnnounceToMirc) {
    if (Settings.GetBool(kShare_Mirc_Enabled) || force) {
      LOGD(L"mIRC");
      const auto data = ReplaceVariables(Settings[kShare_Mirc_Format],
                                         *episode, false, force);
      if (!link::mirc::Send(Settings[kShare_Mirc_Service],
                            Settings[kShare_Mirc_Channels],
                            data, Settings.GetInt(kShare_Mirc_Mode),
                            Settings.GetBool(kShare_Mirc_UseMeAction),
                            Settings.GetBool(kShare_Mirc_MultiServer))) {
        ui::OnMircDdeConnectionFail();
      }
    }
  }

  if (modes & kAnnounceToTwitter) {
    if (Settings.GetBool(kShare_Twitter_Enabled) || force) {
      LOGD(L"Twitter");
      const auto status_text = ReplaceVariables(Settings[kShare_Twitter_Format],
                                                *episode, false, force);
      link::twitter::SetStatusText(status_text);
    }
  }
}

}  // namespace taiga
