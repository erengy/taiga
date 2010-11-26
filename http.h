/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

#ifndef HTTP_H
#define HTTP_H

#include "std.h"

enum HTTP_ClientMode {
  HTTP_Silent = 0,
  HTTP_MAL_Login,
  HTTP_MAL_RefreshList,
  HTTP_MAL_RefreshAndLogin,
  HTTP_MAL_AnimeAdd,
  HTTP_MAL_AnimeDelete,
  HTTP_MAL_AnimeEdit,
  HTTP_MAL_AnimeUpdate,
  HTTP_MAL_ScoreUpdate,
  HTTP_MAL_StatusUpdate,
  HTTP_MAL_TagUpdate,
  HTTP_MAL_Friends,
  HTTP_MAL_Profile,
  HTTP_MAL_SearchAnime,
  HTTP_MAL_Image,
  HTTP_TorrentCheck,
  HTTP_TorrentDownload,
  HTTP_TorrentDownloadAll,
  HTTP_Twitter,
  HTTP_VersionCheck
};

// =============================================================================

class CHTTP {
public:
  CHTTP();
  ~CHTTP();
  
  void    Cleanup();
  void    ClearCookies();
  wstring GetCookie();
  wstring GetData();
  DWORD   GetClientMode();
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
// =============================================================================

class CHTTPClient : public CHTTP {
public:
  CHTTPClient() {}
  virtual ~CHTTPClient() {}

protected:
  BOOL OnError(DWORD dwError);
  BOOL OnSendRequestComplete();
  BOOL OnHeadersAvailable(wstring headers);
  BOOL OnReadData();
  BOOL OnReadComplete();
  BOOL OnRedirect(wstring address);
};

extern CHTTPClient MainClient, ImageClient, SearchClient, TorrentClient, VersionClient;
extern CHTTP HTTPClient, TwitterClient;

#endif // HTTP_H