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

#ifndef WIN_REGISTRY_H
#define WIN_REGISTRY_H

#include "win_main.h"

namespace win {

// =============================================================================

class Registry {
public:
  Registry();
  ~Registry();

  LSTATUS CloseKey();
  LSTATUS CreateKey(HKEY key, const wstring& sub_key, 
    LPWSTR lpClass = NULL, 
    DWORD dwOptions = REG_OPTION_NON_VOLATILE, 
    REGSAM samDesired = KEY_SET_VALUE, 
    LPSECURITY_ATTRIBUTES lpSecurityAttributes = NULL, 
    LPDWORD lpdwDisposition = NULL);
  LONG DeleteKey(const wstring& sub_key);
  LSTATUS DeleteValue(const wstring& value_name);
  void EnumKeys(vector<wstring>& output);
  LSTATUS OpenKey(HKEY key, const wstring& sub_key, 
    DWORD ulOptions = 0, 
    REGSAM samDesired = KEY_QUERY_VALUE);
  LSTATUS QueryValue(const wstring& value_name, 
    LPDWORD lpType, 
    LPBYTE lpData, 
    LPDWORD lpcbData);
  wstring QueryValue(const wstring& value_name);
  LSTATUS SetValue(const wstring& value_name, 
    DWORD dwType, 
    CONST BYTE* lpData, 
    DWORD cbData);
  void SetValue(const wstring& value_name, const wstring& value);

private:
  LSTATUS DeleteSubkeys(HKEY root_key, wstring sub_key);

  HKEY m_Key;
};

}  // namespace win

#endif // WIN_REGISTRY_H