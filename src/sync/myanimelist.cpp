/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <functional>
#include <optional>

#include "sync/myanimelist.h"

#include "base/format.h"
#include "base/html.h"
#include "base/json.h"
#include "base/log.h"
#include "base/string.h"
#include "base/url.h"
#include "media/anime_db.h"
#include "media/anime_item.h"
#include "media/anime_season.h"
#include "media/anime_season_db.h"
#include "media/anime_util.h"
#include "media/library/queue.h"
#include "sync/myanimelist_util.h"
#include "sync/sync.h"
#include "taiga/http.h"
#include "taiga/settings.h"
#include "track/recognition.h"
#include "ui/resource.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace sync::myanimelist {

// API documentation:
// https://myanimelist.net/apiconfig/references/api/v2

struct Error {
  enum class Type {
    // 400 Bad Request
    // {"error":"bad_request","message":"invalid q"}
    BadRequest,

    // 401 Unauthorized
    // WWW-Authenticate: Bearer error="invalid_token",error_description="The access token expired"
    // {"error":"invalid_token"}
    AccessTokenExpired,

    // 401 Unauthorized
    // {"error":"invalid_request","message":"The refresh token is invalid.","hint":"Token has expired"}
    RefreshTokenExpired,

    // 403 Forbidden
    // {"message":"","error":"forbidden"}
    Forbidden,

    Other,
  };

  Type type = Type::Other;
  std::wstring description;
};

constexpr auto kBaseUrl = "https://api.myanimelist.net/v2";
constexpr auto kLibraryPageLimit = 1000;
constexpr auto kSearchPageLimit = 100;
constexpr auto kSeasonPageLimit = 500;

////////////////////////////////////////////////////////////////////////////////

class Account {
public:
  static std::string username() {
    return WstrToStr(taiga::settings.GetSyncServiceMalUsername());
  }
  static void set_username(const std::string& username) {
    return taiga::settings.SetSyncServiceMalUsername(StrToWstr(username));
  }

  static std::string access_token() {
    return WstrToStr(taiga::settings.GetSyncServiceMalAccessToken());
  }
  static std::string refresh_token() {
    return WstrToStr(taiga::settings.GetSyncServiceMalRefreshToken());
  }
  static void set_access_token(const std::string& token) {
    taiga::settings.SetSyncServiceMalAccessToken(StrToWstr(token));
  }
  static void set_refresh_token(const std::string& token) {
    taiga::settings.SetSyncServiceMalRefreshToken(StrToWstr(token));
  }

  bool authenticated() const {
    return authenticated_;
  }
  void set_authenticated(const bool authenticated) {
    authenticated_ = authenticated;
  }

private:
  bool authenticated_ = false;
};

static Account account;

bool IsUserAuthenticated() {
  return account.authenticated();
}

void InvalidateUserAuthentication() {
  account.set_authenticated(false);
}

////////////////////////////////////////////////////////////////////////////////

void SetAuthorizationHeader(taiga::http::Request& request) {
  const auto access_token = Account::access_token();
  if (!access_token.empty()) {
    request.set_header("Authorization", "Bearer {}"_format(access_token));
  }
}

taiga::http::Request BuildRequest() {
  taiga::http::Request request;

  request.set_headers({
      {"Accept", "application/json"},
      {"Accept-Charset", "utf-8"},
      {"Accept-Encoding", "gzip"}});

  SetAuthorizationHeader(request);

  return request;
}

