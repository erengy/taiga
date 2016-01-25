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

#ifndef TAIGA_TAIGA_TAIGA_H
#define TAIGA_TAIGA_TAIGA_H

#include "base/version.h"
#include "taiga/update.h"
#include "win/win_main.h"

#define TAIGA_APP_NAME    L"Taiga"
#define TAIGA_APP_TITLE   L"Taiga"

#define TAIGA_PORTABLE

namespace taiga {

enum PlayStatus {
  kPlayStatusStopped,
  kPlayStatusPlaying,
  kPlayStatusUpdated
};

enum TipType {
  kTipTypeDefault,
  kTipTypeNowPlaying,
  kTipTypeSearch,
  kTipTypeTorrent,
  kTipTypeUpdateFailed,
  kTipTypeNotApproved,
};

class App : public win::App {
public:
  App();
  ~App();

  BOOL InitInstance();
  void Uninitialize();

  void LoadData();

  bool allow_multiple_instances;
  int current_tip_type, play_status;
  bool debug_mode;
  bool logged_in;
  base::SemanticVersion version;

  class Updater : public UpdateHelper {
  public:
    void OnCheck();
    void OnCRCCheck(const std::wstring& path, std::wstring& crc);
    void OnDone();
    void OnProgress(int file_index);
    bool OnRestartApp();
    void OnRunActions();
  } Updater;

private:
  void ParseCommandLineArguments();
};

}  // namespace taiga

extern taiga::App Taiga;

#endif  // TAIGA_TAIGA_TAIGA_H
