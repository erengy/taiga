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

#include "std.h"

#include "dde.h"

#include "string.h"

// =============================================================================

DynamicDataExchange::DynamicDataExchange()
    : is_unicode_(FALSE), 
      instance_(0), 
      conversation_(NULL) {
}

DynamicDataExchange::~DynamicDataExchange() {
  Disconnect();
  UnInitialize();
}

BOOL DynamicDataExchange::ClientTransaction(const wstring& item, const wstring& data, wstring* output, UINT wType) {
  HDDEDATA hData = NULL;
  HSZ hszItem = NULL;
  DWORD dwResult = 0;
  if (is_unicode_) {
    if (wType != XTYP_EXECUTE) {
      hszItem = ::DdeCreateStringHandleW(instance_, item.c_str(), CP_WINUNICODE);
    }
    hData = ::DdeClientTransaction((LPBYTE)data.data(), (data.size() + 1) * sizeof(WCHAR), conversation_, 
      hszItem, CF_UNICODETEXT, wType, 3000, &dwResult);
  } else {
    string item_ansi = ToANSI(item), data_ansi = ToANSI(data);
    if (wType != XTYP_EXECUTE) {
      hszItem = ::DdeCreateStringHandleA(instance_, item_ansi.c_str(), CP_WINANSI);
    }
    hData = ::DdeClientTransaction((LPBYTE)data_ansi.data(), data_ansi.size() + 1, conversation_, 
      hszItem, CF_TEXT, wType, 3000, &dwResult);
  }
  ::DdeFreeStringHandle(instance_, hszItem);
  if (output) {
    char szResult[255];
    ::DdeGetData(hData, (unsigned char*)szResult, 255, 0);
    output->assign(ToUTF8(szResult));
  }
  return hData != 0;
}

BOOL DynamicDataExchange::Connect(const wstring& service, const wstring& topic) {
  if (instance_) {
    HSZ hszService = NULL, hszTopic = NULL;
    if (is_unicode_) {
      hszService = ::DdeCreateStringHandleW(instance_, service.c_str(), CP_WINUNICODE);
      hszTopic = ::DdeCreateStringHandleW(instance_, topic.c_str(), CP_WINUNICODE);
    } else {
      hszService = ::DdeCreateStringHandleA(instance_, ToANSI(service), CP_WINANSI);
      hszTopic = ::DdeCreateStringHandleA(instance_, ToANSI(topic), CP_WINANSI);
    }
    conversation_ = ::DdeConnect(instance_, hszService, hszTopic, NULL);
    ::DdeFreeStringHandle(instance_, hszService);
    ::DdeFreeStringHandle(instance_, hszTopic);
    return conversation_ != NULL;
  } else {
    return FALSE;
  }
}

void DynamicDataExchange::Disconnect() {
  if (conversation_) {
    ::DdeDisconnect(conversation_);
    conversation_ = NULL;
  }
}

BOOL DynamicDataExchange::Initialize(DWORD afCmd, BOOL unicode) {
  is_unicode_ = unicode;
  if (is_unicode_) {
    ::DdeInitializeW(&instance_, DdeCallback, afCmd, 0);
  } else {
    ::DdeInitializeA(&instance_, DdeCallback, afCmd, 0);
  }
  return instance_ != 0;
}

BOOL DynamicDataExchange::IsAvailable() {
  return instance_ != 0;
}

BOOL DynamicDataExchange::NameService(const wstring& service, UINT afCmd) {
  HSZ hszService = NULL;
  if (is_unicode_) {
    hszService = ::DdeCreateStringHandleW(instance_, service.c_str(), CP_WINUNICODE);
  } else {
    hszService = ::DdeCreateStringHandleA(instance_, ToANSI(service), CP_WINANSI);
  }
  HDDEDATA result = ::DdeNameService(instance_, hszService, 0, afCmd);
  ::DdeFreeStringHandle(instance_, hszService);
  return result != 0;
}

void DynamicDataExchange::UnInitialize() {
  if (instance_) {
    ::DdeUninitialize(instance_);
    instance_ = 0;
  }
}

// =============================================================================

HDDEDATA CALLBACK DynamicDataExchange::DdeCallback(UINT uType, UINT uFmt, HCONV hconv, HSZ hsz1, HSZ hsz2, HDDEDATA hdata, DWORD dwData1, DWORD dwData2) {
  DWORD cb = 0;
  LPVOID lpData = NULL;
  char sz1[256] = {'\0'}, sz2[256] = {'\0'};
  //DdeQueryStringA(instance_, hsz1, sz1, 256, CP_WINANSI);
  //DdeQueryStringA(instance_, hsz2, sz2, 256, CP_WINANSI);

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