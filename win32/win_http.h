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

#ifndef WIN_HTTP_H
#define WIN_HTTP_H

#include "win_main.h"
#include <winhttp.h>
#include <map>

namespace win32 {

enum HttpContentEncoding {
  HTTP_Encoding_None = 0,
  HTTP_Encoding_Gzip
};

typedef std::multimap<wstring, wstring> http_header_t;

// =============================================================================

class Url {
public:
  Url() {}
  Url(wstring url) { Crack(url); }
  virtual ~Url() {}

  Url& operator=(const Url& url);
  void operator=(const wstring& url);

public:
  void Crack(wstring url);

public:
  wstring Scheme, Host, Path;
};

// =============================================================================

class Http {
public:
  Http();
  ~Http();
  
  void    Cleanup();
  void    ClearCookies();
  wstring GetCookie();
  DWORD   GetClientMode();
  wstring GetData();
  wstring GetDefaultHeader();
  LPARAM  GetParam();
  int     GetResponseStatusCode();
  void    SetAutoRedirect(BOOL enabled);
  void    SetProxy(const wstring& proxy, const wstring& user, const wstring& pass);
  void    SetUserAgent(const wstring& user_agent);

  bool Connect(wstring szServer, wstring szObject, wstring szData, wstring szVerb, 
    wstring szHeader, wstring szReferer, wstring szFile, 
    DWORD dwClientMode = 0, LPARAM lParam = 0);
  bool Connect(const Url& url, wstring szData, wstring szVerb, 
    wstring szHeader, wstring szReferer, wstring szFile, 
    DWORD dwClientMode = 0, LPARAM lParam = 0);
  bool Get(wstring szServer, wstring szObject, wstring szFile, 
    DWORD dwClientMode = 0, LPARAM lParam = 0);
  bool Get(const Url& url, wstring szFile, 
    DWORD dwClientMode = 0, LPARAM lParam = 0);
  bool Post(wstring szServer, wstring szObject, wstring szData, wstring szFile, 
    DWORD dwClientMode = 0, LPARAM lParam = 0);
  bool Post(const Url& url, wstring szData, wstring szFile, 
    DWORD dwClientMode = 0, LPARAM lParam = 0);

  virtual void OnInitialize() {}
  virtual BOOL OnError(DWORD dwError) { return FALSE; }
  virtual BOOL OnSendRequestComplete() { return FALSE; }
  virtual BOOL OnHeadersAvailable(http_header_t& headers) { return FALSE; }
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
  wstring BuildRequestHeader(wstring header);
  bool ParseHeader(const wstring& text, http_header_t& header);
  bool ParseResponseHeader(const wstring& header);

  BOOL    m_AutoRedirect;
  LPSTR   m_Buffer;
  INT     m_ContentEncoding;
  wstring m_Cookie;
  wstring m_File;
  string  m_OptionalData;
  wstring m_Proxy, m_ProxyUser, m_ProxyPass;
  wstring m_Referer;
  wstring m_UserAgent;
  wstring m_Verb;

  wstring m_RequestHeader;
  int m_ResponseStatusCode;
  http_header_t m_ResponseHeader;

  DWORD  m_dwDownloaded, m_dwTotal;
  DWORD  m_dwClientMode;
  LPARAM m_lParam;

  HINTERNET m_hConnect, m_hRequest, m_hSession;
};

} // namespace win32

#endif // WIN_HTTP_H