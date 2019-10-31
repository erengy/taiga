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

#include "sync/myanimelist.h"

#include "base/format.h"
#include "base/log.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_item.h"
#include "media/anime_season.h"
#include "media/anime_season_db.h"
#include "media/anime_util.h"
#include "media/library/queue.h"
#include "sync/manager.h"
#include "sync/myanimelist_util.h"
#include "sync/sync.h"
#include "taiga/http_new.h"
#include "taiga/settings.h"
#include "ui/resource.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace sync::myanimelist {

// @TODO: Link to API documentation when available

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

////////////////////////////////////////////////////////////////////////////////

static taiga::http::Request BuildRequest() {
  taiga::http::Request request;

  request.set_headers({
      {"Accept", "application/json"},
      {"Accept-Charset", "utf-8"},
      {"Accept-Encoding", "gzip"}});

  // Add authentication
  const auto access_token = Account::access_token();
  if (!access_token.empty()) {
    request.set_header("Authorization", "Bearer {}"_format(access_token));
  }

  return request;
}

static bool HasError(const taiga::http::Response& response) {
  if (response.error()) {
    LOGE(StrToWstr(response.error().str()));
    return true;
  }

  if (response.status_code() == 200) {
    return false;
  }

  if (response.status_code() == 401) {
    // WWW-Authenticate:
    // Bearer error="invalid_token",error_description="The access token expired"
    const auto value =
        StrToWstr(std::string{response.header("www-authenticate")});
    if (!value.empty()) {
      const auto error = InStr(value, L"error=\"", L"\"");
      if (error == L"invalid_token") {
        // @TODO: Access token expired
      }
      const auto error_description =
          InStr(value, L"error_description=\"", L"\"");
      if (!error_description.empty()) {
        LOGE(error_description);
        return true;
      }
    }
  }

  if (Json root; JsonParseString(response.body(), root)) {
    const auto error = StrToWstr(JsonReadStr(root, "error"));
    if (error == L"invalid_request") {
      // @TODO: Refresh token expired
    }
    auto error_description = StrToWstr(JsonReadStr(root, "message"));
    if (const auto hint = JsonReadStr(root, "hint"); !hint.empty()) {
      error_description += L" ({})"_format(StrToWstr(hint));
    }
    if (!error_description.empty()) {
      LOGE(error_description);
      return true;
    }
  }

  return false;
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
         L"tags,"
         L"updated_at";
}

int GetOffset(const Json& json, const std::string& name) {
  if (const auto link = JsonReadStr(json["paging"], name); !link.empty()) {
    Url url = StrToWstr(link);
    return ToInt(url.query[L"offset"]);
  }
  return 0;
}

int ParseAnimeObject(const Json& json) {
  const auto anime_id = JsonReadInt(json, "id");

  if (!anime_id) {
    LOGW(L"Could not parse anime object:\n{}", StrToWstr(json.dump()));
    return anime::ID_UNKNOWN;
  }

  anime::Item anime_item;
  anime_item.SetSource(kMyAnimeList);
  anime_item.SetId(ToWstr(anime_id), kMyAnimeList);
  anime_item.SetLastModified(time(nullptr));  // current time

  anime_item.SetTitle(StrToWstr(JsonReadStr(json, "title")));
  anime_item.SetDateStart(StrToWstr(JsonReadStr(json, "start_date")));
  anime_item.SetDateEnd(StrToWstr(JsonReadStr(json, "end_date")));
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
      for (const auto& title : alternative_titles["synonyms"]) {
        if (title.is_string())
          anime_item.InsertSynonym(StrToWstr(title));
      }
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
  anime_item.SetProducers(get_names(json, "studios"));

  return anime::db.UpdateItem(anime_item);
}

