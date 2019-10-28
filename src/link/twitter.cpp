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

#include "link/twitter.h"

#include "base/file.h"
#include "base/oauth.h"
#include "base/string.h"
#include "taiga/http.h"
#include "taiga/settings.h"
#include "ui/ui.h"

namespace link::twitter {

static oauth::Consumer GetOauthConsumer() {
  return {
    L"9GZsCbqzjOrsPWlIlysvg",
    L"ebjXyymbuLtjDvoxle9Ldj8YYIMoleORapIOoqBrjRw"
  };
}

void RequestToken() {
  HttpRequest http_request;
  http_request.url.protocol = base::http::Protocol::Https;
  http_request.url.host = L"api.twitter.com";
  http_request.url.path = L"/oauth/request_token";
  http_request.header[L"Authorization"] = oauth::BuildAuthorizationHeader(
      http_request.url.Build(), L"GET", GetOauthConsumer());

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTwitterRequest);
}

static void AccessToken(const std::wstring& key, const std::wstring& secret,
                        const std::wstring& pin) {
  HttpRequest http_request;
  http_request.url.protocol = base::http::Protocol::Https;
  http_request.url.host = L"api.twitter.com";
  http_request.url.path = L"/oauth/access_token";
  http_request.header[L"Authorization"] = oauth::BuildAuthorizationHeader(
      http_request.url.Build(), L"POST", GetOauthConsumer(),
      oauth::Access{key, secret, pin});

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTwitterAuth);
}

bool SetStatusText(const std::wstring& status_text) {
  if (taiga::settings.GetShareTwitterOauthToken().empty() ||
      taiga::settings.GetShareTwitterOauthSecret().empty()) {
    return false;
  }

  static std::wstring previous_status_text;
  if (status_text.empty() || status_text == previous_status_text)
    return false;
  previous_status_text = status_text;

  oauth::Parameters post_parameters;
  post_parameters[L"status"] = EncodeUrl(status_text);

  HttpRequest http_request;
  http_request.method = L"POST";
  http_request.url.protocol = base::http::Protocol::Https;
  http_request.url.host = L"api.twitter.com";
  http_request.url.path = L"/1.1/statuses/update.json";
  http_request.body = L"status=" + post_parameters[L"status"];
  http_request.header[L"Authorization"] = oauth::BuildAuthorizationHeader(
      http_request.url.Build(), L"POST", GetOauthConsumer(),
      oauth::Access{taiga::settings.GetShareTwitterOauthToken(),
                    taiga::settings.GetShareTwitterOauthSecret(), {}},
      post_parameters);

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTwitterPost);
  return true;
}

void HandleHttpResponse(const taiga::HttpClientMode mode,
                        const HttpResponse& response) {
  switch (mode) {
    case taiga::kHttpTwitterRequest: {
      bool success = false;
      auto parameters = oauth::ParseQueryString(response.body);
      if (!parameters[L"oauth_token"].empty()) {
        ExecuteLink(L"https://api.twitter.com/oauth/authorize?oauth_token=" +
                    parameters[L"oauth_token"]);
        std::wstring auth_pin;
        if (ui::OnTwitterTokenEntry(auth_pin))
          AccessToken(parameters[L"oauth_token"],
                      parameters[L"oauth_token_secret"],
                      auth_pin);
        success = true;
      }
      ui::OnTwitterTokenRequest(success);
      break;
    }

    case taiga::kHttpTwitterAuth: {
      bool success = false;
      auto parameters = oauth::ParseQueryString(response.body);
      if (!parameters[L"oauth_token"].empty() &&
          !parameters[L"oauth_token_secret"].empty()) {
        taiga::settings.SetShareTwitterOauthToken(parameters[L"oauth_token"]);
        taiga::settings.SetShareTwitterOauthSecret(parameters[L"oauth_token_secret"]);
        taiga::settings.SetShareTwitterUsername(parameters[L"screen_name"]);
        success = true;
      }
      ui::OnTwitterAuth(success);
      break;
    }

    case taiga::kHttpTwitterPost: {
      if (InStr(response.body, L"\"errors\"", 0) == -1) {
        ui::OnTwitterPost(true, L"");
      } else {
        std::wstring error;
        int index_begin = InStr(response.body, L"\"message\":\"", 0);
        int index_end = InStr(response.body, L"\",\"", index_begin);
        if (index_begin > -1 && index_end > -1) {
          index_begin += 11;
          error = response.body.substr(index_begin, index_end - index_begin);
        }
        ui::OnTwitterPost(false, error);
      }
      break;
    }
  }
}

}  // namespace link::twitter
