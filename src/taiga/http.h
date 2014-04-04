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

#ifndef TAIGA_TAIGA_HTTP_H
#define TAIGA_TAIGA_HTTP_H

#include <map>

#include "base/http.h"
#include "base/types.h"
#include "win/win_thread.h"

namespace taiga {

enum HttpClientMode {
  kHttpSilent,
  // Service
  kHttpServiceAuthenticateUser,
  kHttpServiceGetMetadataById,
  kHttpServiceGetMetadataByIdV2,
  kHttpServiceSearchTitle,
  kHttpServiceAddLibraryEntry,
  kHttpServiceDeleteLibraryEntry,
  kHttpServiceGetLibraryEntries,
  kHttpServiceUpdateLibraryEntry,
  // Library
  kHttpGetLibraryEntryImage,
  // Feed
  kHttpFeedCheck,
  kHttpFeedCheckAuto,
  kHttpFeedDownload,
  kHttpFeedDownloadAll,
  // Twitter
  kHttpTwitterRequest,
  kHttpTwitterAuth,
  kHttpTwitterPost,
  // Taiga
  kHttpTaigaUpdateCheck,
  kHttpTaigaUpdateDownload
};

class HttpClient : public base::http::Client {
public:
  friend class HttpManager;

  HttpClient();
  virtual ~HttpClient() {}

  HttpClientMode mode() const;
  void set_mode(HttpClientMode mode);

protected:
  void OnError(CURLcode error_code);
  bool OnHeadersAvailable();
  bool OnProgress();
  bool OnReadComplete();
  bool OnRedirect(const std::wstring& address);

private:
  HttpClientMode mode_;
};

class HttpManager {
public:
  HttpClient& GetNewClient(const base::uid_t& uid);

  void CancelRequest(base::uid_t uid);
  void MakeRequest(HttpRequest& request, HttpClientMode mode);
  void MakeRequest(HttpClient& client, HttpRequest& request, HttpClientMode mode);

  void HandleError(HttpResponse& response, const string_t& error);
  void HandleRedirect(const std::wstring& current_host, const std::wstring& next_host);
  void HandleResponse(HttpResponse& response);

  void FreeMemory();
  void Shutdown();

private:
  void AddToQueue(HttpRequest& request);
  void ProcessQueue();
  void AddConnection(const string_t& hostname);
  void FreeConnection(const string_t& hostname);

  std::map<std::wstring, HttpClient> clients_;
  std::map<std::wstring, unsigned int> connections_;
  win::CriticalSection critical_section_;
  std::vector<HttpRequest> requests_;
};

}  // namespace taiga

extern taiga::HttpManager ConnectionManager;

#endif  // TAIGA_TAIGA_HTTP_H