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

#include "win_taskdialog.h"

namespace win {

std::vector<std::wstring> button_text;

LRESULT CALLBACK MsgBoxHookProc(int nCode, WPARAM wParam, LPARAM lParam);

TaskDialog::TaskDialog() {
  Initialize();
}

TaskDialog::TaskDialog(LPCWSTR title, LPWSTR icon) {
  Initialize();
  SetWindowTitle(title);
  SetMainIcon(icon);
}

HRESULT CALLBACK TaskDialog::Callback(HWND hwnd, UINT uNotification,
                                      WPARAM wParam, LPARAM lParam,
                                      LONG_PTR dwRefData) {
  switch (uNotification) {
    case TDN_DIALOG_CONSTRUCTED:
//    ::SetForegroundWindow(hwnd);
//    ::BringWindowToTop(hwnd);
      break;
    case TDN_HYPERLINK_CLICKED:
      ::ShellExecute(nullptr, nullptr, reinterpret_cast<LPCWSTR>(lParam),
                     nullptr, nullptr, SW_SHOWNORMAL);
      break;
    case TDN_VERIFICATION_CLICKED: {
      auto dlg = reinterpret_cast<TaskDialog*>(dwRefData);
      dlg->verification_checked_ = static_cast<BOOL>(wParam) == TRUE;
      break;
    }
  }

  return S_OK;
}

void TaskDialog::Initialize() {
  ::ZeroMemory(&config_, sizeof(TASKDIALOGCONFIG));

  config_.cbSize = sizeof(TASKDIALOGCONFIG);
  config_.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION |
                    TDF_ENABLE_HYPERLINKS |
                    TDF_POSITION_RELATIVE_TO_WINDOW |
                    TDF_SIZE_TO_CONTENT;
  config_.hInstance = ::GetModuleHandle(nullptr);
  config_.lpCallbackData = reinterpret_cast<LONG_PTR>(this);
  config_.pfCallback = Callback;

  selected_button_id_ = 0;
  verification_checked_ = false;
}

////////////////////////////////////////////////////////////////////////////////

void TaskDialog::AddButton(LPCWSTR text, int id) {
  buttons_.resize(buttons_.size() + 1);
  buttons_.back().pszButtonText = text;
  buttons_.back().nButtonID = id;
}

int TaskDialog::GetSelectedButtonID() const {
  return selected_button_id_;
}

bool TaskDialog::GetVerificationCheck() const {
  return verification_checked_;
}

void TaskDialog::SetCollapsedControlText(LPCWSTR text) {
  config_.pszCollapsedControlText = text;
}

void TaskDialog::SetContent(LPCWSTR text) {
  config_.pszContent = text;
}

void TaskDialog::SetExpandedControlText(LPCWSTR text) {
  config_.pszExpandedControlText = text;
}

void TaskDialog::SetExpandedInformation(LPCWSTR text) {
  config_.pszExpandedInformation = text;
}

void TaskDialog::SetFooter(LPCWSTR text) {
  config_.pszFooter = text;
}

void TaskDialog::SetFooterIcon(LPWSTR icon) {
  config_.dwFlags &= ~TDF_USE_HICON_FOOTER;
  config_.pszFooterIcon = icon;
}

void TaskDialog::SetMainIcon(LPWSTR icon) {
  config_.dwFlags &= ~TDF_USE_HICON_MAIN;
  config_.pszMainIcon = icon;
}

void TaskDialog::SetMainInstruction(LPCWSTR text) {
  config_.pszMainInstruction = text;
}

void TaskDialog::SetVerificationText(LPCWSTR text) {
  config_.pszVerificationText = text;
}

void TaskDialog::SetWindowTitle(LPCWSTR text) {
  config_.pszWindowTitle = text;
}

void TaskDialog::UseCommandLinks(bool use) {
  if (use) {
    config_.dwFlags |= TDF_USE_COMMAND_LINKS;
  } else {
    config_.dwFlags &= ~TDF_USE_COMMAND_LINKS;
  }
}

////////////////////////////////////////////////////////////////////////////////

HRESULT TaskDialog::Show(HWND parent) {
  config_.hwndParent = parent;
  if (!buttons_.empty()) {
    config_.pButtons = &buttons_[0];
    config_.cButtons = static_cast<UINT>(buttons_.size());
    if (buttons_.size() > 1)
      config_.dwFlags &= ~TDF_ALLOW_DIALOG_CANCELLATION;
  }

  // Show task dialog, if available
  if (GetVersion() >= kVersionVista) {
    BOOL verification_flag_checked = TRUE;
    return ::TaskDialogIndirect(&config_, &selected_button_id_, nullptr,
                                &verification_flag_checked);

  // Fall back to normal message box
  } else {
    MSGBOXPARAMS msgbox;
    msgbox.cbSize = sizeof(MSGBOXPARAMS);
    msgbox.hwndOwner = parent;
    msgbox.hInstance = config_.hInstance;
    msgbox.lpszCaption = config_.pszWindowTitle;

    // Set message
    #define ADD_MSG(x) \
        if (x) { msg += L"\n\n"; msg += x; }
    std::wstring msg = config_.pszMainInstruction;
    ADD_MSG(config_.pszContent);
    ADD_MSG(config_.pszExpandedInformation);
    ADD_MSG(config_.pszFooter);
    #undef ADD_MSG
    msgbox.lpszText = msg.c_str();

    // Set buttons
    button_text.resize(config_.cButtons);
    for (unsigned int i = 0; i < button_text.size(); i++) {
      button_text[i] = buttons_[i].pszButtonText;
      unsigned int pos = button_text[i].find(L"\n");
      if (pos != std::wstring::npos)
        button_text[i].resize(pos);
    }
    switch (config_.cButtons) {
      case 2:
        msgbox.dwStyle = MB_YESNO;
        break;
      case 3:
        msgbox.dwStyle = MB_YESNOCANCEL;
        break;
      default:
        msgbox.dwStyle = MB_OK;
        break;
    }

    // Set icon
    if (config_.pszMainIcon == TD_ICON_INFORMATION ||
        config_.pszMainIcon == TD_ICON_SHIELD ||
        config_.pszMainIcon == TD_ICON_SHIELD_GREEN) {
      msgbox.dwStyle |=
          (config_.cButtons > 1 ? MB_ICONQUESTION : MB_ICONINFORMATION);
    } else if (config_.pszMainIcon == TD_ICON_WARNING) {
      msgbox.dwStyle |= MB_ICONWARNING;
    } else if (config_.pszMainIcon == TD_ICON_ERROR ||
               config_.pszMainIcon == TD_ICON_SHIELD_RED) {
      msgbox.dwStyle |= MB_ICONERROR;
    }

    // Hook
    if (!parent)
      parent = ::GetDesktopWindow();
    ::SetProp(parent, L"MsgBoxHook", ::SetWindowsHookEx(WH_CALLWNDPROCRET,
              MsgBoxHookProc, nullptr, ::GetCurrentThreadId()));

    // Show message box
    selected_button_id_ = ::MessageBoxIndirect(&msgbox);

    // Unhook
    if (::GetProp(parent, L"MsgBoxHook"))
      ::UnhookWindowsHookEx(reinterpret_cast<HHOOK>(::RemoveProp(
          parent, L"MsgBoxHook")));

    return S_OK;
  }
}

// MessageBox hook - used to change button text.
// Note that button width is fixed and does not change according to text length.
LRESULT CALLBACK MsgBoxHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  auto cwpr = reinterpret_cast<LPCWPRETSTRUCT>(lParam);
  HWND hwnd = cwpr->hwnd;

