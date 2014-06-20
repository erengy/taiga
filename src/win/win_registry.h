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

#ifndef TAIGA_WIN_REGISTRY_H
#define TAIGA_WIN_REGISTRY_H

#include "win_main.h"

namespace win {

class Registry {
public:
  Registry();
  ~Registry();

  LSTATUS CloseKey();
  LSTATUS CreateKey(
      HKEY key,
      const std::wstring& subkey,
      LPWSTR class_type = nullptr,
      DWORD options = REG_OPTION_NON_VOLATILE,
      REGSAM sam_desired = KEY_SET_VALUE,
      LPSECURITY_ATTRIBUTES security_attributes = nullptr,
      LPDWORD disposition = nullptr);
  LONG DeleteKey(const std::wstring& subkey);
  LSTATUS DeleteValue(const std::wstring& value_name);
  void EnumKeys(std::vector<std::wstring>& output);
  LSTATUS OpenKey(
      HKEY key,
      const std::wstring& subkey,
      DWORD options = 0,
      REGSAM sam_desired = KEY_QUERY_VALUE);
  LSTATUS QueryValue(
      const std::wstring& value_name,
      LPDWORD type,
      LPBYTE data,
      LPDWORD data_size);
  std::wstring QueryValue(const std::wstring& value_name);
  LSTATUS SetValue(
      const std::wstring& value_name,
      DWORD type,
      CONST BYTE* data,
      DWORD data_size);
  void SetValue(
      const std::wstring& value_name,
      const std::wstring& value);

private:
  LSTATUS DeleteSubkeys(HKEY root_key, const std::wstring& subkey);

  HKEY key_;
};

}  // namespace win

#endif  // TAIGA_WIN_REGISTRY_H