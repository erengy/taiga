/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

#ifndef DDE_H
#define DDE_H

#include "std.h"

// =============================================================================

class CDDE {
public:
  CDDE();
  ~CDDE();
  
  BOOL ClientTransaction(const wstring& item, const wstring& data, wstring* output, UINT wType);
  BOOL Connect(const wstring& service, const wstring& topic);
  void Disconnect();
  BOOL Initialize(DWORD afCmd = APPCLASS_STANDARD | APPCMD_CLIENTONLY, BOOL unicode = FALSE);
  BOOL IsAvailable();
  BOOL NameService(const wstring& service, UINT afCmd = DNS_REGISTER);
  void UnInitialize();

  virtual BOOL OnConnect() { return TRUE; }
  virtual void OnPoke() {}
  virtual void OnRequest() {}

private:
  static FNCALLBACK DDECallback;

  BOOL m_bUnicode;
  DWORD m_dwInstance;
  HCONV m_hConversation;
};

#endif // DDE_H