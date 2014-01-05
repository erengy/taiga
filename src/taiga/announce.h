/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#ifndef TAIGA_TAIGA_ANNOUNCE_H
#define TAIGA_TAIGA_ANNOUNCE_H

#include <string>

#include "third_party/oauth/oauth.h"
#include "win/win_window.h"

namespace anime {
class Episode;
}

namespace taiga {

enum AnnouncerModes {
  kAnnounceToHttp      = 0x01,
  kAnnounceToMessenger = 0x02,
  kAnnounceToMirc      = 0x04,
  kAnnounceToSkype     = 0x08,
  kAnnounceToTwitter   = 0x10
};

class Announcer {
public:
  void Clear(int modes, bool force = false);
  void Do(int modes, anime::Episode* episode = nullptr, bool force = false);

  bool TestMircConnection(const std::wstring& service);

private:
  void ToHttp(const std::wstring& address, const std::wstring& data);
  void ToMessenger(const std::wstring& artist, const std::wstring& album, const std::wstring& title, BOOL show);
  bool ToMirc(const std::wstring& service, std::wstring channels, const std::wstring& data, int mode, BOOL use_action, BOOL multi_server);
  void ToSkype(const std::wstring& mood);
  void ToTwitter(const std::wstring& status_text);
};

////////////////////////////////////////////////////////////////////////////////
// mIRC

enum MircChannelMode {
  kMircChannelModeActive,
  kMircChannelModeAll,
  kMircChannelModeCustom
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
  bool SendCommand(const wstring& command);
  bool GetMoodText();
  bool SetMoodText(const wstring& mood);

  static const UINT wm_attach;
  static const UINT wm_discover;

public:
  HWND hwnd, hwnd_skype;
  wstring current_mood, previous_mood;

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

public:
  COAuth oauth;

private:
  wstring status_text_;
};

}  // namespace taiga

////////////////////////////////////////////////////////////////////////////////

extern taiga::Announcer Announcer;
extern taiga::Skype Skype;
extern taiga::Twitter Twitter;

#endif  // TAIGA_TAIGA_ANNOUNCE_H