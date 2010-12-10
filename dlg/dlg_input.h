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

#ifndef DLG_INPUT_H
#define DLG_INPUT_H

#include "../std.h"
#include "../win32/win_dialog.h"

// =============================================================================

/* Input dialog */

class CInputDialog : public CDialog {
public:
  CInputDialog();
  virtual ~CInputDialog() {}

  INT_PTR Result;
  wstring Info, Title, Text;
  void SetNumbers(bool enabled, int min_value, int max_value, int current_value);
  void Show(HWND hParent);

  void OnCancel();
  BOOL OnInitDialog();
  void OnOK();

private:
  bool m_NumbersOnly;
  int m_CurrentValue, m_MinValue, m_MaxValue;
};

#endif // DLG_INPUT_H