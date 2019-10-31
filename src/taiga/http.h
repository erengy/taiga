/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#pragma once

#include <list>
#include <map>

#include <windows/win/thread.h>

#include "base/http.h"
#include "base/types.h"

namespace taiga {

enum HttpClientMode {
  kHttpSilent,
  // Service
  kHttpServiceAuthenticateUser,
  kHttpServiceGetUser,
  kHttpServiceGetMetadataById,
  kHttpServiceGetSeason,
  kHttpServiceSearchTitle,
  kHttpServiceAddLibraryEntry,
  kHttpServiceDeleteLibraryEntry,
  kHttpServiceGetLibraryEntries,
  kHttpServiceUpdateLibraryEntry,
  kHttpMalRequestAccessToken,
};

class HttpClient : public base::http::Client {
public:
  friend class HttpManager;

  HttpClient(const HttpRequest& request);
  virtual ~HttpClient() {}

  HttpClientMode mode() const;
  void set_mode(HttpClientMode mode);

protected:
  void OnError(CURLcode error_code);
  bool OnHeadersAvailable();
  bool OnProgress();
  void OnReadComplete();
  bool OnRedirect(const std::wstring& address, bool refresh);

private:
  HttpClientMode mode_;
};

}  // namespace taiga
