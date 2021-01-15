/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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
#include "base/format.h"
#include "base/json.h"
#include "base/oauth.h"
#include "base/string.h"
#include "base/url.h"
#include "taiga/http.h"
#include "taiga/settings.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/ui.h"

namespace link::twitter {

static void AccessToken(const std::wstring& key, const std::wstring& secret,
                        const std::wstring& pin);

static oauth::Consumer GetOauthConsumer() {
  return {
    L"9GZsCbqzjOrsPWlIlysvg",
    L"ebjXyymbuLtjDvoxle9Ldj8YYIMoleORapIOoqBrjRw"
  };
}

void RequestToken() {
  constexpr auto kTarget = "https://api.twitter.com/oauth/request_token";

  const auto authorization = WstrToStr(oauth::BuildAuthorizationHeader(
      StrToWstr(kTarget), L"GET", GetOauthConsumer()));

  taiga::http::Request request;
  request.set_target(kTarget);
  request.set_header("Authorization", authorization);

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    ui::ChangeStatusText(L"Obtaining request token from Twitter... ({})"_format(
        taiga::http::util::to_string(transfer)));
    return true;
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (response.error()) {
      ui::ChangeStatusText(L"Could not get request token from Twitter. ({})"_format(
          taiga::http::util::to_string(response.error(), {})));
      return;
    }

    auto parameters = oauth::ParseQueryString(StrToWstr(response.body()));
    if (parameters[L"oauth_token"].empty()) {
      ui::ChangeStatusText(L"Could not get request token from Twitter.");
      return;
    }

    ui::ClearStatusText();
    ExecuteLink(
        L"https://api.twitter.com/oauth/authorize?oauth_token={}"_format(
            parameters[L"oauth_token"]));
    if (std::wstring auth_pin; ui::OnTwitterTokenEntry(auth_pin)) {
      AccessToken(parameters[L"oauth_token"],
                  parameters[L"oauth_token_secret"],
                  auth_pin);
    }
  };

  taiga::http::Send(request, on_transfer, on_response);
}

static void AccessToken(const std::wstring& key, const std::wstring& secret,
                        const std::wstring& pin) {
  constexpr auto kTarget = "https://api.twitter.com/oauth/access_token";

  const auto authorization = WstrToStr(oauth::BuildAuthorizationHeader(
      StrToWstr(kTarget), L"POST", GetOauthConsumer(),
      oauth::Access{key, secret, pin}));

  taiga::http::Request request;
  request.set_target(kTarget);
  request.set_header("Authorization", authorization);

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    ui::ChangeStatusText(L"Obtaining access token from Twitter... ({})"_format(
        taiga::http::util::to_string(transfer)));
    return true;
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (response.error()) {
      ui::ChangeStatusText(L"Could not get access token from Twitter. ({})"_format(
          taiga::http::util::to_string(response.error(), {})));
      return;
    }

    auto parameters = oauth::ParseQueryString(StrToWstr(response.body()));
    if (parameters[L"oauth_token"].empty() ||
        parameters[L"oauth_token_secret"].empty()) {
      ui::ChangeStatusText(L"Could not get access token from Twitter.");
      ui::DlgSettings.RefreshTwitterLink();
      return;
    }

    taiga::settings.SetShareTwitterOauthToken(parameters[L"oauth_token"]);
    taiga::settings.SetShareTwitterOauthSecret(
        parameters[L"oauth_token_secret"]);
    taiga::settings.SetShareTwitterUsername(parameters[L"screen_name"]);

    ui::ChangeStatusText(
        L"Taiga is now authorized to post to this Twitter account: " +
        taiga::settings.GetShareTwitterUsername());
    ui::DlgSettings.RefreshTwitterLink();
  };

  taiga::http::Send(request, on_transfer, on_response);
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
  post_parameters[L"status"] = Url::Encode(status_text);

  constexpr auto kTarget = "https://api.twitter.com/1.1/statuses/update.json";

  const auto authorization = WstrToStr(oauth::BuildAuthorizationHeader(
      StrToWstr(kTarget), L"POST", GetOauthConsumer(),
      oauth::Access{taiga::settings.GetShareTwitterOauthToken(),
                    taiga::settings.GetShareTwitterOauthSecret(),
                    {}},
      post_parameters));

  taiga::http::Request request;
  request.set_method("POST");
  request.set_target(kTarget);
  request.set_header("Authorization", authorization);
  request.set_body({{"status", WstrToStr(status_text)}});

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    ui::ChangeStatusText(L"Updating Twitter status... ({})"_format(
        taiga::http::util::to_string(transfer)));
    return true;
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (response.error()) {
      ui::ChangeStatusText(L"Could not update Twitter status. ({})"_format(
          taiga::http::util::to_string(response.error(), {})));
      return;
    }

    Json json;
    JsonParseString(response.body(), json);
    if (json.contains("errors")) {
      for (const auto error : json["errors"]) {
        const auto message = StrToWstr(JsonReadStr(error, "message"));
        if (!message.empty()) {
          ui::ChangeStatusText(
              L"Could not update Twitter status. ({})"_format(message));
          return;
        }
      }
    }

    ui::ChangeStatusText(L"Twitter status updated.");
  };

  taiga::http::Send(request, on_transfer, on_response);
  return true;
}

}  // namespace link::twitter
