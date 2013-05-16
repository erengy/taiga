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

#ifndef SETTINGS_H
#define SETTINGS_H

#include "std.h"
#include "feed.h"
#include "optional.h"

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

class Settings {
public:
  bool Load();
  bool Save();

  // Account
  struct Account {
    // MyAnimeList
    struct Mal {
      BOOL auto_login;
      wstring password, user;
    } MAL;
    // Update
    struct Update {
      int delay, mode, time;
      BOOL check_player, out_of_range;
    } Update;
  } Account;

  // Anime
  struct Anime {
    void SetItem(int id, Optional<wstring> folder, Optional<wstring> titles);
    struct Item {
      int id;
      wstring folder, titles;
    };
    vector<Item> items;
  } Anime;
  // Folders
  struct Folders {
    vector<wstring> root;
    BOOL watch_enabled;
  } Folders;
  
  // Announcements
  struct Announce {
    // HTTP
    struct Http {
      BOOL enabled;
      wstring format, url;
    } HTTP;
    // Messenger
    struct Msn {
      BOOL enabled;
      wstring format;
    } MSN;
    // mIRC
    struct Mirc {
      BOOL enabled, multi_server, use_action;
      int mode;
      wstring channels, format, service;
    } MIRC;
    // Skype
    struct Skype {
      BOOL enabled;
      wstring format;
    } Skype;
    // Twitter
    struct Twitter {
      BOOL enabled;
      wstring format, oauth_key, oauth_secret, user;
    } Twitter;
  } Announce;

  // Program
  struct Program {
    // General
    struct General {
      BOOL auto_start, close, minimize;
      wstring theme;
    } General;
    // Position
    struct Position {
      int x, y, w, h;
      BOOL maximized;
    } Position;
    // Start-up
    struct StartUp {
      BOOL check_new_episodes, check_new_version, minimize;
    } StartUp;
    // Exit
    struct Exit {
      BOOL remember_pos_size;
    } Exit;
    // Proxy
    struct Proxy {
      wstring host, password, user;
    } Proxy;
    // List
    struct List {
      int double_click, middle_click;
      BOOL english_titles;
      BOOL highlight;
      BOOL new_episodes;
      int progress_mode;
      BOOL progress_show_eps;
    } List;
    // Balloon
    struct Balloon {
      BOOL enabled;
      wstring format;
    } Balloon;
  } Program;

  // Recognition
  struct Recognition {
    // Streaming
    struct Streaming {
      bool ann_enabled, crunchyroll_enabled, veoh_enabled,
        viz_enabled, youtube_enabled;
    } Streaming;
  } Recognition;

  // RSS
  struct Rss {
    // Torrent
    struct Torrent {
      BOOL check_enabled, create_folder, hide_unidentified, set_folder;
      int app_mode, check_interval, new_action;
      wstring app_path, download_path, search_url, source;
      struct Filters {
        BOOL global_enabled;
      } Filters;
    } Torrent;
  } RSS;

private:
  wstring file_, folder_;
};

extern Settings Settings;

#endif // SETTINGS_H