  if (!(nCode < 0)) {
    WCHAR class_name[MAX_PATH];
    GetClassName(hwnd, class_name, MAX_PATH);

    if (!lstrcmpi(class_name, L"#32770")) {
      switch (cwpr->message) {
        case WM_INITDIALOG:
          #define SETBUTTONTEXT(x, y) \
            SendMessage(GetDlgItem(hwnd, x), WM_SETTEXT, 0, \
                        reinterpret_cast<LPARAM>(y.c_str()))
          switch (button_text.size()) {
            case 2:
              SETBUTTONTEXT(IDYES, button_text[0]);
              SETBUTTONTEXT(IDNO, button_text[1]);
              break;
            case 3:
              SETBUTTONTEXT(IDYES, button_text[0]);
              SETBUTTONTEXT(IDNO, button_text[1]);
              SETBUTTONTEXT(IDCANCEL, button_text[2]);
              break;
            default:
              SETBUTTONTEXT(IDCANCEL, button_text[0]);
          }
          #undef SETBUTTONTEXT
      }
    }
  }

  HWND parent = GetParent(hwnd);
  if (parent == nullptr)
    parent = GetDesktopWindow();

  return CallNextHookEx(reinterpret_cast<HHOOK>(GetProp(parent, L"MsgBoxHook")),
                        nCode, wParam, lParam);
}

}  // namespace win