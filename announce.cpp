/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "std.h"

#include "announce.h"

#include "anime.h"
#include "anime_episode.h"
#include "common.h"
#include "debug.h"
#include "dde.h"
#include "http.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"

#include "dlg/dlg_main.h"

#include "win32/win_taskdialog.h"

class Announcer Announcer;
class Skype Skype;
class Twitter Twitter;

// =============================================================================

void Announcer::Clear(int modes, bool force) {
  if (modes & ANNOUNCE_TO_HTTP) {
    if (Settings.Announce.HTTP.enabled || force) {
      ToHttp(Settings.Announce.HTTP.url, L"");
    }
  }
  
  if (modes & ANNOUNCE_TO_MESSENGER) {
    if (Settings.Announce.MSN.enabled || force) {
      ToMessenger(L"", L"", L"", false);
    }
  }
  
  if (modes & ANNOUNCE_TO_MIRC) {
    // Not available
  }
  
  if (modes & ANNOUNCE_TO_SKYPE) {
    if (Settings.Announce.Skype.enabled || force) {
      ToSkype(L"");
    }
  }
  
  if (modes & ANNOUNCE_TO_TWITTER) {
    // Not available
  }
}

void Announcer::Do(int modes, anime::Episode* episode, bool force) {
  if (!force && !Settings.Program.General.enable_sharing)
    return;

  if (!episode)
    episode = &CurrentEpisode;

  if (modes & ANNOUNCE_TO_HTTP) {
    if (Settings.Announce.HTTP.enabled || force) {
      debug::Print(L"ANNOUNCE_TO_HTTP\n");
      ToHttp(Settings.Announce.HTTP.url, 
        ReplaceVariables(Settings.Announce.HTTP.format, *episode, true, force));
    }
  }

  if (episode->anime_id <= anime::ID_UNKNOWN)
    return;

  if (modes & ANNOUNCE_TO_MESSENGER) {
    if (Settings.Announce.MSN.enabled || force) {
      debug::Print(L"ANNOUNCE_TO_MESSENGER\n");
      ToMessenger(L"Taiga", L"MyAnimeList", 
        ReplaceVariables(Settings.Announce.MSN.format, *episode, false, force), true);
    }
  }

  if (modes & ANNOUNCE_TO_MIRC) {
    if (Settings.Announce.MIRC.enabled || force) {
      debug::Print(L"ANNOUNCE_TO_MIRC\n");
      ToMirc(Settings.Announce.MIRC.service, 
        Settings.Announce.MIRC.channels, 
        ReplaceVariables(Settings.Announce.MIRC.format, *episode, false, force), 
        Settings.Announce.MIRC.mode, 
        Settings.Announce.MIRC.use_action, 
        Settings.Announce.MIRC.multi_server);
    }
  }

  if (modes & ANNOUNCE_TO_SKYPE) {
    if (Settings.Announce.Skype.enabled || force) {
      debug::Print(L"ANNOUNCE_TO_SKYPE\n");
      ToSkype(ReplaceVariables(Settings.Announce.Skype.format, *episode, false, force));
    }
  }

  if (modes & ANNOUNCE_TO_TWITTER) {
    if (Settings.Announce.Twitter.enabled || force) {
      debug::Print(L"ANNOUNCE_TO_TWITTER\n");
      ToTwitter(ReplaceVariables(Settings.Announce.Twitter.format, *episode, false, force));
    }
  }
}

// =============================================================================

/* HTTP */

void Announcer::ToHttp(wstring address, wstring data) {
  if (address.empty() || data.empty()) return;

  Clients.sharing.http.Post(win32::Url(address), data, L"", HTTP_Silent);
}

// =============================================================================

/* Messenger */

void Announcer::ToMessenger(wstring artist, wstring album, wstring title, BOOL show) {
  if (title.empty() && show) return;
  
  COPYDATASTRUCT cds;
  WCHAR buffer[256];

  wstring wstr = L"\\0Music\\0" + ToWstr(show) + L"\\0{1}\\0" + 
    artist + L"\\0" + title + L"\\0" + album + L"\\0\\0";
  wcscpy_s(buffer, 256, wstr.c_str());

  cds.dwData = 0x547;
  cds.lpData = &buffer;
  cds.cbData = (lstrlenW(buffer) * 2) + 2;

  HWND hMessenger = NULL;
  while (hMessenger = FindWindowEx(NULL, hMessenger, L"MsnMsgrUIManager", NULL)) {
    if (hMessenger > 0) {
      SendMessage(hMessenger, WM_COPYDATA, NULL, (LPARAM)&cds);
    }
  }
}