std::optional<Error> HasError(const taiga::http::Response& response) {
  Error error;

  if (response.error()) {
    error.description = StrToWstr(response.error().str());
    return error;
  }

  if (response.status_class() == 200) {
    return std::nullopt;
  }

  if (const auto value = response.header("www-authenticate"); !value.empty()) {
    error.description = InStr(StrToWstr(value), L"error_description=\"", L"\"");
    if (InStr(StrToWstr(value), L"error=\"", L"\"") == L"invalid_token") {
      error.type = Error::Type::AccessTokenExpired;
      return error;
    }
  }

  if (Json root; JsonParseString(response.body(), root)) {
    if (root.contains("error")) {
      static const std::map<std::string, Error::Type> known_errors{
          {"bad_request", Error::Type::BadRequest},
          {"forbidden", Error::Type::Forbidden},
          {"invalid_request", Error::Type::RefreshTokenExpired}};
      const auto it = known_errors.find(JsonReadStr(root, "error"));
      if (it != known_errors.end()) {
        error.type = it->second;
      }
    }
    if (root.contains("message") && error.description.empty()) {
      error.description = StrToWstr(JsonReadStr(root, "message"));
      if (const auto hint = JsonReadStr(root, "hint"); !hint.empty()) {
        error.description += L" ({})"_format(StrToWstr(hint));
      }
    }
  }

  if (error.description.empty()) {
    if (error.type == Error::Type::Forbidden) {
      error.description = L"Please set up your account via Settings.";
    }
  }
  if (error.description.empty()) {
    error.description = L"Unknown error ({} {})"_format(
        response.status_code(), StrToWstr(response.reason_phrase()));
  }

  return error;
}

void HandleError(const Error& error) {
  switch (error.type) {
    case Error::Type::AccessTokenExpired:
    case Error::Type::RefreshTokenExpired:
      LOGD(error.description);
      break;

    case Error::Type::Forbidden:
    case Error::Type::Other:
    default:
      LOGE(error.description);
      ui::ChangeStatusText(L"MyAnimeList: {}"_format(error.description));
      break;
  }
}

std::wstring GetAnimeFields() {
  return L"alternative_titles,"
         L"average_episode_duration,"
         L"end_date,"
         L"genres,"
         L"id,"
         L"main_picture,"
         L"mean,"
         L"media_type,"
         L"num_episodes,"
         L"popularity,"
         L"rating,"
         L"start_date,"
         L"status,"
         L"studios,"
         L"synopsis,"
         L"title";
}

std::wstring GetListStatusFields() {
  return L"comments,"
         L"finish_date,"
         L"is_rewatching,"
         L"num_times_rewatched,"
         L"num_watched_episodes,"
         L"score,"
         L"start_date,"
         L"status,"
         L"updated_at";
}

std::optional<int> GetOffset(const Json& json, const std::string& name) {
  if (const auto link = JsonReadStr(json["paging"], name); !link.empty()) {
    const Url url = StrToWstr(link);
    const auto query = url.query();
    if (const auto it = query.find(L"offset"); it != query.end()) {
      return ToInt(it->second);
    }
  }
  return std::nullopt;
}

