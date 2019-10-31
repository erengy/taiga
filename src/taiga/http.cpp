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

#include "base/file.h"
#include "base/format.h"
#include "base/json.h"
#include "base/log.h"
#include "base/string.h"
#include "base/url.h"
#include "link/twitter.h"
#include "media/anime_db.h"
#include "media/anime_season_db.h"
#include "media/anime_util.h"
#include "sync/manager.h"
#include "taiga/announce.h"
#include "taiga/config.h"
#include "taiga/http.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/stats.h"
#include "taiga/taiga.h"
#include "taiga/update.h"
#include "taiga/version.h"
#include "track/feed.h"
#include "track/feed_aggregator.h"
#include "track/recognition.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace taiga {

// These are the values commonly used by today's web browsers.
// See: http://www.browserscope.org/?category=network
const unsigned int kMaxSimultaneousConnections = 10;
const unsigned int kMaxSimultaneousConnectionsPerHostname = 6;

HttpClient::HttpClient(const HttpRequest& request)
    : base::http::Client(request),
      mode_(kHttpSilent) {
  // Reuse existing connections
  set_allow_reuse(settings.GetAppConnectionReuseActive());

  // Enable debug mode to log requests and responses
  set_debug_mode(Taiga.options.debug_mode);

  // Disabling certificate revocation checks seems to work for those who get
  // "SSL connect error". See issue #312 for more information.
  set_no_revoke(settings.GetAppConnectionNoRevoke());

  // The default header (e.g. "User-Agent: Taiga/1.0") will be used, unless
  // another value is specified in the request header
  set_user_agent(L"{}/{}.{}"_format(
      TAIGA_APP_NAME, TAIGA_VERSION_MAJOR, TAIGA_VERSION_MINOR));

  // Make sure all new clients use the proxy settings
  set_proxy(settings.GetAppConnectionProxyHost(),
            settings.GetAppConnectionProxyUsername(),
            settings.GetAppConnectionProxyPassword());
}

HttpClientMode HttpClient::mode() const {
  return mode_;
}

void HttpClient::set_mode(HttpClientMode mode) {
  mode_ = mode;
}

////////////////////////////////////////////////////////////////////////////////

void HttpClient::OnError(CURLcode error_code) {
  std::wstring error_text = L"HTTP error #" + ToWstr(error_code) + L": " +
                            StrToWstr(curl_easy_strerror(error_code));
  TrimRight(error_text, L"\r\n ");
  switch (error_code) {
    case CURLE_COULDNT_RESOLVE_HOST:
    case CURLE_COULDNT_CONNECT:
      error_text += L" (" + request_.url.host + L")";
      break;
  }

  LOGE(L"{}\nConnection mode: {}", error_text, mode_);

  ui::OnHttpError(*this, error_text);

  taiga::stats.connections_failed++;
}

bool HttpClient::OnHeadersAvailable() {
  ui::OnHttpHeadersAvailable(*this);
  return false;
}

bool HttpClient::OnRedirect(const std::wstring& address, bool refresh) {
  LOGD(L"Location: {}", address);

  auto check_cloudflare = [](const base::http::Response& response) {
    if (response.code >= 500) {
      auto it = response.header.find(L"Server");
      if (it != response.header.end() &&
          InStr(it->second, L"cloudflare", 0, true) > -1) {
        return true;
      }
    }
    return false;
  };

  if (refresh) {
    if (check_cloudflare(response_)) {
      std::wstring error_text = L"Cannot connect to " + request_.url.host +
                                L" because of Cloudflare DDoS protection";
      LOGE(L"{}\nConnection mode: {}", error_text, mode_);
      ui::OnHttpError(*this, error_text);
    } else {
      HttpRequest http_request = request_;
      http_request.uid = base::http::GenerateRequestId();
      http_request.url = address;
      //ConnectionManager.MakeRequest(http_request, mode());
    }
    Cancel();
    return true;

  } else {
    Url url(address);
    //ConnectionManager.HandleRedirect(request_.url.host, url.host);
    return false;
  }
}

bool HttpClient::OnProgress() {
  ui::OnHttpProgress(*this);
  return false;
}

void HttpClient::OnReadComplete() {
  ui::OnHttpReadComplete();

  taiga::stats.connections_succeeded++;
}

}  // namespace taiga