void ParseLibraryObject(const Json& json, const int anime_id) {
  if (!anime_id) {
    LOGW(L"Could not parse anime list entry #{}", anime_id);
    return;
  }

  anime::Item anime_item;
  anime_item.SetSource(kMyAnimeList);
  anime_item.SetId(ToWstr(anime_id), kMyAnimeList);
  anime_item.AddtoUserList();

  anime_item.SetMyStatus(
      TranslateMyStatusFrom(StrToWstr(JsonReadStr(json, "status"))));
  anime_item.SetMyScore(TranslateMyRatingFrom(JsonReadInt(json, "score")));
  anime_item.SetMyLastWatchedEpisode(JsonReadInt(json, "num_episodes_watched"));
  anime_item.SetMyRewatching(JsonReadBool(json, "is_rewatching"));
  anime_item.SetMyDateStart(StrToWstr(JsonReadStr(json, "start_date")));
  anime_item.SetMyDateEnd(StrToWstr(JsonReadStr(json, "finish_date")));
  anime_item.SetMyRewatchedTimes(JsonReadInt(json, "num_times_rewatched"));
  anime_item.SetMyTags(StrToWstr(JsonReadStr(json, "tags")));
  anime_item.SetMyNotes(StrToWstr(JsonReadStr(json, "comments")));
  anime_item.SetMyLastUpdated(
      TranslateMyLastUpdatedFrom(JsonReadStr(json, "updated_at")));

  anime::db.UpdateItem(anime_item);
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
    return OnTransfer(transfer, L"MyAnimeList: Requesting access token...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      return;
    }

    Json json;
    if (!JsonParseString(response.body(), json)) {
      ui::OnMalRequestAccessToken(false);
      return;
    }

    const auto access_token = JsonReadStr(json, "access_token");
    const auto refresh_token = JsonReadStr(json, "refresh_token");
    if (!access_token.empty() && !refresh_token.empty()) {
      Account::set_access_token(access_token);
      Account::set_refresh_token(refresh_token);
      ui::OnMalRequestAccessToken(true);
    }
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void AuthenticateUser() {
  const auto refresh_token = Account::refresh_token();
  if (refresh_token.empty()) {
    // @TODO
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
    return OnTransfer(transfer, L"MyAnimeList: Authenticating user...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      account.set_authenticated(false);
      ui::OnLogout();
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(
          L"MyAnimeList: Could not parse authentication data.");
      return;
    }

    Account::set_access_token(JsonReadStr(root, "access_token"));
    Account::set_refresh_token(JsonReadStr(root, "refresh_token"));
    account.set_authenticated(true);

    ui::OnLogin();
    sync::Synchronize();
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void GetUser() {
  const auto username = account.authenticated() ? "@me" : Account::username();

  auto request = BuildRequest();
  request.set_target("{}/users/{}"_format(kBaseUrl, username));

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer, L"MyAnimeList: Retrieving user information...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse user data.");
      return;
    }

    Account::set_username(JsonReadStr(root, "name"));
  };

  taiga::http::Send(request, on_transfer, on_response);
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
    return OnTransfer(transfer, L"MyAnimeList: Retrieving anime list...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      ui::OnLibraryChangeFailure();
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse anime list.");
      return;
    }

    const auto previous_page_offset = GetOffset(root, "previous");
    const auto next_page_offset = GetOffset(root, "next");

    if (previous_page_offset == 0) {  // first page
      anime::db.ClearUserData();
    }

    for (const auto& value : root["data"]) {
      if (value.contains("node") && value.contains("list_status")) {
        const auto anime_id = ParseAnimeObject(value["node"]);
        ParseLibraryObject(value["list_status"], anime_id);
      }
    }

    if (next_page_offset > 0) {
      GetLibraryEntries(next_page_offset);
    } else {
      sync::AfterGetLibrary();
    }
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void GetMetadataById(const int id) {
  auto request = BuildRequest();
  request.set_target("{}/anime/{}"_format(kBaseUrl, id));
  request.set_query({{"fields", WstrToStr(GetAnimeFields())}});

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer, L"MyAnimeList: Retrieving anime information...");
  };

  const auto on_response = [id](const taiga::http::Response& response) {
    if (HasError(response)) {
      ui::OnLibraryEntryChangeFailure(id);

      if (response.status_code() == 404) {
        const auto anime_item = anime::db.Find(id);
        if (anime_item && anime::db.DeleteItem(id)) {
          anime::db.SaveDatabase();
          if (const bool in_list = anime_item && anime_item->IsInList()) {
            anime::db.SaveList();
          }
        }
      }

      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse anime data.");
      return;
    }

    const auto anime_id = ParseAnimeObject(root);
    ui::OnLibraryEntryChange(anime_id);
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void GetSeason(const anime::Season season, const int page_offset) {
  const auto season_name =
      WstrToStr(ToLower_Copy(ui::TranslateSeasonName(season.name)));

  auto request = BuildRequest();
  request.set_target(
      "{}/anime/season/{}/{}"_format(kBaseUrl, season.year, season_name));
  request.set_query({
      {"limit", ToStr(kSeasonPageLimit)},
      {"offset", ToStr(page_offset)},
      {"nsfw", "true"},
      {"fields", WstrToStr(GetAnimeFields())}});

  const auto on_transfer = [season](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer,
        L"MyAnimeList: Retrieving {} anime season... ({})"_format(
            ui::TranslateSeason(season)));
  };

  const auto on_response = [season](const taiga::http::Response& response) {
    if (HasError(response)) {
      ui::OnSeasonLoadFail();
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse season data.");
      return;
    }

    const auto previous_page_offset = GetOffset(root, "previous");
    const auto next_page_offset = GetOffset(root, "next");

    if (previous_page_offset == 0) {  // first page
      anime::season_db.items.clear();
    }

    for (const auto& value : root["data"]) {
      const auto anime_id = ParseAnimeObject(value["node"]);
      anime::season_db.items.push_back(anime_id);
      ui::OnLibraryEntryChange(anime_id);
    }

    if (next_page_offset > 0) {
      GetSeason(season, next_page_offset);
    } else {
      sync::AfterGetSeason();
    }
  };

  taiga::http::Send(request, on_transfer, on_response);
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
    return OnTransfer(transfer,
        L"MyAnimeList: Searching for \"{}\"... ({})"_format(title));
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse search results.");
      return;
    }

    std::wstring ids;
    for (const auto& value : root["data"]) {
      const auto anime_id = ParseAnimeObject(value["node"]);
      AppendString(ids, ToWstr(anime_id), L",");
    }
    ui::OnLibrarySearchTitle(anime::ID_UNKNOWN, ids);
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void AddLibraryEntry(const library::QueueItem& queue_item) {
  UpdateLibraryEntry(queue_item);
}