int ParseAnimeObject(const Json& json) {
  const auto anime_id = JsonReadInt(json, "id");

  if (!anime_id) {
    LOGW(L"Could not parse anime object:\n{}", StrToWstr(json.dump()));
    return anime::ID_UNKNOWN;
  }

  auto& anime_item = anime::db.items[anime_id];

  anime_item.SetSource(ServiceId::MyAnimeList);
  anime_item.SetId(ToWstr(anime_id), ServiceId::MyAnimeList);
  anime_item.SetLastModified(time(nullptr));  // current time

  anime_item.SetTitle(StrToWstr(JsonReadStr(json, "title")));
  anime_item.SetDateStart(
      TranslateDateFrom(StrToWstr(JsonReadStr(json, "start_date"))));
  anime_item.SetDateEnd(
      TranslateDateFrom(StrToWstr(JsonReadStr(json, "end_date"))));
  anime_item.SetSynopsis(
      anime::NormalizeSynopsis(StrToWstr(JsonReadStr(json, "synopsis"))));
  anime_item.SetScore(JsonReadDouble(json, "mean"));
  anime_item.SetPopularity(JsonReadInt(json, "popularity"));
  anime_item.SetType(
      TranslateSeriesTypeFrom(StrToWstr(JsonReadStr(json, "media_type"))));
  anime_item.SetAiringStatus(
      TranslateSeriesStatusFrom(StrToWstr(JsonReadStr(json, "status"))));
  anime_item.SetEpisodeCount(JsonReadInt(json, "num_episodes"));
  anime_item.SetEpisodeLength(TranslateEpisodeLengthFrom(
      JsonReadInt(json, "average_episode_duration")));
  anime_item.SetAgeRating(
      TranslateAgeRatingFrom(StrToWstr(JsonReadStr(json, "rating"))));

  if (json.contains("main_picture")) {
    anime_item.SetImageUrl(
        StrToWstr(JsonReadStr(json["main_picture"], "medium")));
  }

  if (json.contains("alternative_titles")) {
    const auto& alternative_titles = json["alternative_titles"];
    if (alternative_titles.contains("synonyms")) {
      std::vector<std::wstring> synonyms;
      for (const auto& synonym : alternative_titles["synonyms"]) {
        if (synonym.is_string())
          synonyms.push_back(StrToWstr(synonym));
      }
      anime_item.SetSynonyms(synonyms);
    }
    anime_item.SetEnglishTitle(
        StrToWstr(JsonReadStr(alternative_titles, "en")));
    anime_item.SetJapaneseTitle(
        StrToWstr(JsonReadStr(alternative_titles, "ja")));
  }

  const auto get_names = [](const Json& json, const std::string& key) {
    std::vector<std::wstring> names;
    if (json.contains(key) && json[key].is_array()) {
      for (const auto& genre : json[key]) {
        const auto name = JsonReadStr(genre, "name");
        if (!name.empty()) {
          names.push_back(StrToWstr(name));
        }
      }
    }
    return names;
  };
  anime_item.SetGenres(get_names(json, "genres"));
  anime_item.SetProducers(std::vector<std::wstring>{});
  anime_item.SetStudios(get_names(json, "studios"));

  Meow.UpdateTitles(anime_item);

  return anime_id;
}

void ParseLibraryObject(const Json& json, const int anime_id) {
  if (!anime_id) {
    LOGW(L"Could not parse anime list entry #{}", anime_id);
    return;
  }

  auto& anime_item = anime::db.items[anime_id];

  anime_item.AddtoUserList();
  anime_item.SetMyStatus(
      TranslateMyStatusFrom(StrToWstr(JsonReadStr(json, "status"))));
  anime_item.SetMyScore(TranslateMyRatingFrom(JsonReadInt(json, "score")));
  anime_item.SetMyLastWatchedEpisode(JsonReadInt(json, "num_episodes_watched"));
  anime_item.SetMyRewatching(JsonReadBool(json, "is_rewatching"));
  anime_item.SetMyDateStart(
      TranslateDateFrom(StrToWstr(JsonReadStr(json, "start_date"))));
  anime_item.SetMyDateEnd(
      TranslateDateFrom(StrToWstr(JsonReadStr(json, "finish_date"))));
  anime_item.SetMyRewatchedTimes(JsonReadInt(json, "num_times_rewatched"));
  anime_item.SetMyLastUpdated(
      TranslateMyLastUpdatedFrom(JsonReadStr(json, "updated_at")));

  std::wstring notes = StrToWstr(JsonReadStr(json, "comments"));
  DecodeHtmlEntities(notes);
  anime_item.SetMyNotes(notes);
}

////////////////////////////////////////////////////////////////////////////////

