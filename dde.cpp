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

#include "std.h"
#include "dde.h"
#include "string.h"

// =============================================================================

CDDE::CDDE() {
  m_bUnicode = FALSE;
  m_dwInstance = 0;
  m_hConversation = NULL;
}

CDDE::~CDDE() {
  Disconnect();
  UnInitialize();
}

BOOL CDDE::ClientTransaction(const wstring& item, const wstring& data, wstring* output, UINT wType) {
  HDDEDATA hData = NULL;
  HSZ hszItem = NULL;
  DWORD dwResult = 0;
  if (m_bUnicode) {
    if (wType != XTYP_EXECUTE) {
      hszItem = ::DdeCreateStringHandleW(m_dwInstance, item.c_str(), CP_WINUNICODE);
    }
    hData = ::DdeClientTransaction((LPBYTE)data.data(), (data.size() + 1) * sizeof(WCHAR), m_hConversation, 
      hszItem, CF_UNICODETEXT, wType, 3000, &dwResult);
  } else {
    string item_ansi = ToANSI(item), data_ansi = ToANSI(data);
    if (wType != XTYP_EXECUTE) {
      hszItem = ::DdeCreateStringHandleA(m_dwInstance, item_ansi.c_str(), CP_WINANSI);
    }
    hData = ::DdeClientTransaction((LPBYTE)data_ansi.data(), data_ansi.size() + 1, m_hConversation, 
      hszItem, CF_TEXT, wType, 3000, &dwResult);
  }
  ::DdeFreeStringHandle(m_dwInstance, hszItem);
  if (output) {
    char szResult[255];
    ::DdeGetData(hData, (unsigned char*)szResult, 255, 0);
    output->assign(ToUTF8(szResult));
  }
  return hData != 0;
}

BOOL CDDE::Connect(const wstring& service, const wstring& topic) {
  if (m_dwInstance) {
    HSZ hszService = NULL, hszTopic = NULL;
    if (m_bUnicode) {
      hszService = ::DdeCreateStringHandleW(m_dwInstance, service.c_str(), CP_WINUNICODE);
      hszTopic = ::DdeCreateStringHandleW(m_dwInstance, topic.c_str(), CP_WINUNICODE);
    } else {
      hszService = ::DdeCreateStringHandleA(m_dwInstance, ToANSI(service), CP_WINANSI);
      hszTopic = ::DdeCreateStringHandleA(m_dwInstance, ToANSI(topic), CP_WINANSI);
    }
    m_hConversation = ::DdeConnect(m_dwInstance, hszService, hszTopic, NULL);
    ::DdeFreeStringHandle(m_dwInstance, hszService);
    ::DdeFreeStringHandle(m_dwInstance, hszTopic);
    return m_hConversation != NULL;
  } else {
    return FALSE;
  }
}

void CDDE::Disconnect() {
  if (m_hConversation) {
    ::DdeDisconnect(m_hConversation);
    m_hConversation = NULL;
  }
}

BOOL CDDE::Initialize(DWORD afCmd, BOOL unicode) {
  m_bUnicode = unicode;
  if (m_bUnicode) {
    ::DdeInitializeW(&m_dwInstance, DDECallback, afCmd, 0);
  } else {
    ::DdeInitializeA(&m_dwInstance, DDECallback, afCmd, 0);
  }
  return m_dwInstance != 0;
}

BOOL CDDE::IsAvailable() {
  return m_dwInstance != 0;
}

BOOL CDDE::NameService(const wstring& service, UINT afCmd) {
  HSZ hszService = NULL;
  if (m_bUnicode) {
    hszService = ::DdeCreateStringHandleW(m_dwInstance, service.c_str(), CP_WINUNICODE);
  } else {
    hszService = ::DdeCreateStringHandleA(m_dwInstance, ToANSI(service), CP_WINANSI);
  }
  HDDEDATA result = ::DdeNameService(m_dwInstance, hszService, 0, afCmd);
  ::DdeFreeStringHandle(m_dwInstance, hszService);
  return result != 0;
}

void CDDE::UnInitialize() {
  if (m_dwInstance) {
    ::DdeUninitialize(m_dwInstance);
    m_dwInstance = 0;
  }
}

// =============================================================================

HDDEDATA CALLBACK CDDE::DDECallback(UINT uType, UINT uFmt, HCONV hconv, HSZ hsz1, HSZ hsz2, HDDEDATA hdata, DWORD dwData1, DWORD dwData2) {
  DWORD cb = 0;
  LPVOID lpData = NULL;
  char sz1[256] = {'\0'}, sz2[256] = {'\0'};
  //DdeQueryStringA(m_dwInstance, hsz1, sz1, 256, CP_WINANSI);
  //DdeQueryStringA(m_dwInstance, hsz2, sz2, 256, CP_WINANSI);

  switch (uType) { 
    case XTYP_CONNECT: {
      OutputDebugStringA("[CONNECT]\n");
      //BOOL result = OnConnect();
      return reinterpret_cast<HDDEDATA>(TRUE);
    }

    case XTYP_POKE: {
      if (hdata) lpData = DdeAccessData(hdata, &cb);
     #ifdef _DEBUG
      string str = "[POKE]";
      str += "Topic: " + *sz1;
      str += " - Item: " + *sz2;
      if (lpData) str += " - Data: "; str += (LPCSTR)lpData;
      str += "\n";
      OutputDebugStringA(str.c_str());
      #endif
      //OnPoke();
      if (hdata) DdeUnaccessData(hdata);
      return reinterpret_cast<HDDEDATA>(DDE_FACK);
    }

    case XTYP_REQUEST: {
      // TODO: Call DdeCreateDataHandle();
      #ifdef _DEBUG
      string str = "[REQUEST] ";
      str += "Topic: "; str += sz1;
      str += " - Item: "; str += sz2;
      str += "\n";
      OutputDebugStringA(str.c_str());
      #endif
      //OnRequest();
      break;
    }

    default:
      break;
  }

  return reinterpret_cast<HDDEDATA>(0); 
}