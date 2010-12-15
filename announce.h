/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

void AnnounceToHTTP(wstring url, wstring data);

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

class CSkype {
public:
  CSkype();
  virtual ~CSkype() {}
  
  BOOL Attach();
  BOOL ChangeMood();

  HWND m_APIWindowHandle;
  wstring m_Mood;
  UINT m_uControlAPIAttach;
  UINT m_uControlAPIDiscover;
};

extern CSkype Skype;
void AnnounceToSkype(wstring mood);

// =============================================================================

/* Twitter */

class CTwitter {
public:
  CTwitter();
  virtual ~CTwitter() {}

  COAuth OAuth;

  bool RequestToken();
  bool AccessToken(const wstring& key, const wstring& secret, const wstring& pin);
  bool SetStatusText(const wstring& status_text);

private:
  wstring m_StatusText;
};

extern CTwitter Twitter;

void AnnounceToTwitter(wstring status_text);

#endif // ANNOUNCE_H