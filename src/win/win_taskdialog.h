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

#ifndef TAIGA_WIN_TASKDIALOG_H
#define TAIGA_WIN_TASKDIALOG_H

#include "win_main.h"

#define TD_ICON_NONE         static_cast<PCWSTR>(0)
#define TD_ICON_INFORMATION  TD_INFORMATION_ICON
#define TD_ICON_WARNING      TD_WARNING_ICON
#define TD_ICON_ERROR        TD_ERROR_ICON
#define TD_ICON_SHIELD       TD_SHIELD_ICON
#define TD_ICON_SHIELD_GREEN MAKEINTRESOURCE(-8)
#define TD_ICON_SHIELD_RED   MAKEINTRESOURCE(-7)

#define TDF_SIZE_TO_CONTENT 0x1000000

namespace win {

class TaskDialog {
public:
  TaskDialog();
  TaskDialog(LPCWSTR title, LPWSTR icon);
  virtual ~TaskDialog() {}

  void AddButton(LPCWSTR text, int id);
  int GetSelectedButtonID() const;
  bool GetVerificationCheck() const;
  void SetCollapsedControlText(LPCWSTR text);
  void SetContent(LPCWSTR text);
  void SetExpandedControlText(LPCWSTR text);
  void SetExpandedInformation(LPCWSTR text);
  void SetFooter(LPCWSTR LPCWSTR);
  void SetFooterIcon(LPWSTR icon);
  void SetMainIcon(LPWSTR icon);
  void SetMainInstruction(LPCWSTR text);
  void SetVerificationText(LPCWSTR text);
  void SetWindowTitle(LPCWSTR text);
  HRESULT Show(HWND parent);
  void UseCommandLinks(bool use);

protected:
  static HRESULT CALLBACK Callback(
      HWND hwnd, UINT uNotification,
      WPARAM wParam, LPARAM lParam,
      LONG_PTR dwRefData);

  void Initialize();

  std::vector<TASKDIALOG_BUTTON> buttons_;
  TASKDIALOGCONFIG config_;
  int selected_button_id_;
  bool verification_checked_;
};

}  // namespace win

#endif  // TAIGA_WIN_TASKDIALOG_H