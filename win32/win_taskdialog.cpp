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

#include "win_taskdialog.h"

namespace win32 {

vector<wstring> ButtonText;
LRESULT CALLBACK MsgBoxHookProc(int nCode, WPARAM wParam, LPARAM lParam);

// =============================================================================

TaskDialog::TaskDialog() {
  Initialize();
}

TaskDialog::TaskDialog(LPCWSTR title, LPWSTR icon) {
  Initialize();
  SetWindowTitle(title);
  SetMainIcon(icon);
}

HRESULT CALLBACK TaskDialog::Callback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData) {
  switch (uNotification) {
    case TDN_DIALOG_CONSTRUCTED:
      //::SetForegroundWindow(hwnd);
      //::BringWindowToTop(hwnd);
      break;
    case TDN_HYPERLINK_CLICKED:
      ::ShellExecute(NULL, NULL, (LPCWSTR)lParam, NULL, NULL, SW_SHOWNORMAL);
      break;
    case TDN_VERIFICATION_CLICKED: {
      auto dlg = reinterpret_cast<TaskDialog*>(dwRefData);
      dlg->m_VerificationChecked = static_cast<BOOL>(wParam) == TRUE;
      break;
    }
  }
  return S_OK;
}

void TaskDialog::Initialize() {
  ::ZeroMemory(&m_Config, sizeof(TASKDIALOGCONFIG));
  m_Config.cbSize = sizeof(TASKDIALOGCONFIG);
  m_Config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | 
                     TDF_ENABLE_HYPERLINKS | 
                     TDF_POSITION_RELATIVE_TO_WINDOW | 
                     TDF_SIZE_TO_CONTENT;
  m_Config.hInstance = ::GetModuleHandle(NULL);
  m_Config.lpCallbackData = reinterpret_cast<LONG_PTR>(this);
  m_Config.pfCallback = Callback;
  m_SelectedButtonID = 0;
  m_VerificationChecked = false;
}

// =============================================================================

void TaskDialog::AddButton(LPCWSTR text, int id) {
  m_Buttons.resize(m_Buttons.size() + 1);
  m_Buttons.back().pszButtonText = text;
  m_Buttons.back().nButtonID = id;
}

int TaskDialog::GetSelectedButtonID() const {
  return m_SelectedButtonID;
}

bool TaskDialog::GetVerificationCheck() const {
  return m_VerificationChecked;
}

void TaskDialog::SetCollapsedControlText(LPCWSTR text) {
  m_Config.pszCollapsedControlText = text;
}

void TaskDialog::SetContent(LPCWSTR text) {
  m_Config.pszContent = text;
}

void TaskDialog::SetExpandedControlText(LPCWSTR text) {
  m_Config.pszExpandedControlText = text;
}

void TaskDialog::SetExpandedInformation(LPCWSTR text) {
  m_Config.pszExpandedInformation = text;
}

void TaskDialog::SetFooter(LPCWSTR text) {
  m_Config.pszFooter = text;
}

void TaskDialog::SetFooterIcon(LPWSTR icon) {
  m_Config.dwFlags &= ~TDF_USE_HICON_FOOTER;
  m_Config.pszFooterIcon = icon;
}

void TaskDialog::SetMainIcon(LPWSTR icon) {
  m_Config.dwFlags &= ~TDF_USE_HICON_MAIN;
  m_Config.pszMainIcon = icon;
}

void TaskDialog::SetMainInstruction(LPCWSTR text) {
  m_Config.pszMainInstruction = text;
}

void TaskDialog::SetVerificationText(LPCWSTR text) {
  m_Config.pszVerificationText = text;
}

void TaskDialog::SetWindowTitle(LPCWSTR text) {
  m_Config.pszWindowTitle = text;
}

void TaskDialog::UseCommandLinks(bool use) {
  if (use) {
    m_Config.dwFlags |= TDF_USE_COMMAND_LINKS;
  } else {
    m_Config.dwFlags &= ~TDF_USE_COMMAND_LINKS;
  }
}

// =============================================================================

