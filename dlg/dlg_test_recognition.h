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

#ifndef DLG_TEST_RECOGNITOIN_H
#define DLG_TEST_RECOGNITOIN_H

#include "../std.h"
#include "../animelist.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

/* Recognition test dialog */

class CTestRecognition : public CDialog {
public:
  CTestRecognition() {};
  ~CTestRecognition() {};

  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnSize(UINT uMsg, UINT nType, SIZE size);

  vector<CEpisode> m_EpisodeList, m_EpisodeListTest;
  CListView m_List;
};

extern CTestRecognition RecognitionTestWindow;

#endif // DLG_TEST_RECOGNITOIN_H