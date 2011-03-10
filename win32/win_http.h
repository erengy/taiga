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

#ifndef WIN_HTTP_H
#define WIN_HTTP_H

#include "win_main.h"
#include <winhttp.h>

enum HTTP_ContentEncoding {
  HTTP_Encoding_None = 0,
  HTTP_Encoding_Gzip
};

// =============================================================================

class CCrackURL {
public:
  CCrackURL() {}
  CCrackURL(wstring url) { Crack(url); }
  virtual ~CCrackURL() {}

  void Crack(wstring url);
  wstring Scheme, Host, Path;
};

// =============================================================================

class CHTTP {
public:
  CHTTP();
  ~CHTTP();
  
  void    Cleanup();
  void    ClearCookies();
  wstring GetCookie();
  DWORD   GetClientMode();
  wstring GetData();
  wstring GetDefaultHeader();
  LPARAM  GetParam();
  void    SetAutoRedirect(BOOL enabled);
  void    SetProxy(const wstring& proxy, const wstring& user, const wstring& pass);
  void    SetUserAgent(const wstring& user_agent);
  
  bool Connect(wstring szServer, wstring szObject, wstring szData, wstring szVerb, 
    wstring szHeader, wstring szReferer, wstring szFile, 
    DWORD dwClientMode = 0, LPARAM lParam = 0);
  bool Get(wstring szServer, wstring szObject, wstring szFile, 
    DWORD dwClientMode = 0, LPARAM lParam = 0);
  bool Post(wstring szServer, wstring szObject, wstring szData, wstring szFile, 
    DWORD dwClientMode = 0, LPARAM lParam = 0);

  virtual BOOL OnError(DWORD dwError) { return FALSE; }
  virtual BOOL OnSendRequestComplete() { return FALSE; }
  virtual BOOL OnHeadersAvailable(wstring headers) { return FALSE; }
  virtual BOOL OnDataAvailable() { return FALSE; }
  virtual BOOL OnReadData() { return FALSE; }
  virtual BOOL OnReadComplete() { return TRUE; }
  virtual BOOL OnRedirect(wstring address) { return FALSE; }

private:
  static void CALLBACK Callback(HINTERNET hInternet, DWORD_PTR dwContext, 
    DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);
  void StatusCallback(HINTERNET hInternet, 
    DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);

protected:
  void ParseHeaders(wstring headers);

  BOOL    m_AutoRedirect;
  LPSTR   m_Buffer;
  INT     m_ContentEncoding;
  wstring m_Cookie;
  wstring m_File;
  string  m_OptionalData;
  wstring m_Proxy, m_ProxyUser, m_ProxyPass;
  wstring m_UserAgent;

  DWORD  m_dwDownloaded, m_dwTotal;
  DWORD  m_dwClientMode;
  LPARAM m_lParam;

  HINTERNET m_hConnect, m_hRequest, m_hSession;
};

#endif // WIN_HTTP_H