HRESULT TaskDialog::Show(HWND hParent) {
  m_Config.hwndParent = hParent;
  if (!m_Buttons.empty()) {
    m_Config.pButtons = &m_Buttons[0];
    m_Config.cButtons = static_cast<UINT>(m_Buttons.size());
    if (m_Buttons.size() > 1) {
      m_Config.dwFlags &= ~TDF_ALLOW_DIALOG_CANCELLATION;
    }
  }

  // Show task dialog, if available
  if (GetWinVersion() >= VERSION_VISTA) {
    BOOL VerificationFlagChecked = TRUE;
    return ::TaskDialogIndirect(&m_Config, &m_SelectedButtonID, NULL, &VerificationFlagChecked);
  
  // Fall back to normal message box
  } else {
    MSGBOXPARAMS msgbox;
    msgbox.cbSize = sizeof(MSGBOXPARAMS);
    // Set properties
    msgbox.hwndOwner = hParent;
    msgbox.hInstance = m_Config.hInstance;
    msgbox.lpszCaption = m_Config.pszWindowTitle;
    // Set message
    #define ADD_MSG(x) if (x) { msg += L"\n\n"; msg += x; }
    wstring msg = m_Config.pszMainInstruction;
    ADD_MSG(m_Config.pszContent);
    ADD_MSG(m_Config.pszExpandedInformation);
    ADD_MSG(m_Config.pszFooter);
    msgbox.lpszText = msg.c_str();
    // Set buttons
    ButtonText.resize(m_Config.cButtons);
    for (unsigned int i = 0; i < ButtonText.size(); i++) {
      ButtonText[i] = m_Buttons[i].pszButtonText;
      unsigned int pos = ButtonText[i].find(L"\n");
      if (pos != wstring::npos) ButtonText[i].resize(pos);
    }
    switch (m_Config.cButtons) {
      case 2:  msgbox.dwStyle = MB_YESNO; break;
      case 3:  msgbox.dwStyle = MB_YESNOCANCEL; break;
      default: msgbox.dwStyle = MB_OK;
    }
    // Set icon
    #define icon m_Config.pszMainIcon
    if (icon == TD_ICON_INFORMATION || icon == TD_ICON_SHIELD || icon == TD_ICON_SHIELD_GREEN) {
      msgbox.dwStyle |= (m_Config.cButtons > 1 ? MB_ICONQUESTION : MB_ICONINFORMATION);
    } else if (icon == TD_ICON_WARNING) {
      msgbox.dwStyle |= MB_ICONWARNING;
    } else if (icon == TD_ICON_ERROR || icon == TD_ICON_SHIELD_RED) {
      msgbox.dwStyle |= MB_ICONERROR;
    }
    #undef icon
    // Hook
    if (hParent == NULL) hParent = ::GetDesktopWindow();
    ::SetProp(hParent, L"MsgBoxHook", ::SetWindowsHookEx(WH_CALLWNDPROCRET, 
      MsgBoxHookProc, NULL, ::GetCurrentThreadId()));
    // Show message box
    m_SelectedButtonID = ::MessageBoxIndirect(&msgbox);
    // Unhook
    if (::GetProp(hParent, L"MsgBoxHook")) {
      ::UnhookWindowsHookEx(reinterpret_cast<HHOOK>(::RemoveProp(hParent, L"MsgBoxHook")));
    }
    // Return
    return S_OK;
  }
}

// MessageBox hook - used to change button text.
// Note that button width is fixed and does not change according to text length.
LRESULT CALLBACK MsgBoxHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
  HWND hwnd = reinterpret_cast<LPCWPRETSTRUCT>(lParam)->hwnd;
  if (!(nCode < 0)) {
    WCHAR szClassName[MAX_PATH];
    GetClassName(hwnd, szClassName, MAX_PATH);
    if (!lstrcmpi(szClassName, L"#32770")) {
      switch(((LPCWPRETSTRUCT)lParam)->message) {
        case WM_INITDIALOG:
          #define SETBUTTONTEXT(x, y) \
            SendMessage(GetDlgItem(hwnd, x), WM_SETTEXT, 0, reinterpret_cast<LPARAM>(y.c_str()))
          switch (ButtonText.size()) {
            case 2:
              SETBUTTONTEXT(IDYES, ButtonText[0]);
              SETBUTTONTEXT(IDNO,  ButtonText[1]);
              break;
            case 3:
              SETBUTTONTEXT(IDYES,    ButtonText[0]);
              SETBUTTONTEXT(IDNO,     ButtonText[1]);
              SETBUTTONTEXT(IDCANCEL, ButtonText[2]);
              break;
            default:
              SETBUTTONTEXT(IDCANCEL, ButtonText[0]);
          }
          #undef SETBUTTONTEXT
      }
    }
  }
  HWND hParent = GetParent(hwnd);
  if (hParent == NULL) hParent = GetDesktopWindow();
  return CallNextHookEx(reinterpret_cast<HHOOK>(GetProp(hParent, L"MsgBoxHook")), nCode, wParam, lParam);
}

// Fallback code is longer than TaskDialog code...  -_-'

} // namespace win32