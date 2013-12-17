/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#include "announce.h"
#include "base/dde.h"
#include "base/encoding.h"
#include "base/foreach.h"
#include "base/logger.h"
#include "base/string.h"
#include "library/anime.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "win/win_taskdialog.h"

taiga::Announcer Announcer;
taiga::Skype Skype;
taiga::Twitter Twitter;

namespace taiga {

void Announcer::Clear(int modes, bool force) {
  if (modes & kAnnounceToHttp)
    if (Settings.Announce.HTTP.enabled || force)
      ToHttp(Settings.Announce.HTTP.url, L"");

  if (modes & kAnnounceToMessenger)
    if (Settings.Announce.MSN.enabled || force)
      ToMessenger(L"", L"", L"", false);

  if (modes & kAnnounceToSkype)
    if (Settings.Announce.Skype.enabled || force)
      ToSkype(::Skype.previous_mood);
}

void Announcer::Do(int modes, anime::Episode* episode, bool force) {
  if (!force && !Settings.Program.General.enable_sharing)
    return;

  if (!episode)
    episode = &CurrentEpisode;

  if (modes & kAnnounceToHttp) {
    if (Settings.Announce.HTTP.enabled || force) {
      LOG(LevelDebug, L"HTTP");
      ToHttp(Settings.Announce.HTTP.url,
             ReplaceVariables(Settings.Announce.HTTP.format,
                              *episode, true, force));
    }
  }

  if (episode->anime_id <= anime::ID_UNKNOWN)
    return;

  if (modes & kAnnounceToMessenger) {
    if (Settings.Announce.MSN.enabled || force) {
      LOG(LevelDebug, L"Messenger");
      ToMessenger(L"Taiga", L"MyAnimeList",
                  ReplaceVariables(Settings.Announce.MSN.format,
                                   *episode, false, force),
                  true);
    }
  }

  if (modes & kAnnounceToMirc) {
    if (Settings.Announce.MIRC.enabled || force) {
      LOG(LevelDebug, L"mIRC");
      ToMirc(Settings.Announce.MIRC.service,
             Settings.Announce.MIRC.channels,
             ReplaceVariables(Settings.Announce.MIRC.format,
                              *episode, false, force),
             Settings.Announce.MIRC.mode,
             Settings.Announce.MIRC.use_action,
             Settings.Announce.MIRC.multi_server);
    }
  }

  if (modes & kAnnounceToSkype) {
    if (Settings.Announce.Skype.enabled || force) {
      LOG(LevelDebug, L"Skype");
      ToSkype(ReplaceVariables(Settings.Announce.Skype.format,
                               *episode, false, force));
    }
  }

  if (modes & kAnnounceToTwitter) {
    if (Settings.Announce.Twitter.enabled || force) {
      LOG(LevelDebug, L"Twitter");
      ToTwitter(ReplaceVariables(Settings.Announce.Twitter.format,
                                 *episode, false, force));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// HTTP

void Announcer::ToHttp(const std::wstring& address, const std::wstring& data) {
  if (address.empty() || data.empty())
    return;

  win::http::Url url(address);

  HttpRequest http_request;
  http_request.method = L"POST";
  http_request.host = url.host;
  http_request.path = url.path;
  http_request.body = data;

  ConnectionManager.MakeRequest(http_request, taiga::kHttpSilent);
}

////////////////////////////////////////////////////////////////////////////////
// Messenger

void Announcer::ToMessenger(const std::wstring& artist,
                            const std::wstring& album,
                            const std::wstring& title,
                            BOOL show) {
  if (title.empty() && show)
    return;

  COPYDATASTRUCT cds;
  WCHAR buffer[256];

  wstring wstr = L"\\0Music\\0" + ToWstr(show) + L"\\0{1}\\0" + 
                 artist + L"\\0" + title + L"\\0" + album + L"\\0\\0";
  wcscpy_s(buffer, 256, wstr.c_str());

  cds.dwData = 0x547;
  cds.lpData = &buffer;
  cds.cbData = (lstrlenW(buffer) * 2) + 2;

  HWND hwnd = nullptr;
  while (hwnd = FindWindowEx(nullptr, hwnd, L"MsnMsgrUIManager", nullptr))
    if (hwnd > 0)
      SendMessage(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
}

////////////////////////////////////////////////////////////////////////////////
// mIRC

bool Announcer::ToMirc(const std::wstring& service,
                       std::wstring channels,
                       const std::wstring& data,
                       int mode,
                       BOOL use_action,
                       BOOL multi_server) {
  if (!FindWindow(L"mIRC", nullptr))
    return false;
  if (service.empty() || channels.empty() || data.empty())
    return false;

  // Initialize
  base::DynamicDataExchange dde;
  if (!dde.Initialize(/*APPCLASS_STANDARD | APPCMD_CLIENTONLY, TRUE*/)) {
    win::TaskDialog dlg(L"Announce to mIRC", TD_ICON_ERROR);
    dlg.SetMainInstruction(L"DDE initialization failed.");
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(g_hMain);
    return false;
  }

  // List channels
  if (mode != kMircChannelModeCustom) {
    if (dde.Connect(service, L"CHANNELS")) {
      dde.ClientTransaction(L" ", L"", &channels, XTYP_REQUEST);
      dde.Disconnect();
    }
  }
  std::vector<std::wstring> channel_list;
  Tokenize(channels, L" ,;", channel_list);
  foreach_(it, channel_list) {
    Trim(*it);
    if (it->empty())
      continue;
    if (it->at(0) == '*') {
      *it = it->substr(1);
      if (mode == kMircChannelModeActive) {
        wstring temp = *it;
        channel_list.clear();
        channel_list.push_back(temp);
        break;
      }
    }
    if (it->at(0) != '#')
      it->insert(it->begin(), '#');
  }

  // Connect
  if (!dde.Connect(service, L"COMMAND")) {
    win::TaskDialog dlg(L"Announce to mIRC", TD_ICON_ERROR);
    dlg.SetMainInstruction(L"DDE connection failed.");
    dlg.SetContent(L"Please enable DDE server from mIRC Options > Other > DDE.");
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(g_hMain);
    dde.UnInitialize();
    return false;
  }

  // Send message to channels
  foreach_(it, channel_list) {
    wstring message;
    message += multi_server ? L"/scon -a " : L"";
    message += use_action ? L"/describe " : L"/msg ";
    message += *it + L" " + data;
    dde.ClientTransaction(L" ", message, NULL, XTYP_POKE);
  }

  // Clean up
  dde.Disconnect();
  dde.UnInitialize();

  return true;
}

bool Announcer::TestMircConnection(const std::wstring& service) {
  wstring content;
  win::TaskDialog dlg(L"Test DDE connection", TD_ICON_ERROR);
  dlg.AddButton(L"OK", IDOK);

  // Search for mIRC window
  if (!FindWindow(L"mIRC", nullptr)) {
    dlg.SetMainInstruction(L"mIRC is not running.");
    dlg.Show(g_hMain);
    return false;
  }

  // Initialize
  base::DynamicDataExchange dde;
  if (!dde.Initialize(/*APPCLASS_STANDARD | APPCMD_CLIENTONLY, TRUE*/)) {
    dlg.SetMainInstruction(L"DDE initialization failed.");
    dlg.Show(g_hMain);
    return false;
  }

  // Try to connect
  if (!dde.Connect(service, L"CHANNELS")) {
    dlg.SetMainInstruction(L"DDE connection failed.");
    dlg.SetContent(L"Please enable DDE server from mIRC Options > Other > DDE.");
    dlg.Show(g_hMain);
    dde.UnInitialize();
    return false;
  } else {
    wstring channels;
    dde.ClientTransaction(L" ", L"", &channels, XTYP_REQUEST);
    if (!channels.empty())
      content = L"Current channels: " + channels;
  }

  // Success
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Successfuly connected to DDE server!");
  dlg.SetContent(content.c_str());
  dlg.Show(g_hMain);
  dde.Disconnect();
  dde.UnInitialize();

  return true;
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

bool Skype::SendCommand(const wstring& command) {
  string str = WstrToStr(command);
  const char* buffer = str.c_str();

  COPYDATASTRUCT cds;
  cds.dwData = 0;
  cds.lpData = (void*)buffer;
  cds.cbData = strlen(buffer) + 1;

  if (SendMessage(hwnd_skype, WM_COPYDATA,
                  reinterpret_cast<WPARAM>(hwnd), 
                  reinterpret_cast<LPARAM>(&cds)) == FALSE) {
    LOG(LevelError, L"WM_COPYDATA failed.");
    hwnd_skype = nullptr;
    return false;
  } else {
    LOG(LevelDebug, L"WM_COPYDATA succeeded.");
    return true;
  }
}

bool Skype::GetMoodText() {
  wstring command = L"GET PROFILE RICH_MOOD_TEXT";
  return SendCommand(command);
}

bool Skype::SetMoodText(const wstring& mood) {
  current_mood = mood;
  wstring command = L"SET PROFILE RICH_MOOD_TEXT " + mood;
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
    wstring command = StrToWstr(reinterpret_cast<LPCSTR>(pCDS->lpData));
    LOG(LevelDebug, L"Received WM_COPYDATA: " + command);

    wstring profile_command = L"PROFILE RICH_MOOD_TEXT ";
    if (StartsWith(command, profile_command)) {
      wstring mood = command.substr(profile_command.length());
      if (mood != current_mood && mood != previous_mood) {
        LOG(LevelDebug, L"Saved previous mood message: " + mood);
        previous_mood = mood;
      }
    }

    return TRUE;

  } else if (uMsg == wm_attach) {
    hwnd_skype = nullptr;

    switch (lParam) {
      case kSkypeControlApiAttachSuccess:
        LOG(LevelDebug, L"Attach succeeded.");
        hwnd_skype = reinterpret_cast<HWND>(wParam);
        GetMoodText();
        if (!current_mood.empty())
          SetMoodText(current_mood);
        break;
      case kSkypeControlApiAttachPendingAuthorization:
        LOG(LevelDebug, L"Waiting for user confirmation...");
        break;
      case kSkypeControlApiAttachRefused:
        LOG(LevelError, L"User denied access to client.");
        break;
      case kSkypeControlApiAttachNotAvailable:
        LOG(LevelError, L"API is not available.");
        break;
      case kSkypeControlApiAttachApiAvailable:
        LOG(LevelDebug, L"API is now available.");
        Discover();
        break;
      default:
        LOG(LevelDebug, L"Received unknown message.");
        break;
    }

    return TRUE;

  } else if (uMsg == wm_discover) {
    LOG(LevelDebug, L"Received SkypeControlAPIDiscover message.");
  }

  return FALSE;
}

void Announcer::ToSkype(const wstring& mood) {
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
  oauth.ConsumerKey = L"9GZsCbqzjOrsPWlIlysvg";
  oauth.ConsumerSecret = L"ebjXyymbuLtjDvoxle9Ldj8YYIMoleORapIOoqBrjRw";
}

bool Twitter::RequestToken() {
  HttpRequest http_request;
  http_request.host = L"api.twitter.com";
  http_request.path = L"oauth/request_token";
  http_request.header[L"Authorization"] =
      oauth.BuildAuthorizationHeader(L"http://api.twitter.com/oauth/request_token",
                                     L"GET");

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTwitterRequest);
  return true;
}

bool Twitter::AccessToken(const wstring& key, const wstring& secret, const wstring& pin) {
  HttpRequest http_request;
  http_request.host = L"api.twitter.com";
  http_request.path = L"oauth/access_token";
  http_request.header[L"Authorization"] =
      oauth.BuildAuthorizationHeader(L"http://api.twitter.com/oauth/access_token",
                                     L"POST", nullptr, key, secret, pin);

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTwitterAuth);
  return true;
}

bool Twitter::SetStatusText(const wstring& status_text) {
  if (Settings.Announce.Twitter.oauth_key.empty() ||
      Settings.Announce.Twitter.oauth_secret.empty())
    return false;
  if (status_text.empty() || status_text == status_text_)
    return false;

  status_text_ = status_text;

  OAuthParameters post_parameters;
  post_parameters[L"status"] = EncodeUrl(status_text_);

  HttpRequest http_request;
  http_request.method = L"POST";
  http_request.host = L"api.twitter.com";
  http_request.path = L"1.1/statuses/update.json";
  http_request.body = L"status=" + post_parameters[L"status"];
  http_request.header[L"Authorization"] =
      oauth.BuildAuthorizationHeader(L"http://api.twitter.com/1.1/statuses/update.json",
                                     L"POST", &post_parameters,
                                     Settings.Announce.Twitter.oauth_key,
                                     Settings.Announce.Twitter.oauth_secret);

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTwitterPost);
  return true;
}

void Announcer::ToTwitter(const wstring& status_text) {
  ::Twitter.SetStatusText(status_text); 
}

}  // namespace taiga