void RequestAccessToken(const std::wstring& authorization_code,
                        const std::wstring& code_verifier) {
  taiga::http::Request request;
  request.set_method("POST");
  request.set_target("https://myanimelist.net/v1/oauth2/token");
  request.set_header("Content-Type", "application/x-www-form-urlencoded");
  request.set_body({
      {"client_id", kClientId},
      {"grant_type", "authorization_code"},
      {"code", WstrToStr(authorization_code)},
      {"redirect_uri", kRedirectUrl},
      {"code_verifier", WstrToStr(code_verifier)}});

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(RequestType::RequestAccessToken, transfer,
                      L"MyAnimeList: Requesting access token...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (const auto error = HasError(response)) {
      HandleError(*error);
      sync::OnError(RequestType::RequestAccessToken);
      ui::OnMalRequestAccessTokenError(error->description);
      return;
    }

    Json json;

    if (!JsonParseString(response.body(), json)) {
      sync::OnError(RequestType::RequestAccessToken);
      ui::OnMalRequestAccessTokenError(L"Could not parse the response.");
      return;
    }

    Account::set_access_token(JsonReadStr(json, "access_token"));
    Account::set_refresh_token(JsonReadStr(json, "refresh_token"));

    sync::OnResponse(RequestType::RequestAccessToken);
    ui::OnMalRequestAccessTokenSuccess();
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void RefreshAccessToken(std::function<void()> after_response) {
  const auto refresh_token = Account::refresh_token();
  if (refresh_token.empty()) {
    ui::ChangeStatusText(L"MyAnimeList: Refresh token is unavailable.");
    return;
  }

  auto request = BuildRequest();
  request.set_method("POST");
  request.set_target("https://myanimelist.net/v1/oauth2/token");
  request.set_header("Content-Type", "application/x-www-form-urlencoded");
  request.set_body({
      {"client_id", kClientId},
      {"grant_type", "refresh_token"},
      {"refresh_token", refresh_token}});

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(RequestType::RefreshAccessToken, transfer,
                      L"MyAnimeList: Refreshing access token...");
  };

  const auto on_response = [after_response](const taiga::http::Response& response) {
    if (const auto error = HasError(response)) {
      HandleError(*error);
      account.set_authenticated(false);
      if (error->type == Error::Type::RefreshTokenExpired) {
        ui::ChangeStatusText(
            L"MyAnimeList: Refresh token has expired. "
            L"Please re-authorize your account via Settings.");
      }
      sync::OnError(RequestType::RefreshAccessToken);
      return;
    }

    Json root;

    if (!JsonParseString(response.body(), root)) {
      account.set_authenticated(false);
      ui::ChangeStatusText(
          L"MyAnimeList: Could not parse authentication data.");
      sync::OnError(RequestType::RefreshAccessToken);
      return;
    }

    Account::set_access_token(JsonReadStr(root, "access_token"));
    Account::set_refresh_token(JsonReadStr(root, "refresh_token"));
    account.set_authenticated(true);

    sync::OnResponse(RequestType::RefreshAccessToken);

    if (after_response) {
      after_response();
    }
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void RefreshAccessToken() {
  RefreshAccessToken([]() { GetUser(); });
}

void SendRequest(taiga::http::Request request,
                 taiga::http::TransferCallback on_transfer,
                 taiga::http::ResponseCallback on_response) {
  const auto handle_response = [=](const taiga::http::Response& response) {
    if (const auto error = HasError(response)) {
      HandleError(*error);

      if (error->type == Error::Type::AccessTokenExpired) {
        account.set_authenticated(false);
        // Refresh the access token and retry the original request
        RefreshAccessToken([=]() mutable {
          SetAuthorizationHeader(request);
          taiga::http::Send(request, on_transfer, on_response);
        });
        return;
      }
    }

    if (on_response) {
      on_response(response);
    }
  };

  taiga::http::Send(request, on_transfer, handle_response);
}

void GetUser() {
  const auto username = account.authenticated() ? "@me" : Account::username();

  auto request = BuildRequest();
  request.set_target("{}/users/{}"_format(kBaseUrl, username));

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(RequestType::GetUser, transfer,
                      L"MyAnimeList: Retrieving user information...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      sync::OnError(RequestType::GetUser);
      return;
    }

    Json root;

    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse user data.");
      sync::OnError(RequestType::GetUser);
      return;
    }

    Account::set_username(JsonReadStr(root, "name"));

    sync::OnResponse(RequestType::GetUser);

    if (account.authenticated()) {
      sync::Synchronize();
    } else {
      GetLibraryEntries();
    }
  };

  SendRequest(request, on_transfer, on_response);
}