void DeleteLibraryEntry(const int id) {
  auto request = BuildRequest();
  request.set_method("DELETE");
  request.set_target("{}/anime/{}/my_list_status"_format(kBaseUrl, id));

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer, L"MyAnimeList: Deleting anime from list...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      library::queue.updating = false;
      return;
    }

    // Returns either "200 OK" or "404 Not Found"
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void UpdateLibraryEntry(const library::QueueItem& queue_item) {
  const auto id = queue_item.anime_id;

  auto request = BuildRequest();
  request.set_method("PATCH");
  request.set_target("{}/anime/{}/my_list_status"_format(kBaseUrl, id));
  request.set_query({{"fields", WstrToStr(GetListStatusFields())}});
  request.set_header("Content-Type", "application/x-www-form-urlencoded");

  query_t fields;
  if (queue_item.episode)
    fields[L"num_watched_episodes"] = ToWstr(*queue_item.episode);
  if (queue_item.status)
    fields[L"status"] = ToWstr(*queue_item.status);
  if (queue_item.score)
    fields[L"score"] = ToWstr(*queue_item.score);
  if (queue_item.date_start)
    fields[L"start_date"] = queue_item.date_start->to_string();
  if (queue_item.date_finish)
    fields[L"finish_date"] = queue_item.date_finish->to_string();
  if (queue_item.enable_rewatching)
    fields[L"is_rewatching"] = ToWstr(*queue_item.enable_rewatching);
  if (queue_item.rewatched_times)
    fields[L"num_times_rewatched"] = ToWstr(*queue_item.rewatched_times);
  if (queue_item.tags)
    fields[L"tags"] = *queue_item.tags;
  if (queue_item.notes)
    fields[L"comments"] = *queue_item.notes;

  request.set_body(hypr::Body{WstrToStr(BuildUrlParameters(fields))});

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer, L"MyAnimeList: Updating anime list...");
  };

  const auto on_response = [id](const taiga::http::Response& response) {
    if (HasError(response)) {
      library::queue.updating = false;
      return;
    }

    if (response.status_code() == 404) {
      // @TODO: Anime does not exist in user's list
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"MyAnimeList: Could not parse anime list entry.");
      return;
    }

    ParseLibraryObject(root["data"], id);

    sync::AfterLibraryUpdate();
  };

  taiga::http::Send(request, on_transfer, on_response);
}

////////////////////////////////////////////////////////////////////////////////

Service::Service() {
  host_ = L"api.myanimelist.net";

  id_ = kMyAnimeList;
  canonical_name_ = L"myanimelist";
  name_ = L"MyAnimeList";
}

bool Service::RequestNeedsAuthentication(RequestType request_type) const {
  switch (request_type) {
    case kAddLibraryEntry:
    case kDeleteLibraryEntry:
    case kGetLibraryEntries:
    case kGetMetadataById:
    case kGetSeason:
    case kGetUser:
    case kSearchTitle:
    case kUpdateLibraryEntry:
      return true;
  }

  return false;
}

}  // namespace sync::myanimelist
