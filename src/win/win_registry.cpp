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

#include "win_registry.h"

namespace win {

Registry::Registry()
    : key_(nullptr) {
}

Registry::~Registry() {
  CloseKey();
}

LSTATUS Registry::CloseKey() {
  if (key_) {
    LSTATUS status = ::RegCloseKey(key_);
    key_ = nullptr;
    return status;
  } else {
    return ERROR_SUCCESS;
  }
}

LSTATUS Registry::CreateKey(HKEY key,
                            const std::wstring& subkey,
                            LPWSTR class_type,
                            DWORD options,
                            REGSAM sam_desired,
                            LPSECURITY_ATTRIBUTES security_attributes,
                            LPDWORD disposition) {
  CloseKey();

  return ::RegCreateKeyEx(
      key, subkey.c_str(), 0, class_type, options, sam_desired,
      security_attributes, &key_, disposition);
}

LONG Registry::DeleteKey(const std::wstring& subkey) {
  return DeleteSubkeys(key_, subkey);
}

LSTATUS Registry::DeleteSubkeys(HKEY root_key, const std::wstring& subkey) {
  LSTATUS status = ::RegDeleteKey(root_key, subkey.c_str());

  if (status == ERROR_SUCCESS)
    return status;

  HKEY key;
  status = ::RegOpenKeyEx(root_key, subkey.c_str(), 0, KEY_READ, &key);

  if (status != ERROR_SUCCESS)
    return status;

  DWORD subkey_count = 0;
  std::vector<std::wstring> keys;
  WCHAR name[MAX_PATH] = {L'\0'};

  if (::RegQueryInfoKey(key, nullptr, nullptr, nullptr, &subkey_count,
                        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                        nullptr) == ERROR_SUCCESS) {
    for (DWORD i = 0; i < subkey_count; i++) {
      DWORD name_length = MAX_PATH;
      if (::RegEnumKeyEx(key, i, name, &name_length, nullptr, nullptr, nullptr,
                         nullptr) == ERROR_SUCCESS) {
        keys.push_back(name);
      }
    }
  }

  for (auto it = keys.begin(); it != keys.end(); ++it) {
    DeleteSubkeys(key, it->c_str());
  }

  ::RegCloseKey(key);

  return ::RegDeleteKey(root_key, subkey.c_str());
}

LSTATUS Registry::DeleteValue(const std::wstring& value_name) {
  return ::RegDeleteValue(key_, value_name.c_str());
}

void Registry::EnumKeys(std::vector<std::wstring>& output) {
  // sam_desired must be: KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS

  WCHAR name[MAX_PATH] = {0};
  DWORD subkey_count = 0;

  if (::RegQueryInfoKey(key_, nullptr, nullptr, nullptr, &subkey_count,
                        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                        nullptr) == ERROR_SUCCESS) {
    for (DWORD i = 0; i < subkey_count; i++) {
      DWORD name_length = MAX_PATH;
      if (::RegEnumKeyEx(key_, i, name, &name_length, nullptr, nullptr, nullptr,
                         nullptr) == ERROR_SUCCESS) {
        output.push_back(name);
      }
    }
  }
}

LSTATUS Registry::OpenKey(HKEY key, const std::wstring& subkey, DWORD options,
                          REGSAM sam_desired) {
  CloseKey();

  return ::RegOpenKeyEx(key, subkey.c_str(), options, sam_desired, &key_);
}

LSTATUS Registry::QueryValue(const std::wstring& value_name, LPDWORD type,
                             LPBYTE data, LPDWORD data_size) {
  return ::RegQueryValueEx(key_, value_name.c_str(), nullptr, type,
                           data, data_size);
}

LSTATUS Registry::SetValue(const std::wstring& value_name, DWORD type,
                           CONST BYTE* data, DWORD data_size) {
  return ::RegSetValueEx(key_, value_name.c_str(), 0, type,
                         data, data_size);
}

std::wstring Registry::QueryValue(const std::wstring& value_name) {
  DWORD type = 0;
  WCHAR buffer[MAX_PATH];
  DWORD buffer_size = sizeof(buffer);

  if (QueryValue(value_name.c_str(), &type, reinterpret_cast<LPBYTE>(&buffer),
                 &buffer_size) == ERROR_SUCCESS)
    if (type == REG_SZ)
      return buffer;

  return std::wstring();
}

void Registry::SetValue(const std::wstring& value_name,
                        const std::wstring& value) {
  SetValue(value_name.c_str(), REG_SZ, (LPBYTE)value.c_str(),
           value.length() * sizeof(WCHAR));
}

}  // namespace win