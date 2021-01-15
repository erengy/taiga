/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include "taiga/resource.h"
#include "ui/dlg/dlg_input.h"

namespace ui {

InputDialog::InputDialog()
    : current_value_(0), min_value_(0), max_value_(0),
      numbers_only_(false), password_(false), result(0) {
  info = L"Set new value:";
  title = L"Input";
};

void InputDialog::SetNumbers(bool enabled, int min_value, int max_value,
                             int current_value) {
  numbers_only_ = enabled;
  min_value_ = min_value;
  max_value_ = max_value;
  current_value_ = current_value;
}

void InputDialog::SetPassword(bool enabled) {
  password_ = enabled;
}

void InputDialog::Show(HWND parent) {
  result = Create(IDD_INPUT, parent, true);
}

////////////////////////////////////////////////////////////////////////////////

BOOL InputDialog::OnInitDialog() {
  // Set dialog title
  SetWindowText(GetWindowHandle(), title.c_str());

  // Set information text
  SetDlgItemText(IDC_STATIC_INPUTINFO, info.c_str());

  // Set text style and properties
  edit_.Attach(GetDlgItem(IDC_EDIT_INPUT));
  spin_.Attach(GetDlgItem(IDC_SPIN_INPUT));
  if (numbers_only_) {
    edit_.SetStyle(ES_NUMBER, 0);
    spin_.SetBuddy(edit_.GetWindowHandle());
    spin_.SetRange32(min_value_, max_value_ > 0 ? max_value_ : 9999);
    spin_.SetPos32(current_value_);
  } else {
    edit_.SetStyle(0, ES_NUMBER);
    edit_.SetText(text.c_str());
    spin_.Enable(FALSE);
    spin_.Hide();
  }
  edit_.SetPasswordChar(password_ ? L'\u25cf' : 0);  // black circle
  edit_.SetSel(0, -1);

  return TRUE;
}

void InputDialog::OnCancel() {
  EndDialog(IDCANCEL);
}

void InputDialog::OnOK() {
  edit_.GetText(text);
  EndDialog(IDOK);
}

}  // namespace ui