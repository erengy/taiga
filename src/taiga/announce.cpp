/*
** Taiga
** Copyright (C) 2010-2018, Eren Okka
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

#include <discord_rpc.h>
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
taiga::Discord Discord;
taiga::Mirc Mirc;
taiga::Skype Skype;
taiga::Twitter Twitter;

namespace taiga {

void Announcer::Clear(int modes, bool force) {
  if (modes & kAnnounceToDiscord)
    if (Settings.GetBool(kShare_Discord_Enabled) || force)
      ::Discord.ClearPresence();

  if (modes & kAnnounceToHttp)
    if (Settings.GetBool(kShare_Http_Enabled) || force)
      ToHttp(Settings[kShare_Http_Url], L"");

  if (modes & kAnnounceToSkype)
    if (Settings.GetBool(kShare_Skype_Enabled) || force)
      ToSkype(::Skype.previous_mood);
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

  if (modes & kAnnounceToSkype) {
    if (Settings.GetBool(kShare_Skype_Enabled) || force) {
      LOGD(L"Skype");
      ToSkype(ReplaceVariables(Settings[kShare_Skype_Format],
                               *episode, false, force));
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

Discord::~Discord() {
  ClearPresence();
  Shutdown();
}

void Discord::Initialize() const {
  DiscordEventHandlers handlers = {0};
  handlers.ready = OnReady;
  handlers.disconnected = OnDisconnected;
  handlers.errored = OnError;

  const auto application_id = WstrToStr(Settings[kShare_Discord_ApplicationId]);

  Discord_Initialize(application_id.c_str(), &handlers, FALSE, nullptr);
}

void Discord::Shutdown() const {
  Discord_Shutdown();
}

void Discord::ClearPresence() const {
  Discord_ClearPresence();
  RunCallbacks();
}

void Discord::UpdatePresence(const std::string& details,
                             const std::string& state,
                             time_t timestamp) const {
  const std::string small_image_key =
      WstrToStr(GetCurrentService()->canonical_name());

  std::string small_image_text =
      WstrToStr(GetCurrentService()->name());
  if (Settings.GetBool(kShare_Discord_Username_Enabled)) {
    small_image_text = WstrToStr(GetCurrentUserDisplayName() + L" at ") +
                       small_image_text;
  }

  DiscordRichPresence presence = {0};
  presence.state = state.c_str();
  presence.details = details.c_str();
  presence.startTimestamp = timestamp;
  presence.largeImageKey = "default";
  presence.largeImageText = details.c_str();
  presence.smallImageKey = small_image_key.c_str();
  presence.smallImageText = small_image_text.c_str();

  Discord_UpdatePresence(&presence);
  RunCallbacks();
}

void Discord::RunCallbacks() const {
#ifdef DISCORD_DISABLE_IO_THREAD
  Discord_UpdateConnection();
#endif
  Discord_RunCallbacks();
}

void Discord::OnReady() {
  LOGD(L"Discord: ready");
}

void Discord::OnDisconnected(int errcode, const char* message) {
  LOGD(L"Discord: disconnected ({}: {})", errcode, message);
}

void Discord::OnError(int errcode, const char* message) {
  LOGD(L"Discord: error ({}: {})", errcode, message);
}

void Announcer::ToDiscord(const std::wstring& details,
                          const std::wstring& state,
                          time_t timestamp) {
  ::Discord.UpdatePresence(WstrToStr(LimitText(details, 64)),
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
// Skype

const UINT Skype::wm_attach = ::RegisterWindowMessage(L"SkypeControlAPIAttach");
const UINT Skype::wm_discover = ::RegisterWindowMessage(L"SkypeControlAPIDiscover");

Skype::Skype()
    : hwnd(nullptr),
      hwnd_skype(nullptr) {
}

Skype::~Skype() {
  window_.Destroy();
}

void Skype::Create() {
  hwnd = window_.Create();
}

BOOL Skype::Discover() {
  PDWORD_PTR sendMessageResult = nullptr;
  return SendMessageTimeout(HWND_BROADCAST, wm_discover,
                            reinterpret_cast<WPARAM>(hwnd),
                            0, SMTO_NORMAL, 1000, sendMessageResult);
}

bool Skype::SendCommand(const std::wstring& command) {
  std::string str = WstrToStr(command);
  const char* buffer = str.c_str();

  COPYDATASTRUCT cds;
  cds.dwData = 0;
  cds.lpData = (void*)buffer;
  cds.cbData = strlen(buffer) + 1;

  if (SendMessage(hwnd_skype, WM_COPYDATA,
                  reinterpret_cast<WPARAM>(hwnd),
                  reinterpret_cast<LPARAM>(&cds)) == FALSE) {
    LOGE(L"WM_COPYDATA failed.");
    hwnd_skype = nullptr;
    return false;
  } else {
    LOGD(L"WM_COPYDATA succeeded.");
    return true;
  }
}

bool Skype::GetMoodText() {
  std::wstring command = L"GET PROFILE RICH_MOOD_TEXT";
  return SendCommand(command);
}

bool Skype::SetMoodText(const std::wstring& mood) {
  current_mood = mood;
  std::wstring command = L"SET PROFILE RICH_MOOD_TEXT " + mood;
  return SendCommand(command);
}

void Skype::Window::PreRegisterClass(WNDCLASSEX& wc) {
  wc.lpszClassName = L"TaigaSkypeW";
}

void Skype::Window::PreCreate(CREATESTRUCT& cs) {
  cs.lpszName = L"Taiga <3 Skype";
  cs.style = WS_OVERLAPPEDWINDOW;
}

LRESULT Skype::Window::WindowProc(HWND hwnd, UINT uMsg,
                                  WPARAM wParam, LPARAM lParam) {
  if (::Skype.HandleMessage(uMsg, wParam, lParam))
    return TRUE;

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT Skype::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (uMsg == WM_COPYDATA) {
    if (hwnd_skype == nullptr ||
        hwnd_skype != reinterpret_cast<HWND>(wParam))
      return FALSE;

    auto pCDS = reinterpret_cast<PCOPYDATASTRUCT>(lParam);
    std::wstring command = StrToWstr(reinterpret_cast<LPCSTR>(pCDS->lpData));
    LOGD(L"Received WM_COPYDATA: {}", command);

    std::wstring profile_command = L"PROFILE RICH_MOOD_TEXT ";
    if (StartsWith(command, profile_command)) {
      std::wstring mood = command.substr(profile_command.length());
      if (mood != current_mood && mood != previous_mood) {
        LOGD(L"Saved previous mood message: {}", mood);
        previous_mood = mood;
      }
    }

    return TRUE;

  } else if (uMsg == wm_attach) {
    hwnd_skype = nullptr;

    switch (lParam) {
      case kSkypeControlApiAttachSuccess:
        LOGD(L"Attach succeeded.");
        hwnd_skype = reinterpret_cast<HWND>(wParam);
        GetMoodText();
        if (!current_mood.empty())
          SetMoodText(current_mood);
        break;
      case kSkypeControlApiAttachPendingAuthorization:
        LOGD(L"Waiting for user confirmation...");
        break;
      case kSkypeControlApiAttachRefused:
        LOGE(L"User denied access to client.");
        break;
      case kSkypeControlApiAttachNotAvailable:
        LOGE(L"API is not available.");
        break;
      case kSkypeControlApiAttachApiAvailable:
        LOGD(L"API is now available.");
        Discover();
        break;
      default:
        LOGD(L"Received unknown message.");
        break;
    }

    return TRUE;

  } else if (uMsg == wm_discover) {
    LOGD(L"Received SkypeControlAPIDiscover message.");
  }

  return FALSE;
}

void Announcer::ToSkype(const std::wstring& mood) {
  ::Skype.current_mood = mood;

  if (::Skype.hwnd_skype == nullptr) {
    ::Skype.Discover();
  } else {
    ::Skype.SetMoodText(mood);
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