// =============================================================================

/* mIRC */

bool Announcer::ToMirc(wstring service, wstring channels, wstring data, int mode, BOOL use_action, BOOL multi_server) {
  if (!FindWindow(L"mIRC", NULL)) return FALSE;
  if (service.empty() || channels.empty() || data.empty()) return FALSE;

  // Initialize
  DynamicDataExchange DDE;
  if (!DDE.Initialize(/*APPCLASS_STANDARD | APPCMD_CLIENTONLY, TRUE*/)) {
    win32::TaskDialog dlg(L"Announce to mIRC", TD_ICON_ERROR);
    dlg.SetMainInstruction(L"DDE initialization failed.");
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(g_hMain);
    return false;
  }

  // List channels
  if (mode != MIRC_CHANNELMODE_CUSTOM) {
    if (DDE.Connect(service, L"CHANNELS")) {
      DDE.ClientTransaction(L" ", L"", &channels, XTYP_REQUEST);
      DDE.Disconnect();
    }
  }
  vector<wstring> channel_list;
  Tokenize(channels, L" ,;", channel_list);
  for (size_t i = 0; i < channel_list.size(); i++) {
    Trim(channel_list[i]);
    if (channel_list[i].empty()) {
      continue;
    }
    if (channel_list[i].at(0) == '*') {
      channel_list[i] = channel_list[i].substr(1);
      if (mode == MIRC_CHANNELMODE_ACTIVE) {
        wstring temp = channel_list[i];
        channel_list.clear();
        channel_list.push_back(temp);
        break;
      }
    }
    if (channel_list[i].at(0) != '#') {
      channel_list[i].insert(channel_list[i].begin(), '#');
    }
  }

  // Connect
  if (!DDE.Connect(service, L"COMMAND")) {
    win32::TaskDialog dlg(L"Announce to mIRC", TD_ICON_ERROR);
    dlg.SetMainInstruction(L"DDE connection failed.");
    dlg.SetContent(L"Please enable DDE server from mIRC Options > Other > DDE.");
    dlg.AddButton(L"OK", IDOK);
    dlg.Show(g_hMain);
    DDE.UnInitialize();
    return false;
  }
  
  // Send message to channels
  for (size_t i = 0; i < channel_list.size(); i++) {
    wstring message;
    message += multi_server ? L"/scon -a " : L"";
    message += use_action ? L"/describe " : L"/msg ";
    message += channel_list[i] + L" " + data;
    DDE.ClientTransaction(L" ", message, NULL, XTYP_POKE);
  }

  // Clean up
  DDE.Disconnect();
  DDE.UnInitialize();
  return true;
}

bool Announcer::TestMircConnection(wstring service) {
  wstring content;
  win32::TaskDialog dlg(L"Test DDE connection", TD_ICON_ERROR);
  dlg.AddButton(L"OK", IDOK);
  
  // Search for mIRC window
  if (!FindWindow(L"mIRC", NULL)) {
    dlg.SetMainInstruction(L"mIRC is not running.");
    dlg.Show(g_hMain);
    return false;
  }

  // Initialize
  DynamicDataExchange DDE;
  if (!DDE.Initialize(/*APPCLASS_STANDARD | APPCMD_CLIENTONLY, TRUE*/)) {
    dlg.SetMainInstruction(L"DDE initialization failed.");
    dlg.Show(g_hMain);
    return false;
  }

  // Try to connect
  if (!DDE.Connect(service, L"CHANNELS")) {
    dlg.SetMainInstruction(L"DDE connection failed.");
    dlg.SetContent(L"Please enable DDE server from mIRC Options > Other > DDE.");
    dlg.Show(g_hMain);
    DDE.UnInitialize();
    return false;
  } else {
    wstring channels;
    DDE.ClientTransaction(L" ", L"", &channels, XTYP_REQUEST);
    if (!channels.empty()) content = L"Current channels: " + channels;
  }

  // Success
  dlg.SetMainIcon(TD_ICON_INFORMATION);
  dlg.SetMainInstruction(L"Successfuly connected to DDE server!");
  dlg.SetContent(content.c_str());
  dlg.Show(g_hMain);
  DDE.Disconnect();
  DDE.UnInitialize();
  return true;
}

