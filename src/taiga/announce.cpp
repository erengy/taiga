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

#include <windows/win/dde.h>

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
taiga::Mirc Mirc;
taiga::Twitter Twitter;

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

bool Mirc::GetChannels(const std::wstring& service,
                       std::vector<std::wstring>& channels) {
  win::DynamicDataExchange dde;

  if (!dde.Initialize())
    return false;
  if (!dde.Connect(service, L"CHANNELS"))
    return false;

  std::wstring data;
  bool success = dde.ClientTransaction(L" ", L"", &data, XTYP_REQUEST) != FALSE;
  Split(data, L" ", channels);

  return success;
}

bool Mirc::IsRunning() {
  return FindWindow(L"mIRC", nullptr) != nullptr;
}

bool Mirc::SendCommands(const std::wstring& service,
                        const std::vector<std::wstring>& commands) {
  win::DynamicDataExchange dde;

  if (!dde.Initialize())
    return false;
  if (!dde.Connect(service, L"COMMAND"))
    return false;

  bool success = true;
  for (const auto& command : commands)
    if (success)
      success = dde.ClientTransaction(L" ", command, nullptr, XTYP_POKE) != FALSE;

  return success;
}

bool Mirc::Send(const std::wstring& service,
                std::wstring channels,
                const std::wstring& data,
                int mode,
                bool use_action,
                bool multi_server) {
  if (!IsRunning())
    return false;
  if (service.empty() || channels.empty() || data.empty())
    return false;

  // Initialize
  win::DynamicDataExchange dde;
  if (!dde.Initialize()) {
    ui::OnMircDdeInitFail();
    return false;
  }

  // List channels
  std::vector<std::wstring> channel_list;
  std::wstring active_channel;

  switch (mode) {
    case kMircChannelModeActive:
    case kMircChannelModeAll:
      GetChannels(service, channel_list);
      break;
    case kMircChannelModeCustom:
      Tokenize(channels, L" ,;", channel_list);
      break;
  }

  for (auto& channel : channel_list) {
    Trim(channel);
    if (channel.empty())  // ?
      continue;
    if (channel.front() == '*') {
      channel = channel.substr(1);
      active_channel = channel;
    } else if (channel.front() != '#') {
      channel.insert(channel.begin(), '#');
    }
  }

  // Send message to channels
  std::vector<std::wstring> commands;
  for (const auto& channel : channel_list) {
    if (mode == kMircChannelModeActive && channel != active_channel)
      continue;
    std::wstring command;
    if (multi_server)
      command += L"/scon -a ";
    command += use_action ? L"/describe " : L"/msg ";
    command += channel + L" " + data;
    commands.push_back(command);
  }
  return SendCommands(service, commands);
}

bool Announcer::ToMirc(const std::wstring& service,
                       std::wstring channels,
                       const std::wstring& data,
                       int mode,
                       bool use_action,
                       bool multi_server) {
  if (::Mirc.Send(service, channels, data, mode, use_action, multi_server)) {
    return true;
  } else {
    ui::OnMircDdeConnectionFail();
    return false;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Twitter

Twitter::Twitter() {
  // These are unique values that identify Taiga
  oauth.consumer_key = L"9GZsCbqzjOrsPWlIlysvg";
  oauth.consumer_secret = L"ebjXyymbuLtjDvoxle9Ldj8YYIMoleORapIOoqBrjRw";
}

bool Twitter::RequestToken() {
  HttpRequest http_request;
  http_request.url.protocol = base::http::Protocol::Https;
  http_request.url.host = L"api.twitter.com";
  http_request.url.path = L"/oauth/request_token";
  http_request.header[L"Authorization"] =
      oauth.BuildAuthorizationHeader(http_request.url.Build(), L"GET");

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTwitterRequest);
  return true;
}

bool Twitter::AccessToken(const std::wstring& key, const std::wstring& secret,
                          const std::wstring& pin) {
  HttpRequest http_request;
  http_request.url.protocol = base::http::Protocol::Https;
  http_request.url.host = L"api.twitter.com";
  http_request.url.path = L"/oauth/access_token";
  http_request.header[L"Authorization"] =
      oauth.BuildAuthorizationHeader(http_request.url.Build(),
                                     L"POST", nullptr, key, secret, pin);

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTwitterAuth);
  return true;
}

bool Twitter::SetStatusText(const std::wstring& status_text) {
  if (Settings[kShare_Twitter_OauthToken].empty() ||
      Settings[kShare_Twitter_OauthSecret].empty())
    return false;
  if (status_text.empty() || status_text == status_text_)
    return false;

  status_text_ = status_text;

  oauth_parameter_t post_parameters;
  post_parameters[L"status"] = EncodeUrl(status_text_);

  HttpRequest http_request;
  http_request.method = L"POST";
  http_request.url.protocol = base::http::Protocol::Https;
  http_request.url.host = L"api.twitter.com";
  http_request.url.path = L"/1.1/statuses/update.json";
  http_request.body = L"status=" + post_parameters[L"status"];
  http_request.header[L"Authorization"] =
      oauth.BuildAuthorizationHeader(http_request.url.Build(),
                                     L"POST", &post_parameters,
                                     Settings[kShare_Twitter_OauthToken],
                                     Settings[kShare_Twitter_OauthSecret]);

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTwitterPost);
  return true;
}

void Twitter::HandleHttpResponse(HttpClientMode mode,
                                 const HttpResponse& response) {
  switch (mode) {
    case kHttpTwitterRequest: {
      bool success = false;
      oauth_parameter_t parameters = oauth.ParseQueryString(response.body);
      if (!parameters[L"oauth_token"].empty()) {
        ExecuteLink(L"https://api.twitter.com/oauth/authorize?oauth_token=" +
                    parameters[L"oauth_token"]);
        string_t auth_pin;
        if (ui::OnTwitterTokenEntry(auth_pin))
          AccessToken(parameters[L"oauth_token"],
                      parameters[L"oauth_token_secret"],
                      auth_pin);
        success = true;
      }
      ui::OnTwitterTokenRequest(success);
      break;
    }

    case kHttpTwitterAuth: {
      bool success = false;
      oauth_parameter_t parameters = oauth.ParseQueryString(response.body);
      if (!parameters[L"oauth_token"].empty() &&
          !parameters[L"oauth_token_secret"].empty()) {
        Settings.Set(kShare_Twitter_OauthToken, parameters[L"oauth_token"]);
        Settings.Set(kShare_Twitter_OauthSecret, parameters[L"oauth_token_secret"]);
        Settings.Set(kShare_Twitter_Username, parameters[L"screen_name"]);
        success = true;
      }
      ui::OnTwitterAuth(success);
      break;
    }

    case kHttpTwitterPost: {
      if (InStr(response.body, L"\"errors\"", 0) == -1) {
        ui::OnTwitterPost(true, L"");
      } else {
        string_t error;
        int index_begin = InStr(response.body, L"\"message\":\"", 0);
        int index_end = InStr(response.body, L"\",\"", index_begin);
        if (index_begin > -1 && index_end > -1) {
          index_begin += 11;
          error = response.body.substr(index_begin, index_end - index_begin);
        }
        ui::OnTwitterPost(false, error);
      }
      break;
    }
  }
}

void Announcer::ToTwitter(const std::wstring& status_text) {
  ::Twitter.SetStatusText(status_text);
}

}  // namespace taiga