void GetLibraryEntries(const int page_offset) {
  auto request = BuildRequest();
  request.set_target(
      "{}/users/{}/animelist"_format(kBaseUrl, Account::username()));
  request.set_query({
      {"limit", ToStr(kLibraryPageLimit)},
      {"offset", ToStr(page_offset)},
      {"nsfw", "true"},
      {"fields", "{},list_status{{{}}}"_format(
          WstrToStr(GetAnimeFields()), WstrToStr(GetListStatusFields()))}});

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(RequestType::GetLibraryEntries, transfer,
                      L"MyAnimeList: Retrieving anime list...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      sync::OnError(RequestType::GetLibraryEntries);
      return;
    }

    Json root;

    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse anime list.");
      sync::OnError(RequestType::GetLibraryEntries);
      return;
    }

    const auto previous_page_offset = GetOffset(root, "previous");
    const auto next_page_offset = GetOffset(root, "next");

    if (!previous_page_offset) {  // first page
      anime::db.ClearUserData();
    }

    for (const auto& value : root["data"]) {
      if (value.contains("node") && value.contains("list_status")) {
        const auto anime_id = ParseAnimeObject(value["node"]);
        ParseLibraryObject(value["list_status"], anime_id);
      }
    }

    if (next_page_offset && *next_page_offset > 0) {
      GetLibraryEntries(*next_page_offset);
    } else {
      sync::OnResponse(RequestType::GetLibraryEntries);
    }
  };

  SendRequest(request, on_transfer, on_response);
}

void GetMetadataById(const int id) {
  auto request = BuildRequest();
  request.set_target("{}/anime/{}"_format(kBaseUrl, id));
  request.set_query({{"fields", WstrToStr(GetAnimeFields())}});

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(RequestType::GetMetadataById, transfer,
                      L"MyAnimeList: Retrieving anime information...");
  };

  const auto on_response = [id](const taiga::http::Response& response) {
    if (HasError(response)) {
      if (response.status_code() == 404) {
        sync::OnInvalidAnimeId(id);
      } else {
        ui::OnLibraryEntryChangeFailure(id);
      }
      sync::OnError(RequestType::GetMetadataById);
      return;
    }

    Json root;

    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse anime data.");
      ui::OnLibraryEntryChangeFailure(id);
      sync::OnError(RequestType::GetMetadataById);
      return;
    }

    const auto anime_id = ParseAnimeObject(root);

    ui::OnLibraryEntryChange(anime_id);
    sync::OnResponse(RequestType::GetMetadataById);
  };

  SendRequest(request, on_transfer, on_response);
}

void GetSeason(const anime::Season season, const int page_offset) {
  const auto season_year = static_cast<int>(season.year);
  const auto season_name =
      WstrToStr(ToLower_Copy(ui::TranslateSeasonName(season.name)));

  auto request = BuildRequest();
  request.set_target(
      "{}/anime/season/{}/{}"_format(kBaseUrl, season_year, season_name));
  request.set_query({
      {"limit", ToStr(kSeasonPageLimit)},
      {"offset", ToStr(page_offset)},
      {"nsfw", "true"},
      {"fields", WstrToStr(GetAnimeFields())}});

  const auto on_transfer = [season](const taiga::http::Transfer& transfer) {
    return OnTransfer(RequestType::GetSeason, transfer,
        L"MyAnimeList: Retrieving {} anime season..."_format(
            ui::TranslateSeason(season)));
  };

  const auto on_response = [season](const taiga::http::Response& response) {
    if (HasError(response)) {
      sync::OnError(RequestType::GetSeason);
      return;
    }

    Json root;

    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse season data.");
      sync::OnError(RequestType::GetSeason);
      return;
    }

    const auto previous_page_offset = GetOffset(root, "previous");
    const auto next_page_offset = GetOffset(root, "next");

    if (!previous_page_offset) {  // first page
      anime::season_db.items.clear();
    }

    for (const auto& value : root["data"]) {
      const auto anime_id = ParseAnimeObject(value["node"]);
      anime::season_db.items.push_back(anime_id);
      ui::OnLibraryEntryChange(anime_id);
    }

    if (next_page_offset && *next_page_offset > 0) {
      GetSeason(season, *next_page_offset);
    } else {
      sync::OnResponse(RequestType::GetSeason);
    }
  };

  SendRequest(request, on_transfer, on_response);
}

