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

#ifndef HTTP_H
#define HTTP_H

#include "std.h"
#include "win32/win_http.h"

enum HTTP_ClientMode {
  HTTP_Silent = 0,
  // MyAnimeList
  HTTP_MAL_Login,
  HTTP_MAL_RefreshList,
  HTTP_MAL_RefreshAndLogin,
  HTTP_MAL_AnimeAdd,
  HTTP_MAL_AnimeDelete,
  HTTP_MAL_AnimeDetails,
  HTTP_MAL_AnimeEdit,
  HTTP_MAL_AnimeUpdate,
  HTTP_MAL_ScoreUpdate,
  HTTP_MAL_StatusUpdate,
  HTTP_MAL_TagUpdate,
  HTTP_MAL_Friends,
  HTTP_MAL_Profile,
  HTTP_MAL_SearchAnime,
  HTTP_MAL_Image,
  // Feed
  HTTP_Feed_Check,
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

// =============================================================================

class CHTTPClient : public CHTTP {
public:
  CHTTPClient();
  virtual ~CHTTPClient() {}

protected:
  BOOL OnError(DWORD dwError);
  BOOL OnSendRequestComplete();
  BOOL OnHeadersAvailable(wstring headers);
  BOOL OnReadData();
  BOOL OnReadComplete();
  BOOL OnRedirect(wstring address);
};

extern CHTTPClient HTTPClient, ImageClient, MainClient, SearchClient, TwitterClient, VersionClient;

// =============================================================================

void SetProxies(const wstring& proxy, const wstring& user, const wstring& pass);

#endif // HTTP_H