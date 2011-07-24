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

#ifndef TAIGA_H
#define TAIGA_H

#include "std.h"
#include "update.h"
#include "win32/win_main.h"

#define APP_NAME             L"Taiga"
#define APP_TITLE            L"Taiga 0.8"
#define APP_VERSION          L"0.8.104"
#define APP_BUILD            L"2011-07-24"
#define APP_VERSION_MAJOR    0
#define APP_VERSION_MINOR    8
#define APP_VERSION_REVISION 104

#ifndef PORTABLE
#define PORTABLE
#endif

enum PlayStatus {
  PLAYSTATUS_STOPPED,
  PLAYSTATUS_PLAYING,
  PLAYSTATUS_UPDATED
};

enum TipType {
  TIPTYPE_NORMAL,
  TIPTYPE_SEARCH,
  TIPTYPE_TORRENT,
  TIPTYPE_UPDATEFAILED
};

// =============================================================================

class CTaiga : public CApp {
public:
  CTaiga();
  ~CTaiga();
  
  BOOL CheckNewVersion(bool silent);
  wstring GetDataPath();
  BOOL InitInstance();
  void ReadData();
  
  bool LoggedIn, UpdatesEnabled;
  int CurrentTipType, PlayStatus;
  int TickerMedia, TickerNewEpisodes, TickerQueue;

  class CUpdate : public CUpdateHelper {
  public:
    CUpdate() {}
    virtual ~CUpdate() {}

    void OnCheck();
    void OnCRCCheck(const wstring& path, wstring& crc);
    void OnDone();
    void OnProgress(int file_index);
    bool OnRestartApp();
    void OnRunActions();
  } UpdateHelper;
};

extern CTaiga Taiga;

#endif // TAIGA_H