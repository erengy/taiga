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

#include "win_accessibility.h"

namespace win {

AccessibleChild::AccessibleChild()
    : role(0) {
}

AccessibleObject::AccessibleObject()
    : acc_(nullptr),
      win_event_hook_(nullptr) {
}

AccessibleObject::~AccessibleObject() {
#ifdef _DEBUG
  Unhook();
#endif
  Release();
}

////////////////////////////////////////////////////////////////////////////////

HRESULT AccessibleObject::FromWindow(HWND hwnd, DWORD object_id) {
  if (hwnd == nullptr)
    return E_INVALIDARG;

  Release();

  return AccessibleObjectFromWindow(hwnd, object_id, IID_IAccessible,
                                    (void**)&acc_);
}

void AccessibleObject::Release() {
  if (acc_ != nullptr) {
    acc_->Release();
    acc_ = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////

HRESULT AccessibleObject::GetName(std::wstring& name, long child_id,
                                  IAccessible* acc) {
  if (acc == nullptr)
    acc = acc_;
  if (acc == nullptr)
    return E_INVALIDARG;

  VARIANT var_child;
  var_child.vt = VT_I4;
  var_child.lVal = child_id;

  BSTR buffer;
  HRESULT hr = acc->get_accName(var_child, &buffer);

  if (buffer) {
    name = buffer;
  } else {
    name.clear();
  }
  SysFreeString(buffer);

  return hr;
}

HRESULT AccessibleObject::GetRole(DWORD& role, long child_id,
                                  IAccessible* acc) {
  if (acc == nullptr)
    acc = acc_;
  if (acc == nullptr)
    return E_INVALIDARG;

  VARIANT var_child;
  var_child.vt = VT_I4;
  var_child.lVal = child_id;

  VARIANT var_result;
  HRESULT hr = acc->get_accRole(var_child, &var_result);

  if (hr == S_OK && var_result.vt == VT_I4) {
    role = var_result.lVal;
    return S_OK;
  }

  return E_FAIL;
}

HRESULT AccessibleObject::GetRole(std::wstring& role, long child_id,
                                  IAccessible* acc) {
  DWORD role_id = 0;

  HRESULT hr = GetRole(role_id, child_id, acc);

  if (hr != S_OK)
    return hr;

  UINT role_length = GetRoleText(role_id, nullptr, 0);
  LPTSTR buffer = (LPTSTR)malloc((role_length + 1) * sizeof(TCHAR));

  if (buffer != nullptr) {
    GetRoleText(role_id, buffer, role_length + 1);
    if (buffer) {
      role = buffer;
    } else {
      role.clear();
    }
    free(buffer);
  } else {
    return E_OUTOFMEMORY;
  }

  return S_OK;
}

HRESULT AccessibleObject::GetValue(std::wstring& value, long child_id,
                                   IAccessible* acc) {
  if (acc == nullptr)
    acc = acc_;
  if (acc == nullptr)
    return E_INVALIDARG;

  VARIANT var_child;
  var_child.vt = VT_I4;
  var_child.lVal = child_id;

  BSTR buffer;
  HRESULT hr = acc->get_accValue(var_child, &buffer);

  if (buffer) {
    value = buffer;
  } else {
    value.clear();
  }
  SysFreeString(buffer);

  return hr;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT AccessibleObject::GetChildCount(long* child_count, IAccessible* acc) {
  if (acc == nullptr)
    acc = acc_;
  if (acc == nullptr)
    return E_INVALIDARG;

  return acc->get_accChildCount(child_count);
}

HRESULT AccessibleObject::BuildChildren(std::vector<AccessibleChild>& children,
                                        IAccessible* acc, LPARAM param) {
  if (acc == nullptr)
    acc = acc_;
  if (acc == nullptr)
    return E_INVALIDARG;

  long child_count = 0;
  HRESULT hr = acc->get_accChildCount(&child_count);

  if (FAILED(hr))
    return hr;
  if (child_count == 0)
    return S_FALSE;

  long obtained_count = 0;
  std::vector<VARIANT> var_array(child_count);
  hr = AccessibleChildren(acc, 0L, child_count, var_array.data(),
                          &obtained_count);

  if (FAILED(hr))
    return hr;

  children.resize(obtained_count);
  for (int i = 0; i < obtained_count; i++) {
    VARIANT var_child = var_array[i];

    if (var_child.vt == VT_DISPATCH) {
      IDispatch* dispatch = var_child.pdispVal;
      IAccessible* child = nullptr;
      hr = dispatch->QueryInterface(IID_IAccessible, (void**)&child);
      if (hr == S_OK) {
        GetName(children.at(i).name, CHILDID_SELF, child);
        GetRole(children.at(i).role, CHILDID_SELF, child);
        GetValue(children.at(i).value, CHILDID_SELF, child);
        if (AllowChildTraverse(children.at(i), param))
          BuildChildren(children.at(i).children, child, param);
        child->Release();
      }
      dispatch->Release();

    } else {
      GetName(children.at(i).name, var_child.lVal, acc);
      GetRole(children.at(i).role, var_child.lVal, acc);
      GetValue(children.at(i).value, var_child.lVal, acc);
    }
  }

  return S_OK;
}

bool AccessibleObject::AllowChildTraverse(AccessibleChild& child,
                                          LPARAM param) {
  return true;
}

////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
HWINEVENTHOOK AccessibleObject::Hook(DWORD eventMin, DWORD eventMax,
                                     HMODULE hmodWinEventProc,
                                     WINEVENTPROC pfnWinEventProc,
                                     DWORD idProcess, DWORD idThread,
                                     DWORD dwFlags) {
  Unhook();

  win_event_hook_ = SetWinEventHook(eventMin, eventMax,
                                    hmodWinEventProc,
                                    pfnWinEventProc,
                                    idProcess, idThread,
                                    dwFlags);

  return win_event_hook_;
}

bool AccessibleObject::IsHooked() {
  return win_event_hook_ != nullptr;
}

void AccessibleObject::Unhook() {
  if (win_event_hook_ != nullptr) {
    UnhookWinEvent(win_event_hook_);
    win_event_hook_ = nullptr;
  }
}

void CALLBACK WinEventFunc(HWINEVENTHOOK hook, DWORD dwEvent, HWND hwnd,
                           LONG idObject, LONG idChild,
                           DWORD dwEventThread, DWORD dwmsEventTime) {
  IAccessible* acc = nullptr;
  VARIANT var_child;
  HRESULT hr = AccessibleObjectFromEvent(hwnd, idObject, idChild, &acc,
                                         &var_child);

  if (hr == S_OK && acc != nullptr) {
    BSTR name;
    acc->get_accName(var_child, &name);

    switch (dwEvent) {
      case EVENT_SYSTEM_FOREGROUND:
      case EVENT_SYSTEM_ALERT:
      case EVENT_OBJECT_FOCUS:
      case EVENT_OBJECT_SELECTION:
      case EVENT_OBJECT_VALUECHANGE:
        break;
    }

    SysFreeString(name);
    acc->Release();
  }
}
#endif

}  // namespace win