// =============================================================================

/* Skype */

Skype::Skype() {
  api_window_handle = NULL;
  control_api_attach = ::RegisterWindowMessage(L"SkypeControlAPIAttach");
  control_api_discover = ::RegisterWindowMessage(L"SkypeControlAPIDiscover");
}

BOOL Skype::Attach() {
  PDWORD_PTR sendMessageResult = NULL;
  return SendMessageTimeout(HWND_BROADCAST, control_api_discover, reinterpret_cast<WPARAM>(g_hMain), 
    0, SMTO_NORMAL, 1000, sendMessageResult);
}

BOOL Skype::ChangeMood() {
  mood = L"SET PROFILE RICH_MOOD_TEXT " + mood;
  const char* buffer = ToANSI(mood);
  mood.clear();

  COPYDATASTRUCT cds;
  cds.dwData = 0;
  cds.lpData = (void*)buffer;
  cds.cbData = strlen(buffer) + 1;

  if (SendMessage(api_window_handle, WM_COPYDATA, reinterpret_cast<WPARAM>(g_hMain), 
    reinterpret_cast<LPARAM>(&cds)) == FALSE) {
      api_window_handle = NULL;
      return FALSE;
  } else {
    return TRUE;
  }
}

void Announcer::ToSkype(const wstring& mood) {
  Skype.mood = mood;

  if (Skype.api_window_handle == NULL) {
    Skype.Attach();
  } else {
    Skype.ChangeMood();
  }
}

// =============================================================================

/* Twitter */

Twitter::Twitter() {
  // These are unique values that identify Taiga
  oauth.ConsumerKey = L"9GZsCbqzjOrsPWlIlysvg";
  oauth.ConsumerSecret = L"ebjXyymbuLtjDvoxle9Ldj8YYIMoleORapIOoqBrjRw";
}

bool Twitter::RequestToken() {
  wstring header = 
    Clients.sharing.twitter.GetDefaultHeader() + 
    oauth.BuildHeader(
      L"http://api.twitter.com/oauth/request_token", 
      L"GET", NULL);

  return Clients.sharing.twitter.Connect(
    L"api.twitter.com", L"oauth/request_token",
    L"", L"GET", header, L"myanimelist.net", L"",
    HTTP_Twitter_Request);
}

bool Twitter::AccessToken(const wstring& key, const wstring& secret, const wstring& pin) {
  wstring header = 
    Clients.sharing.twitter.GetDefaultHeader() + 
    oauth.BuildHeader(
      L"http://api.twitter.com/oauth/access_token", 
      L"POST", NULL, 
      key, secret, pin);

  return Clients.sharing.twitter.Connect(
    L"api.twitter.com", L"oauth/access_token",
    L"", L"GET", header, L"myanimelist.net", L"",
    HTTP_Twitter_Auth);
}

bool Twitter::SetStatusText(const wstring& status_text) {
  if (Settings.Announce.Twitter.oauth_key.empty() || Settings.Announce.Twitter.oauth_secret.empty()) {
    return false;
  }
  if (status_text.empty() || status_text == status_text_) {
    return false;
  }
  status_text_ = status_text;

  OAuthParameters post_parameters;
  post_parameters[L"status"] = EncodeUrl(status_text_);

  wstring header = 
    Clients.sharing.twitter.GetDefaultHeader() + 
    oauth.BuildHeader(
      L"http://api.twitter.com/1.1/statuses/update.json", 
      L"POST", &post_parameters, 
      Settings.Announce.Twitter.oauth_key, 
      Settings.Announce.Twitter.oauth_secret);

  return Clients.sharing.twitter.Connect(
    L"api.twitter.com", L"1.1/statuses/update.json", 
    L"status=" + post_parameters[L"status"],
    L"POST", header, L"myanimelist.net", L"", 
    HTTP_Twitter_Post);
}

void Announcer::ToTwitter(const wstring& status_text) {
  Twitter.SetStatusText(status_text); 
}