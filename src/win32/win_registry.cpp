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

#include "win_registry.h"

namespace win32 {

// =============================================================================

Registry::Registry() :
  m_Key(NULL)
{
}

Registry::~Registry() {
  CloseKey();
}

LSTATUS Registry::CloseKey() {
  if (m_Key) {
    LSTATUS status = ::RegCloseKey(m_Key);
    m_Key = NULL;
    return status;
  } else {
    return ERROR_SUCCESS;
  }
}

LSTATUS Registry::CreateKey(HKEY key, const wstring& sub_key, LPWSTR lpClass, DWORD dwOptions, REGSAM samDesired, 
                             LPSECURITY_ATTRIBUTES lpSecurityAttributes, LPDWORD lpdwDisposition) {
  CloseKey();
  return ::RegCreateKeyEx(key, sub_key.c_str(), 0, lpClass, dwOptions, samDesired, 
    lpSecurityAttributes, &m_Key, lpdwDisposition);
}

LONG Registry::DeleteKey(const wstring& sub_key) {
  return DeleteSubkeys(m_Key, sub_key);
}

LSTATUS Registry::DeleteSubkeys(HKEY root_key, wstring sub_key) {
  LSTATUS status = ::RegDeleteKey(root_key, sub_key.c_str());
  if (status == ERROR_SUCCESS) return status;

  HKEY key;
  status = ::RegOpenKeyEx(root_key, sub_key.c_str(), 0, KEY_READ, &key);
  if (status != ERROR_SUCCESS) return status;
  DWORD cSubKeys = 0;
  vector<wstring> keys;
  WCHAR name[MAX_PATH] = {0};
  if (::RegQueryInfoKey(key, NULL, NULL, NULL, &cSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
    for (DWORD i = 0; i < cSubKeys; i++) {
      DWORD dwcName = MAX_PATH;
      if (::RegEnumKeyEx(key, i, name, &dwcName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        keys.push_back(name);
      }
    }
  }
  for (unsigned int i = 0; i < keys.size(); i++) {
    DeleteSubkeys(key, keys[i].c_str());
  }
  ::RegCloseKey(key);

  return ::RegDeleteKey(root_key, sub_key.c_str());
}

LSTATUS Registry::DeleteValue(const wstring& value_name) {
  return ::RegDeleteValue(m_Key, value_name.c_str());
}

void Registry::EnumKeys(vector<wstring>& output) {
  // samDesired must be: KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS
  WCHAR name[MAX_PATH] = {0};
  DWORD cSubKeys = 0;
  if (::RegQueryInfoKey(m_Key, NULL, NULL, NULL, &cSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
    for (DWORD i = 0; i < cSubKeys; i++) {
      DWORD dwcName = MAX_PATH;
      if (::RegEnumKeyEx(m_Key, i, name, &dwcName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        output.push_back(name);
      }
    }
  }
}

LSTATUS Registry::OpenKey(HKEY key, const wstring& sub_key, DWORD ulOptions, REGSAM samDesired) {
  CloseKey();
  return ::RegOpenKeyEx(key, sub_key.c_str(), ulOptions, samDesired, &m_Key);
}

LSTATUS Registry::QueryValue(const wstring& value_name, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
  return ::RegQueryValueEx(m_Key, value_name.c_str(), NULL, lpType, lpData, lpcbData);
}

LSTATUS Registry::SetValue(const wstring& value_name, DWORD dwType, CONST BYTE* lpData, DWORD cbData) {
  return ::RegSetValueEx(m_Key, value_name.c_str(), 0, dwType, lpData, cbData);
}

wstring Registry::QueryValue(const wstring& value_name) {
  DWORD dwType = 0;
  WCHAR szBuffer[MAX_PATH];
  DWORD dwBufferSize = sizeof(szBuffer);
  if (QueryValue(value_name.c_str(), &dwType, (LPBYTE)&szBuffer, &dwBufferSize) == ERROR_SUCCESS) {
    if (dwType == REG_SZ) return szBuffer;
  }
  return L"";
}

void Registry::SetValue(const wstring& value_name, const wstring& value) {
  SetValue(value_name.c_str(), REG_SZ, (LPBYTE)value.c_str(), value.length() * sizeof(WCHAR));
}

} // namespace win32