/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#pragma once

#include <string>

#include <windows/win/window.h>

#include "base/oauth.h"
#include "base/types.h"
#include "taiga/http.h"

namespace anime {
class Episode;
}

namespace taiga {

enum AnnouncerModes {
  kAnnounceToHttp      = 0x01,
  kAnnounceToMirc      = 0x02,
  kAnnounceToSkype     = 0x04,
  kAnnounceToTwitter   = 0x08
};

class Announcer {
public:
  void Clear(int modes, bool force = false);
  void Do(int modes, anime::Episode* episode = nullptr, bool force = false);

private:
  void ToHttp(const std::wstring& address, const std::wstring& data);
  bool ToMirc(const std::wstring& service, std::wstring channels, const std::wstring& data, int mode, bool use_action, bool multi_server);
  void ToSkype(const std::wstring& mood);
  void ToTwitter(const std::wstring& status_text);
};

////////////////////////////////////////////////////////////////////////////////
// mIRC

enum MircChannelMode {
  kMircChannelModeActive = 1,
  kMircChannelModeAll,
  kMircChannelModeCustom
};

class Mirc {
public:
  bool GetChannels(const std::wstring& service, std::vector<std::wstring>& channels);
  bool IsRunning();
  bool Send(const std::wstring& service, std::wstring channels, const std::wstring& data, int mode, bool use_action, bool multi_server);

private:
  bool SendCommands(const std::wstring& service, const std::vector<std::wstring>& commands);
};

////////////////////////////////////////////////////////////////////////////////
// Skype

enum SkypeConnectionStatus {
  kSkypeControlApiAttachSuccess,
  kSkypeControlApiAttachPendingAuthorization,
  kSkypeControlApiAttachRefused,
  kSkypeControlApiAttachNotAvailable,
  kSkypeControlApiAttachApiAvailable = 0x8001
};

class Skype {
public:
  Skype();
  virtual ~Skype();

  void Create();
  BOOL Discover();
  LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
  bool SendCommand(const std::wstring& command);
  bool GetMoodText();
  bool SetMoodText(const std::wstring& mood);

  static const UINT wm_attach;
  static const UINT wm_discover;

public:
  HWND hwnd, hwnd_skype;
  std::wstring current_mood, previous_mood;

private:
  class Window : public win::Window {
  private:
    void PreRegisterClass(WNDCLASSEX& wc);
    void PreCreate(CREATESTRUCT& cs);
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } window_;
};

////////////////////////////////////////////////////////////////////////////////
// Twitter

class Twitter {
public:
  Twitter();
  ~Twitter() {}

  bool RequestToken();
  bool AccessToken(const std::wstring& key, const std::wstring& secret, const std::wstring& pin);
  bool SetStatusText(const std::wstring& status_text);

  void HandleHttpResponse(HttpClientMode mode, const HttpResponse& response);

public:
  OAuth oauth;

private:
  std::wstring status_text_;
};

}  // namespace taiga

////////////////////////////////////////////////////////////////////////////////

extern taiga::Announcer Announcer;
extern taiga::Mirc Mirc;
extern taiga::Skype Skype;
extern taiga::Twitter Twitter;
