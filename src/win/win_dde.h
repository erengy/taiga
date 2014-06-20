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

#ifndef TAIGA_WIN_DDE_H
#define TAIGA_WIN_DDE_H

#include "win_main.h"

namespace win {

// A helper class to use Dynamic Data Exchange (DDE) protocol

class DynamicDataExchange {
public:
  DynamicDataExchange();
  ~DynamicDataExchange();

  BOOL Initialize(DWORD afCmd = APPCLASS_STANDARD | APPCMD_CLIENTONLY,
                  bool unicode = false);
  void UnInitialize();

  BOOL Connect(const std::wstring& service, const std::wstring& topic);
  void Disconnect();

  BOOL ClientTransaction(const std::wstring& item,
                         const std::wstring& data,
                         std::wstring* output,
                         UINT wType);
  BOOL IsAvailable();
  BOOL NameService(const std::wstring& service, UINT afCmd = DNS_REGISTER);

  virtual BOOL OnConnect() { return TRUE; }
  virtual void OnPoke() {}
  virtual void OnRequest() {}

private:
  HSZ CreateStringHandle(const std::wstring& str);
  void FreeStringHandle(HSZ str);

  static FNCALLBACK DdeCallback;

  HCONV conversation_;
  DWORD instance_;
  bool is_unicode_;
};

}  // namespace win

#endif  // TAIGA_WIN_DDE_H