/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

// =============================================================================

/* HTTP */

void AnnounceToHTTP(wstring address, wstring data);

// =============================================================================

/* Messenger */

void AnnounceToMessenger(wstring artist, wstring album, wstring title, BOOL show);

// =============================================================================

/* mIRC */

enum MircChannelMode {
  MIRC_CHANNELMODE_ACTIVE = 1,
  MIRC_CHANNELMODE_ALL    = 2,
  MIRC_CHANNELMODE_CUSTOM = 3
};

BOOL AnnounceToMIRC(wstring service, wstring channels, wstring data, int mode, BOOL use_action, BOOL multi_server);
BOOL TestMIRCConnection(wstring service);

// =============================================================================

/* Skype */

class Skype {
public:
  Skype();
  virtual ~Skype() {}
  
  BOOL Attach();
  BOOL ChangeMood();

public:
  HWND api_window_handle;
  wstring mood;
  UINT control_api_attach;
  UINT control_api_discover;
};

extern Skype Skype;
void AnnounceToSkype(const wstring& mood);

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
void AnnounceToTwitter(const wstring& status_text);

#endif // ANNOUNCE_H