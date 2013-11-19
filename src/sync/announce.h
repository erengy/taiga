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

#ifndef ANNOUNCE_H
#define ANNOUNCE_H

#include "std.h"

#include "third_party/oauth/oauth.h"

#include "win32/win_window.h"

namespace anime {
class Episode;
}

// =============================================================================

enum AnnouncerModes {
  ANNOUNCE_TO_HTTP      = 0x01,
  ANNOUNCE_TO_MESSENGER = 0x02,
  ANNOUNCE_TO_MIRC      = 0x04,
  ANNOUNCE_TO_SKYPE     = 0x08,
  ANNOUNCE_TO_TWITTER   = 0x10
};

class Announcer {
public:
  Announcer() {}
  virtual ~Announcer() {}

  void Clear(int modes, bool force = false);
  void Do(int modes, anime::Episode* episode = nullptr, bool force = false);

public:
  bool TestMircConnection(wstring service);

private:
  void ToHttp(wstring address, wstring data);
  void ToMessenger(wstring artist, wstring album, wstring title, BOOL show);
  bool ToMirc(wstring service, wstring channels, wstring data, int mode, BOOL use_action, BOOL multi_server);
  void ToSkype(const wstring& mood);
  void ToTwitter(const wstring& status_text);
};

extern Announcer Announcer;

// =============================================================================

/* mIRC */

enum MircChannelMode {
  MIRC_CHANNELMODE_ACTIVE = 1,
  MIRC_CHANNELMODE_ALL    = 2,
  MIRC_CHANNELMODE_CUSTOM = 3
};

// =============================================================================

/* Skype */

enum SkypeConnectionStatus {
  SKYPECONTROLAPI_ATTACH_SUCCESS = 0,
  SKYPECONTROLAPI_ATTACH_PENDING_AUTHORIZATION,
  SKYPECONTROLAPI_ATTACH_REFUSED,
  SKYPECONTROLAPI_ATTACH_NOT_AVAILABLE,
  SKYPECONTROLAPI_ATTACH_API_AVAILABLE = 0x8001
};

class Skype {
public:
  Skype();
  virtual ~Skype();

  void Create();
  BOOL Discover();
  LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
  BOOL SendCommand(const wstring& command);
  BOOL GetMoodText();
  BOOL SetMoodText(const wstring& mood);

  static const UINT wm_attach;
  static const UINT wm_discover;

public:
  HWND hwnd, hwnd_skype;
  wstring current_mood, previous_mood;

private:
  class Window : public win32::Window {
  public:
    Window() {}
    virtual ~Window() {}
  private:
    void PreRegisterClass(WNDCLASSEX& wc);
    void PreCreate(CREATESTRUCT& cs);
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } window_;
};

extern Skype Skype;

// =============================================================================

/* Twitter */

class Twitter {
public:
  Twitter();
  virtual ~Twitter() {}

  bool RequestToken();
  bool AccessToken(const wstring& key, const wstring& secret, const wstring& pin);
  bool SetStatusText(const wstring& status_text);

public:
  COAuth oauth;

private:
  wstring status_text_;
};

extern Twitter Twitter;

#endif // ANNOUNCE_H