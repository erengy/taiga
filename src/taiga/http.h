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

#ifndef TAIGA_TAIGA_HTTP_H
#define TAIGA_TAIGA_HTTP_H

#include "base/std.h"
#include "base/types.h"
#include "win/http/win_http.h"

enum HttpClientMode {
  HTTP_Silent = 0,
  // MyAnimeList
  HTTP_MAL_Login,
  HTTP_MAL_RefreshList,
  HTTP_MAL_AnimeAdd,
  HTTP_MAL_AnimeAskToDiscuss,
  HTTP_MAL_AnimeDelete,
  HTTP_MAL_AnimeDetails,
  HTTP_MAL_AnimeUpdate,
  HTTP_MAL_SearchAnime,
  HTTP_MAL_Image,
  HTTP_MAL_UserImage,
  // Feed
  HTTP_Feed_Check,
  HTTP_Feed_CheckAuto,
  HTTP_Feed_Download,
  HTTP_Feed_DownloadAll,
  HTTP_Feed_DownloadIcon,
  // Twitter
  HTTP_Twitter_Request,
  HTTP_Twitter_Auth,
  HTTP_Twitter_Post,
  // Taiga
  HTTP_UpdateCheck,
  HTTP_UpdateDownload
};

enum HttpClientType {
  HTTP_Client_Image = 1,
  HTTP_Client_Search
};

// =============================================================================

class HttpClient : public win::http::Client {
public:
  HttpClient();
  virtual ~HttpClient() {}

  DWORD GetClientMode() const;
  void SetClientMode(DWORD mode);
  LPARAM GetParam() const;

protected:
  void OnError(DWORD error);
  bool OnSendRequestComplete();
  bool OnHeadersAvailable();
  bool OnDataAvailable();
  bool OnReadData();
  bool OnReadComplete();
  bool OnRedirect(const std::wstring& address);

private:
  DWORD mode_;
};

class AnimeClients {
public:
  AnimeClients();
  virtual ~AnimeClients();

  void Cleanup(bool force);
  HttpClient* GetClient(int type, int anime_id);
  void UpdateProxy(const wstring& proxy, const wstring& user, const wstring& pass);

private:
  std::map<int, std::map<int, HttpClient*>> clients_;
};

struct HttpClients {
  AnimeClients anime;

  HttpClient application;

  struct Service {
    HttpClient image;
    HttpClient list;
    HttpClient search;
  } service;

  struct Sharing {
    HttpClient http;
    HttpClient twitter;
  } sharing;
};

extern HttpClients Clients;

// =============================================================================

void SetProxies(const wstring& proxy, const wstring& user, const wstring& pass);

#endif  // TAIGA_TAIGA_HTTP_H