void SearchTitle(const std::wstring& title) {
  auto request = BuildRequest();
  request.set_target("{}/anime"_format(kBaseUrl));
  request.set_query({
      {"q", WstrToStr(title)},
      {"limit", ToStr(kSearchPageLimit)},
      {"nsfw", "true"},
      {"fields", WstrToStr(GetAnimeFields())}});

  const auto on_transfer = [title](const taiga::http::Transfer& transfer) {
    return OnTransfer(RequestType::SearchTitle, transfer,
        L"MyAnimeList: Searching for \"{}\"..."_format(title));
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      sync::OnError(RequestType::SearchTitle);
      return;
    }

    Json root;

    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse search results.");
      sync::OnError(RequestType::SearchTitle);
      return;
    }

    std::vector<int> ids;
    for (const auto& value : root["data"]) {
      const auto anime_id = ParseAnimeObject(value["node"]);
      ids.push_back(anime_id);
    }

    ui::OnLibrarySearchTitle(ids);
    sync::OnResponse(RequestType::SearchTitle);
  };

  SendRequest(request, on_transfer, on_response);
}

void AddLibraryEntry(const library::QueueItem& queue_item) {
  UpdateLibraryEntry(queue_item);
}

void DeleteLibraryEntry(const int id) {
  auto request = BuildRequest();
  request.set_method("DELETE");
  request.set_target("{}/anime/{}/my_list_status"_format(kBaseUrl, id));

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(RequestType::DeleteLibraryEntry, transfer,
                      L"MyAnimeList: Deleting anime from list...");
  };

  const auto on_response = [id](const taiga::http::Response& response) {
    if (const auto error = HasError(response)) {
      if (response.status_code() == 404) {
        // We consider "404 Not Found" to be a success.
      } else {
        ui::OnLibraryUpdateFailure(id, error->description, false);
        sync::OnError(RequestType::DeleteLibraryEntry);
        return;
      }
    }

    sync::OnResponse(RequestType::DeleteLibraryEntry);
  };

  SendRequest(request, on_transfer, on_response);
}

void UpdateLibraryEntry(const library::QueueItem& queue_item) {
  const auto id = queue_item.anime_id;

  auto request = BuildRequest();
  request.set_method("PATCH");
  request.set_target("{}/anime/{}/my_list_status"_format(kBaseUrl, id));
  request.set_query({{"fields", WstrToStr(GetListStatusFields())}});
  request.set_header("Content-Type", "application/x-www-form-urlencoded");

  hypr::Params params;
  if (queue_item.episode)
    params.add("num_watched_episodes", ToStr(*queue_item.episode));
  if (queue_item.status)
    params.add("status", TranslateMyStatusTo(*queue_item.status));
  if (queue_item.score)
    params.add("score", ToStr(TranslateMyRatingTo(*queue_item.score)));
  if (queue_item.date_start)
    params.add("start_date", WstrToStr(queue_item.date_start->to_string()));
  if (queue_item.date_finish)
    params.add("finish_date", WstrToStr(queue_item.date_finish->to_string()));
  if (queue_item.enable_rewatching)
    params.add("is_rewatching", ToStr(*queue_item.enable_rewatching));
  if (queue_item.rewatched_times)
    params.add("num_times_rewatched", ToStr(*queue_item.rewatched_times));
  if (queue_item.notes)
    params.add("comments", WstrToStr(*queue_item.notes));

  request.set_body(params);

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(RequestType::UpdateLibraryEntry, transfer,
                      L"MyAnimeList: Updating anime list...");
  };

  const auto on_response = [id](const taiga::http::Response& response) {
    if (auto error = HasError(response)) {
      if (response.status_code() == 404) {
        error->description = L"Anime list entry does not exist";
      }
      ui::OnLibraryUpdateFailure(id, error->description, false);
      sync::OnError(RequestType::UpdateLibraryEntry);
      return;
    }

    Json root;

    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse anime list entry.");
      sync::OnError(RequestType::UpdateLibraryEntry);
      return;
    }

    ParseLibraryObject(root, id);

    sync::OnResponse(RequestType::UpdateLibraryEntry);
  };

  SendRequest(request, on_transfer, on_response);
}

}  // namespace sync::myanimelist
