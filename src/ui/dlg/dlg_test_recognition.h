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

#ifndef DLG_TEST_RECOGNITOIN_H
#define DLG_TEST_RECOGNITOIN_H

#include "base/std.h"

#include "library/anime_episode.h"

#include "win/ctrl/win_ctrl.h"
#include "win/win_dialog.h"

// =============================================================================

class RecognitionTestDialog : public win::Dialog {
public:
  RecognitionTestDialog() {};
  ~RecognitionTestDialog() {};

  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnSize(UINT uMsg, UINT nType, SIZE size);

private:
  class EpisodeTest : public anime::Episode {
  public:
    EpisodeTest() : priority(0) {}
    int priority;
  };
  vector<EpisodeTest> episodes_, test_episodes_;

  win::ListView list_;
};

extern RecognitionTestDialog RecognitionTest;

#endif // DLG_TEST_RECOGNITOIN_H