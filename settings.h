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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "std.h"
#include "feed.h"

enum MAL_API {
  MAL_API_NONE     = 1,
  MAL_API_OFFICIAL = 2
};

enum UpdateMode {
  UPDATE_MODE_NONE = 1,
  UPDATE_MODE_AUTO = 2,
  UPDATE_MODE_ASK  = 3
};

enum UpdateTime {
  UPDATE_TIME_INSTANT    = 1,
  UPDATE_MODE_WAITPLAYER = 2,
  UPDATE_MODE_AFTERDELAY = 3
};

enum ListProgressMode {
  LIST_PROGRESS_AVAILABLEEPS = 0,
  LIST_PROGRESS_QUEUEDEPS    = 1
};

// =============================================================================

class CSettings {
public:
  bool Read();
  bool Write();

  // Account
  class CSettingsAccount {
  public:
    // MyAnimeList
    class CSettingsAccountMAL {
    public:
      int API;
      BOOL AutoLogin;
      wstring Password, User;
    } MAL;
    // Update
    class CSettingsAccountUpdate {
    public:
      int Delay, Mode, Time;
      BOOL CheckPlayer, OutOfRange;
    } Update;
  } Account;

  // Anime
  class CSettingsAnime {
  public:
    void SetItem(int id, wstring folder, wstring titles);
    class CSettingsAnimeItem {
    public:
      int ID;
      wstring Folder, Titles;
    };
    vector<CSettingsAnimeItem> Items;
  } Anime;
  // Folders
  class CSettingsFolders {
  public:
    vector<wstring> Root;
    BOOL WatchEnabled;
  } Folders;
  
  // Announcements
  class CSettingsAnnounce {
  public:
    // HTTP
    class CSettingsAnnounceHTTP {
    public:
      BOOL Enabled;
      wstring Format, URL;
    } HTTP;
    // Messenger
    class CSettingsAnnounceMSN {
    public:
      BOOL Enabled;
      wstring Format;
    } MSN;
    // mIRC
    class CSettingsAnnounceMIRC {
    public:
      BOOL Enabled, MultiServer, UseAction;
      int Mode;
      wstring Channels, Format, Service;
    } MIRC;
    // Skype
    class CSettingsAnnounceSkype {
    public:
      BOOL Enabled;
      wstring Format;
    } Skype;
    // Twitter
    class CSettingsAnnounceTwitter {
    public:
      BOOL Enabled;
      wstring Format, OAuthKey, OAuthSecret, User;
    } Twitter;
  } Announce;

  // Program
  class CSettingsProgram {
  public:
    // General
    class CSettingsProgramGeneral {
    public:
      BOOL AutoStart, Close, Minimize;
      int SearchIndex, SizeX, SizeY;
      wstring Theme;
    } General;
    // Start-up
    class CSettingsProgramStartUp {
    public:
      BOOL CheckNewEpisodes, CheckNewVersion, Minimize;
    } StartUp;
    // Exit
    class CSettingsProgramExit {
    public:
      BOOL Ask, SaveBuffer;
    } Exit;
    // Proxy
    class CSettingsProgramProxy {
    public:
      wstring Host, Password, User;
    } Proxy;
    // List
    class CSettingsProgramList {
    public:
      int DoubleClick, MiddleClick;
      BOOL Highlight;
      int ProgressMode;
      BOOL ProgressShowEps;
    } List;
    // Balloon
    class CSettingsProgramBalloon {
    public:
      BOOL Enabled;
      wstring Format;
    } Balloon;
  } Program;

  // RSS
  class CSettingsRSS {
  public:
    // Torrent
    class CSettingsRSSTorrent {
    public:
      BOOL CheckEnabled, HideUnidentified, SetFolder;
      int AppMode, CheckInterval, NewAction;
      wstring AppPath, Source;
      class CSettingsRSSTorrentFilters {
      public:
        BOOL GlobalEnabled;
      } Filters;
    } Torrent;
  } RSS;

private:
  wstring m_File, m_Folder;
};

extern CSettings Settings;

#endif // SETTINGS_H