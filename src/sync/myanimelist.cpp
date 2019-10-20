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
#include "base/http.h"
#include "base/log.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_item.h"
#include "media/anime_util.h"
#include "sync/myanimelist_util.h"

namespace sync {
namespace myanimelist {

constexpr auto kClientId = L"f6e398095cf7525360276786ec4407bc";

constexpr auto kRedirectUrl = L"https://taiga.moe/api/myanimelist/auth";

constexpr auto kLibraryPageLimit = 1000;
constexpr auto kSearchPageLimit = 100;
constexpr auto kSeasonPageLimit = 500;

Service::Service() {
  host_ = L"api.myanimelist.net";

  id_ = kMyAnimeList;
  canonical_name_ = L"myanimelist";
  name_ = L"MyAnimeList";
}

////////////////////////////////////////////////////////////////////////////////

void Service::BuildRequest(Request& request, HttpRequest& http_request) {
  http_request.url.host = host_;
  http_request.url.protocol = base::http::Protocol::Https;

  http_request.header[L"Accept"] = L"application/json";
  http_request.header[L"Accept-Charset"] = L"utf-8";
  http_request.header[L"Accept-Encoding"] = L"gzip";

  if (RequestNeedsAuthentication(request.type) &&
      request.type != kAuthenticateUser) {
    http_request.header[L"Authorization"] = L"Bearer " + user().access_token;
  }

  switch (request.type) {
    BUILD_HTTP_REQUEST(kAddLibraryEntry, AddLibraryEntry);
    BUILD_HTTP_REQUEST(kAuthenticateUser, AuthenticateUser);
    BUILD_HTTP_REQUEST(kGetUser, GetUser);
    BUILD_HTTP_REQUEST(kDeleteLibraryEntry, DeleteLibraryEntry);
    BUILD_HTTP_REQUEST(kGetLibraryEntries, GetLibraryEntries);
    BUILD_HTTP_REQUEST(kGetMetadataById, GetMetadataById);
    BUILD_HTTP_REQUEST(kGetSeason, GetSeason);
    BUILD_HTTP_REQUEST(kSearchTitle, SearchTitle);
    BUILD_HTTP_REQUEST(kUpdateLibraryEntry, UpdateLibraryEntry);
  }
}

void Service::HandleResponse(Response& response, HttpResponse& http_response) {
  if (RequestSucceeded(response, http_response)) {
    switch (response.type) {
      HANDLE_HTTP_RESPONSE(kAddLibraryEntry, AddLibraryEntry);
      HANDLE_HTTP_RESPONSE(kAuthenticateUser, AuthenticateUser);
      HANDLE_HTTP_RESPONSE(kGetUser, GetUser);
      HANDLE_HTTP_RESPONSE(kDeleteLibraryEntry, DeleteLibraryEntry);
      HANDLE_HTTP_RESPONSE(kGetLibraryEntries, GetLibraryEntries);
      HANDLE_HTTP_RESPONSE(kGetMetadataById, GetMetadataById);
      HANDLE_HTTP_RESPONSE(kGetSeason, GetSeason);
      HANDLE_HTTP_RESPONSE(kSearchTitle, SearchTitle);
      HANDLE_HTTP_RESPONSE(kUpdateLibraryEntry, UpdateLibraryEntry);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Request builders

void Service::AuthenticateUser(Request& request, HttpRequest& http_request) {
  http_request.url.host = L"myanimelist.net";
  http_request.url.path = L"/v1/oauth2/token";

  http_request.method = L"POST";
  http_request.header[L"Content-Type"] = L"application/x-www-form-urlencoded";

  http_request.data[L"client_id"] = kClientId;
  http_request.data[L"grant_type"] = L"refresh_token";
  http_request.data[L"refresh_token"] = user().refresh_token;
}

void Service::GetUser(Request& request, HttpRequest& http_request) {
  const auto username = user().authenticated ?
      L"@me" : request.data[canonical_name_ + L"-username"];
  http_request.url.path = L"/v2/users/{}"_format(username);
}

void Service::GetLibraryEntries(Request& request, HttpRequest& http_request) {
  const auto username = request.data[canonical_name_ + L"-username"];
  http_request.url.path = L"/v2/users/{}/animelist"_format(username);
  http_request.url.query[L"limit"] = ToWstr(kLibraryPageLimit);
  http_request.url.query[L"offset"] = request.data[L"page_offset"];
  http_request.url.query[L"nsfw"] = L"true";
  http_request.url.query[L"fields"] =
      L"{},list_status{{{}}}"_format(GetAnimeFields(), GetListStatusFields());
}

void Service::GetMetadataById(Request& request, HttpRequest& http_request) {
  const auto id = request.data[canonical_name_ + L"-id"];
  http_request.url.path = L"/v2/anime/{}"_format(id);
  http_request.url.query[L"fields"] = GetAnimeFields();
}

void Service::GetSeason(Request& request, HttpRequest& http_request) {
  const auto year = request.data[L"year"];
  const auto season = request.data[L"season"];
  http_request.url.path = L"/v2/anime/season/{}/{}"_format(year, season);
  http_request.url.query[L"limit"] = ToWstr(kSeasonPageLimit);
  http_request.url.query[L"offset"] = request.data[L"page_offset"];
  http_request.url.query[L"nsfw"] = L"true";
  http_request.url.query[L"fields"] = GetAnimeFields();
}

void Service::SearchTitle(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/v2/anime";
  http_request.url.query[L"q"] = request.data[L"title"];
  http_request.url.query[L"limit"] = ToWstr(kSearchPageLimit);
  http_request.url.query[L"nsfw"] = L"true";
  http_request.url.query[L"fields"] = GetAnimeFields();
}

void Service::AddLibraryEntry(Request& request, HttpRequest& http_request) {
  UpdateLibraryEntry(request, http_request);
}

void Service::DeleteLibraryEntry(Request& request, HttpRequest& http_request) {
  const auto id = request.data[canonical_name_ + L"-id"];
  http_request.url.path = L"/v2/anime/{}/my_list_status"_format(id);
  http_request.method = L"DELETE";
}

void Service::UpdateLibraryEntry(Request& request, HttpRequest& http_request) {
  const auto id = request.data[canonical_name_ + L"-id"];
  http_request.url.path = L"/v2/anime/{}/my_list_status"_format(id);
  http_request.url.query[L"fields"] = GetListStatusFields();

  http_request.method = L"PATCH";
  http_request.header[L"Content-Type"] = L"application/x-www-form-urlencoded";

  query_t query;

  for (const auto& pair : request.data) {
    const auto name = TranslateKeyTo(pair.first);
    if (!name.empty()) {
      std::wstring value = pair.second;
      if (name == L"score") {
        value = ToWstr(TranslateMyRatingTo(ToInt(value)));
      } else if (name == L"status") {
        value = TranslateMyStatusTo(ToInt(value));
      }
      query.emplace(name, value);
    }
  }

  http_request.body = BuildUrlParameters(query);
}

////////////////////////////////////////////////////////////////////////////////
// Response handlers

void Service::AuthenticateUser(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  user().access_token = StrToWstr(JsonReadStr(root, "access_token"));
  user().refresh_token = StrToWstr(JsonReadStr(root, "refresh_token"));
}

void Service::GetUser(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  user_.id = StrToWstr(JsonReadStr(root, "id"));
  user_.username = StrToWstr(JsonReadStr(root, "name"));
}

void Service::GetLibraryEntries(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  ParseLinks(root, response);

  if (response.data[L"previous_page_offset"].empty()) {  // first page
    anime::db.ClearUserData();
  }

  for (const auto& value : root["data"]) {
    const auto anime_id = ParseAnimeObject(value["node"]);
    ParseLibraryObject(value["list_status"], anime_id);
  }
}

void Service::GetMetadataById(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  ParseAnimeObject(root);
}

void Service::GetSeason(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  ParseLinks(root, response);

  for (const auto& value : root["data"]) {
    const auto anime_id = ParseAnimeObject(value["node"]);
    AppendString(response.data[L"ids"], ToWstr(anime_id), L",");
  }
}

void Service::SearchTitle(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  for (const auto& value : root["data"]) {
    const auto anime_id = ParseAnimeObject(value["node"]);
    AppendString(response.data[L"ids"], ToWstr(anime_id), L",");
  }
}

void Service::AddLibraryEntry(Response& response, HttpResponse& http_response) {
  UpdateLibraryEntry(response, http_response);
}

void Service::DeleteLibraryEntry(Response& response, HttpResponse& http_response) {
  // Returns either "200 OK" or "404 Not Found"
}

void Service::UpdateLibraryEntry(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  const int anime_id = ToInt(response.data[L"taiga-id"]);
  ParseLibraryObject(root["data"], anime_id);
}

////////////////////////////////////////////////////////////////////////////////

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

bool Service::RequestSucceeded(Response& response,
                               const HttpResponse& http_response) {
  if (http_response.GetStatusCategory() == 200)
    return true;

  switch (response.type) {
    case kGetMetadataById:
      // Invalid anime ID
      if (http_response.code == 404)
        response.data[L"invalid_id"] = L"true";
      break;
    case kDeleteLibraryEntry:
      // Anime does not exist in user's list
      if (http_response.code == 404)
        return true;
      break;
  }

  if (http_response.code == 401) {
    user().authenticated = false;
    // @TODO
    // WWW-Authenticate: Bearer error="invalid_token",error_description="The access token expired"
    response.data[L"error"] = L"401 Unauthorized";
  }

  Json root;
  std::wstring error_description;
  if (response.data[L"error"].empty() &&
      JsonParseString(http_response.body, root)) {
    error_description = StrToWstr(JsonReadStr(root, "message"));
    if (const auto hint = JsonReadStr(root, "hint"); !hint.empty()) {
      error_description += L" ({})"_format(StrToWstr(hint));
    }
  }
  response.data[L"error"] = error_description;

  HandleError(http_response, response);
  return false;
}

////////////////////////////////////////////////////////////////////////////////

int Service::ParseAnimeObject(const Json& json) const {
  const auto anime_id = JsonReadInt(json, "id");

  if (!anime_id) {
    LOGW(L"Could not parse anime object:\n{}", StrToWstr(json.dump()));
    return anime::ID_UNKNOWN;
  }

  anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_id), this->id());
  anime_item.SetLastModified(time(nullptr));  // current time

  anime_item.SetTitle(StrToWstr(JsonReadStr(json, "title")));
  anime_item.SetDateStart(StrToWstr(JsonReadStr(json, "start_date")));
  anime_item.SetDateEnd(StrToWstr(JsonReadStr(json, "end_date")));
  anime_item.SetSynopsis(anime::NormalizeSynopsis(StrToWstr(JsonReadStr(json, "synopsis"))));
  anime_item.SetScore(JsonReadDouble(json, "mean"));
  anime_item.SetPopularity(JsonReadInt(json, "popularity"));
  anime_item.SetType(TranslateSeriesTypeFrom(StrToWstr(JsonReadStr(json, "media_type"))));
  anime_item.SetAiringStatus(TranslateSeriesStatusFrom(StrToWstr(JsonReadStr(json, "status"))));
  anime_item.SetEpisodeCount(JsonReadInt(json, "num_episodes"));
  anime_item.SetEpisodeLength(TranslateEpisodeLengthFrom(JsonReadInt(json, "average_episode_duration")));
  anime_item.SetAgeRating(TranslateAgeRatingFrom(StrToWstr(JsonReadStr(json, "rating"))));

  if (json.contains("main_picture")) {
    anime_item.SetImageUrl(StrToWstr(JsonReadStr(json["main_picture"], "medium")));
  }

  if (json.contains("alternative_titles")) {
    const auto& alternative_titles = json["alternative_titles"];
    if (alternative_titles.contains("synonyms")) {
      for (const auto& title : alternative_titles["synonyms"]) {
        if (title.is_string())
          anime_item.InsertSynonym(StrToWstr(title));
      }
    }
    anime_item.SetEnglishTitle(StrToWstr(JsonReadStr(alternative_titles, "en")));
    anime_item.SetJapaneseTitle(StrToWstr(JsonReadStr(alternative_titles, "ja")));
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

int Service::ParseLibraryObject(const Json& json, int anime_id) const {
  if (!anime_id) {
    LOGW(L"Could not parse anime list entry #{}", anime_id);
    return anime::ID_UNKNOWN;
  }

  anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_id), this->id());
  anime_item.AddtoUserList();

  anime_item.SetMyStatus(TranslateMyStatusFrom(StrToWstr(JsonReadStr(json, "status"))));
  anime_item.SetMyScore(TranslateMyRatingFrom(JsonReadInt(json, "score")));
  anime_item.SetMyLastWatchedEpisode(JsonReadInt(json, "num_episodes_watched"));
  anime_item.SetMyRewatching(JsonReadBool(json, "is_rewatching"));
  anime_item.SetMyDateStart(StrToWstr(JsonReadStr(json, "start_date")));
  anime_item.SetMyDateEnd(StrToWstr(JsonReadStr(json, "finish_date")));
  anime_item.SetMyRewatchedTimes(JsonReadInt(json, "num_times_rewatched"));
  anime_item.SetMyTags(StrToWstr(JsonReadStr(json, "tags")));
  anime_item.SetMyNotes(StrToWstr(JsonReadStr(json, "comments")));
  anime_item.SetMyLastUpdated(TranslateMyLastUpdatedFrom(JsonReadStr(json, "updated_at")));

  return anime::db.UpdateItem(anime_item);
}

void Service::ParseLinks(const Json& json, Response& response) const {
  const auto parse_link = [&](const std::string& name) {
    const auto link = JsonReadStr(json["paging"], name);
    if (!link.empty()) {
      Url url = StrToWstr(link);
      response.data[StrToWstr(name) + L"_page_offset"] = url.query[L"offset"];
    }
  };

  parse_link("previous");
  parse_link("next");
}

bool Service::ParseResponseBody(const std::wstring& body,
                                Response& response, Json& json) {
  if (JsonParseString(body, json))
    return true;

  switch (response.type) {
    case kGetLibraryEntries:
      response.data[L"error"] = L"Could not parse anime list";
      break;
    case kGetMetadataById:
      response.data[L"error"] = L"Could not parse anime object";
      break;
    case kGetSeason:
      response.data[L"error"] = L"Could not parse season data";
      break;
    case kSearchTitle:
      response.data[L"error"] = L"Could not parse search results";
      break;
    case kUpdateLibraryEntry:
      response.data[L"error"] = L"Could not parse anime list entry";
      break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring Service::GetAnimeFields() const {
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

std::wstring Service::GetListStatusFields() const {
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

}  // namespace myanimelist
}  // namespace sync
