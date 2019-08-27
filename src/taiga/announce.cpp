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

#include "base/file.h"
#include "base/log.h"
#include "base/string.h"
#include "base/url.h"
#include "library/anime.h"
#include "library/anime_episode.h"
#include "library/anime_util.h"
#include "sync/service.h"
#include "taiga/announce.h"
#include "taiga/http.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "ui/ui.h"

taiga::Announcer Announcer;

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
      ToHttp(Settings[kShare_Http_Url], L"");
}

void Announcer::Do(int modes, anime::Episode* episode, bool force) {
  if (!force && !Settings.GetBool(kApp_Option_EnableSharing))
    return;

  if (!episode)
    episode = &CurrentEpisode;

  if (modes & kAnnounceToHttp) {
    if (Settings.GetBool(kShare_Http_Enabled) || force) {
      LOGD(L"HTTP");
      ToHttp(Settings[kShare_Http_Url],
             ReplaceVariables(Settings[kShare_Http_Format],
                              *episode, true, force));
    }
  }

  if (!anime::IsValidId(episode->anime_id))
    return;

  if (modes & kAnnounceToDiscord) {
    if (Settings.GetBool(kShare_Discord_Enabled) || force) {
      LOGD(L"Discord");
      auto timestamp = ::time(nullptr);
      if (!force)
        timestamp -= Settings.GetInt(taiga::kSync_Update_Delay);
      ToDiscord(ReplaceVariables(Settings[kShare_Discord_Format_Details],
                                 *episode, false, force),
                ReplaceVariables(Settings[kShare_Discord_Format_State],
                                 *episode, false, force),
                timestamp);
    }
  }

  if (modes & kAnnounceToMirc) {
    if (Settings.GetBool(kShare_Mirc_Enabled) || force) {
      LOGD(L"mIRC");
      ToMirc(Settings[kShare_Mirc_Service],
             Settings[kShare_Mirc_Channels],
             ReplaceVariables(Settings[kShare_Mirc_Format],
                              *episode, false, force),
             Settings.GetInt(kShare_Mirc_Mode),
             Settings.GetBool(kShare_Mirc_UseMeAction),
             Settings.GetBool(kShare_Mirc_MultiServer));
    }
  }

  if (modes & kAnnounceToTwitter) {
    if (Settings.GetBool(kShare_Twitter_Enabled) || force) {
      LOGD(L"Twitter");
      ToTwitter(ReplaceVariables(Settings[kShare_Twitter_Format],
                                 *episode, false, force));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Discord

void Announcer::ToDiscord(const std::wstring& details,
                          const std::wstring& state,
                          time_t timestamp) {
  link::discord::UpdatePresence(WstrToStr(LimitText(details, 64)),
                                WstrToStr(LimitText(state, 64)),
                                timestamp);
}

////////////////////////////////////////////////////////////////////////////////
// HTTP

void Announcer::ToHttp(const std::wstring& address, const std::wstring& data) {
  if (address.empty() || data.empty())
    return;

  HttpRequest http_request;
  http_request.method = L"POST";
  http_request.url = address;
  http_request.body = data;

  ConnectionManager.MakeRequest(http_request, taiga::kHttpSilent);
}

////////////////////////////////////////////////////////////////////////////////
// mIRC

bool Announcer::ToMirc(const std::wstring& service,
                       std::wstring channels,
                       const std::wstring& data,
                       int mode,
                       bool use_action,
                       bool multi_server) {
  if (link::mirc::Send(service, channels, data, mode, use_action, multi_server)) {
    return true;
  } else {
    ui::OnMircDdeConnectionFail();
    return false;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Twitter

void Announcer::ToTwitter(const std::wstring& status_text) {
  link::twitter::SetStatusText(status_text);
}

}  // namespace taiga