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

#include "base/format.h"
#include "base/http.h"
#include "base/log.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_item.h"
#include "media/anime_util.h"
#include "sync/anilist.h"
#include "sync/anilist_util.h"
#include "taiga/settings.h"

namespace sync {
namespace anilist {

constexpr auto kRepeatingMediaListStatus = "REPEATING";

Service::Service() {
  host_ = L"graphql.anilist.co";

  id_ = kAniList;
  canonical_name_ = L"anilist";
  name_ = L"AniList";
}

////////////////////////////////////////////////////////////////////////////////

void Service::BuildRequest(Request& request, HttpRequest& http_request) {
  http_request.url.host = host_;
  http_request.url.protocol = base::http::Protocol::Https;
  http_request.method = L"POST";

  http_request.header[L"Accept"] = L"application/json";
  http_request.header[L"Accept-Charset"] = L"utf-8";
  http_request.header[L"Accept-Encoding"] = L"gzip";
  http_request.header[L"Content-Type"] = L"application/json";

  if (RequestNeedsAuthentication(request.type))
    http_request.header[L"Authorization"] = L"Bearer " + user().access_token;

  switch (request.type) {
    BUILD_HTTP_REQUEST(kAddLibraryEntry, AddLibraryEntry);
    BUILD_HTTP_REQUEST(kAuthenticateUser, AuthenticateUser);
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
  constexpr auto query{R"(
{
  Viewer {
    id
    name
    mediaListOptions {
      scoreFormat
    }
  }
})"
  };

  http_request.body = BuildRequestBody(ExpandQuery(query), nullptr);
}

void Service::GetLibraryEntries(Request& request, HttpRequest& http_request) {
  constexpr auto query{R"(
query ($userName: String!) {
  MediaListCollection (userName: $userName, type: ANIME) {
    lists {
      entries {
        ...mediaListFragment
      }
    }
  }
}

fragment mediaListFragment on MediaList {
  {mediaListFields}
  media {
    ...mediaFragment
  }
}

fragment mediaFragment on Media {
  {mediaFields}
})"
  };

  const Json variables{
    {"userName", WstrToStr(request.data[canonical_name_ + L"-username"])},
  };

  http_request.body = BuildRequestBody(ExpandQuery(query), variables);
}

void Service::GetMetadataById(Request& request, HttpRequest& http_request) {
  constexpr auto query{R"(
query ($id: Int!) {
  Media (id: $id, type: ANIME) {
    {mediaFields}
  }
})"
  };

  const Json variables{
    {"id", ToInt(request.data[canonical_name_ + L"-id"])},
  };

  http_request.body = BuildRequestBody(ExpandQuery(query), variables);
}

void Service::GetSeason(Request& request, HttpRequest& http_request) {
  constexpr auto query{R"(
query ($season: MediaSeason!, $seasonYear: Int!, $page: Int) {
  Page(page: $page) {
    media(season: $season, seasonYear: $seasonYear, type: ANIME, sort: START_DATE) {
      {mediaFields}
    }
    pageInfo {
      total
      perPage
      currentPage
      lastPage
      hasNextPage
    }
  }
})"
  };

  const int page = request.data.count(L"page_offset") ?
      ToInt(request.data[L"page_offset"]) : 1;

  const Json variables{
    {"season", TranslateSeasonTo(request.data[L"season"])},
    {"seasonYear", ToInt(request.data[L"year"])},
    {"page", page},
  };

  http_request.body = BuildRequestBody(ExpandQuery(query), variables);
}

void Service::SearchTitle(Request& request, HttpRequest& http_request) {
  constexpr auto query{R"(
query ($query: String!) {
  Page {
    media(search: $query, type: ANIME) {
      {mediaFields}
    }
  }
})"
  };

  const Json variables{
    {"query", WstrToStr(request.data[L"title"])},
  };

  http_request.body = BuildRequestBody(ExpandQuery(query), variables);
}

void Service::AddLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.body = BuildLibraryObject(request);
}

void Service::DeleteLibraryEntry(Request& request, HttpRequest& http_request) {
  constexpr auto query{R"(
mutation ($id: Int!) {
  DeleteMediaListEntry (id: $id) {
    deleted
  }
})"
  };

  const Json variables{
    {"id", ToInt(request.data[canonical_name_ + L"-library-id"])},
  };

  http_request.body = BuildRequestBody(query, variables);
}

void Service::UpdateLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.body = BuildLibraryObject(request);
}

////////////////////////////////////////////////////////////////////////////////
// Response handlers

void Service::AuthenticateUser(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  ParseUserObject(root["data"]["Viewer"]);
}

void Service::GetLibraryEntries(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  anime::db.ClearUserData();

  const auto& lists = root["data"]["MediaListCollection"]["lists"];
  for (const auto& list : lists) {
    const auto& entries = list["entries"];
    for (const auto& entry : entries) {
      ParseMediaListObject(entry);
    }
  }
}

void Service::GetMetadataById(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  ParseMediaObject(root["data"]["Media"]);
}

void Service::GetSeason(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  const auto& page = root["data"]["Page"];

  for (const auto& media : page["media"]) {
    const auto anime_id = ParseMediaObject(media);
    AppendString(response.data[L"ids"], ToWstr(anime_id), L",");
  }

  const auto& page_info = page["pageInfo"];
  if (JsonReadBool(page_info, "hasNextPage")) {
    const int current_page = JsonReadInt(page_info, "currentPage");
    response.data[L"next_page_offset"] = ToWstr(current_page + 1);
  }
}

void Service::SearchTitle(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  for (const auto& media : root["data"]["Page"]["media"]) {
    const auto anime_id = ParseMediaObject(media);
    AppendString(response.data[L"ids"], ToWstr(anime_id), L",");
  }
}

void Service::AddLibraryEntry(Response& response, HttpResponse& http_response) {
  UpdateLibraryEntry(response, http_response);
}

void Service::DeleteLibraryEntry(Response& response, HttpResponse& http_response) {
  // Returns: {"data":{"DeleteMediaListEntry":{"deleted":true}}}
}

void Service::UpdateLibraryEntry(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  ParseMediaListObject(root["data"]["SaveMediaListEntry"]);
}

////////////////////////////////////////////////////////////////////////////////

bool Service::RequestNeedsAuthentication(RequestType request_type) const {
  switch (request_type) {
    case kAuthenticateUser:
    case kAddLibraryEntry:
    case kDeleteLibraryEntry:
    case kUpdateLibraryEntry:
      return true;
    case kGetLibraryEntries:
    case kGetMetadataById:
    case kGetSeason:
    case kSearchTitle:
      return !user().access_token.empty();
  }

  return false;
}

bool Service::RequestSucceeded(Response& response,
                               const HttpResponse& http_response) {
  if (http_response.GetStatusCategory() == 200)
    return true;

  // Error
  Json root;
  std::wstring error_description;

  if (JsonParseString(http_response.body, root)) {
    if (root.count("errors")) {
      const auto& errors = root["errors"];
      if (errors.is_array() && !errors.empty()) {
        const auto& error = errors.front();
        error_description = StrToWstr(JsonReadStr(error, "message"));
        if (error.count("validation")) {
          const auto& validation = error["validation"];
          error_description = L"Validation error: " + StrToWstr(
              validation.front().front().get<std::string>());
        }
      }
    }
  }

  response.data[L"error"] = error_description;
  HandleError(http_response, response);

  return false;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring Service::BuildLibraryObject(Request& request) const {
  constexpr auto query{R"(
mutation (
    $id: Int,
    $mediaId: Int,
    $status: MediaListStatus,
    $scoreRaw: Int,
    $progress: Int,
    $repeat: Int,
    $notes: String,
    $startedAt: FuzzyDateInput,
    $completedAt: FuzzyDateInput) {
  SaveMediaListEntry (
      id: $id,
      mediaId: $mediaId,
      status: $status,
      scoreRaw: $scoreRaw,
      progress: $progress,
      repeat: $repeat,
      notes: $notes,
      startedAt: $startedAt,
      completedAt: $completedAt) {
    {mediaListFields}
    media {
      {mediaFields}
    }
  }
})"
  };

  Json variables{
    {"mediaId", ToInt(request.data[canonical_name_ + L"-id"])},
  };

  const auto library_id = ToInt(request.data[canonical_name_ + L"-library-id"]);
  if (library_id)
    variables["id"] = library_id;

  std::string status;
  if (request.data.count(L"status"))
    status = TranslateMyStatusTo(ToInt(request.data[L"status"]));
  if (request.data.count(L"enable_rewatching") &&
      ToBool(request.data[L"enable_rewatching"])) {
    status = kRepeatingMediaListStatus;
  }
  if (!status.empty())
    variables["status"] = status;

  if (request.data.count(L"score"))
    variables["scoreRaw"] = ToInt(request.data[L"score"]);
  if (request.data.count(L"episode"))
    variables["progress"] = ToInt(request.data[L"episode"]);
  if (request.data.count(L"rewatched_times"))
    variables["repeat"] = ToInt(request.data[L"rewatched_times"]);
  if (request.data.count(L"notes"))
    variables["notes"] = WstrToStr(request.data[L"notes"]);
  if (request.data.count(L"date_start"))
    variables["startedAt"] = TranslateFuzzyDateTo(Date(request.data[L"date_start"]));
  if (request.data.count(L"date_finish"))
    variables["completedAt"] = TranslateFuzzyDateTo(Date(request.data[L"date_finish"]));

  return BuildRequestBody(ExpandQuery(query), variables);
}

std::wstring Service::BuildRequestBody(const std::string& query,
                                       const Json& variables) const {
  const Json json{
    {"query", query},
    {"variables", variables}
  };

  return StrToWstr(json.dump());
}

int Service::ParseMediaObject(const Json& json) const {
  const auto anime_id = JsonReadInt(json, "id");

  if (!anime_id) {
    LOGW(L"Could not parse anime object:\n{}", StrToWstr(json.dump()));
    return anime::ID_UNKNOWN;
  }

  anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_id), this->id());
  anime_item.SetLastModified(time(nullptr));  // current time

  anime_item.SetTitle(StrToWstr(JsonReadStr(json["title"], "userPreferred")));
  anime_item.SetType(TranslateSeriesTypeFrom(JsonReadStr(json, "format")));
  anime_item.SetSynopsis(anime::NormalizeSynopsis(StrToWstr(JsonReadStr(json, "description"))));
  anime_item.SetDateStart(TranslateFuzzyDateFrom(json["startDate"]));
  anime_item.SetDateEnd(TranslateFuzzyDateFrom(json["endDate"]));
  anime_item.SetEpisodeCount(JsonReadInt(json, "episodes"));
  anime_item.SetEpisodeLength(JsonReadInt(json, "duration"));
  anime_item.SetImageUrl(StrToWstr(JsonReadStr(json["coverImage"], "large")));
  anime_item.SetScore(TranslateSeriesRatingFrom(JsonReadInt(json, "averageScore")));
  anime_item.SetPopularity(JsonReadInt(json, "popularity"));

  ParseMediaTitleObject(json, anime_item);

  std::vector<std::wstring> genres;
  for (const auto& genre : json["genres"]) {
    if (genre.is_string())
      genres.push_back(StrToWstr(genre));
  }
  anime_item.SetGenres(genres);

  for (const auto& synonym : json["synonyms"]) {
    if (synonym.is_string())
      anime_item.InsertSynonym(StrToWstr(synonym));
  }

  std::vector<std::wstring> studios;
  for (const auto& edge : json["studios"]["edges"]) {
    studios.push_back(StrToWstr(JsonReadStr(edge["node"], "name")));
  }
  RemoveEmptyStrings(studios);
  anime_item.SetProducers(studios);

  const auto& next_airing_episode = json["nextAiringEpisode"];
  if (!next_airing_episode.is_null()) {
    anime_item.SetNextEpisodeTime(JsonReadInt(next_airing_episode, "airingAt"));
    const int episode_number = JsonReadInt(next_airing_episode, "episode");
    if (episode_number > 0) {
      anime_item.SetLastAiredEpisodeNumber(episode_number - 1);
    }
  }

  return anime::db.UpdateItem(anime_item);
}

int Service::ParseMediaListObject(const Json& json) const {
  const auto anime_id = JsonReadInt(json["media"], "id");
  const auto library_id = JsonReadInt(json, "id");

  if (!anime_id) {
    LOGW(L"Could not parse library entry #{}", library_id);
    return anime::ID_UNKNOWN;
  }

  ParseMediaObject(json["media"]);

  anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_id), this->id());
  anime_item.AddtoUserList();

  anime_item.SetMyId(ToWstr(library_id));

  const auto status = JsonReadStr(json, "status");
  if (status == kRepeatingMediaListStatus) {
    anime_item.SetMyStatus(anime::kWatching);
    anime_item.SetMyRewatching(true);
  } else {
    anime_item.SetMyStatus(TranslateMyStatusFrom(status));
  }

  anime_item.SetMyScore(JsonReadInt(json, "score"));
  anime_item.SetMyLastWatchedEpisode(JsonReadInt(json, "progress"));
  anime_item.SetMyRewatchedTimes(JsonReadInt(json, "repeat"));
  anime_item.SetMyNotes(StrToWstr(JsonReadStr(json, "notes")));
  anime_item.SetMyDateStart(TranslateFuzzyDateFrom(json["startedAt"]));
  anime_item.SetMyDateEnd(TranslateFuzzyDateFrom(json["completedAt"]));
  anime_item.SetMyLastUpdated(ToWstr(JsonReadInt(json, "updatedAt")));

  return anime::db.UpdateItem(anime_item);
}

void Service::ParseMediaTitleObject(const Json& json,
                                    anime::Item& anime_item) const {
  enum class TitleLanguage {
    Romaji,
    English,
    Native,
  };

  static const std::map<std::string, TitleLanguage> title_languages{
    {"romaji", TitleLanguage::Romaji},
    {"english", TitleLanguage::English},
    {"native", TitleLanguage::Native},
  };

  const auto& titles = json["title"];
  const auto origin = StrToWstr(JsonReadStr(json, "countryOfOrigin"));

  for (auto it = titles.begin(); it != titles.end(); ++it) {
    auto language = title_languages.find(it.key());
    if (language == title_languages.end() || !it->is_string())
      continue;

    const auto title = StrToWstr(it.value());

    switch (language->second) {
      case TitleLanguage::Romaji:
        anime_item.SetTitle(title);
        break;
      case TitleLanguage::English:
        anime_item.SetEnglishTitle(title);
        break;
      case TitleLanguage::Native:
        if (IsEqual(origin, L"JP")) {
          anime_item.SetJapaneseTitle(title);
        } else if (!origin.empty()) {
          anime_item.InsertSynonym(title);
        }
        break;
    }
  }
}

void Service::ParseUserObject(const Json& json) {
  user_.id = ToWstr(JsonReadInt(json, "id"));
  user_.username = StrToWstr(JsonReadStr(json, "name"));
  user_.rating_system =
      StrToWstr(JsonReadStr(json["mediaListOptions"], "scoreFormat"));

  taiga::settings.SetSyncServiceAniListRatingSystem(user_.rating_system);
}

bool Service::ParseResponseBody(const std::wstring& body,
                                Response& response, Json& json) {
  if (JsonParseString(body, json))
    return true;

  switch (response.type) {
    case kGetLibraryEntries:
      response.data[L"error"] = L"Could not parse library entries";
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
      response.data[L"error"] = L"Could not parse library entry";
      break;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

std::string Service::ExpandQuery(const std::string& query) const {
  auto str = StrToWstr(query);

  ReplaceString(str, L"{mediaFields}", GetMediaFields());
  ReplaceString(str, L"{mediaListFields}", GetMediaListFields());

  ReplaceChar(str, '\n', ' ');
  while (ReplaceString(str, L"  ", L" "));

  return WstrToStr(str);
}

std::wstring Service::GetMediaFields() const {
  return LR"(id
title {
  romaji(stylised: true)
  english(stylised: true)
  native(stylised: true)
  userPreferred
}
format
description
startDate { year month day }
endDate { year month day }
episodes
duration
countryOfOrigin
updatedAt
coverImage { large }
genres
synonyms
averageScore
popularity
studios { edges { node { name } } }
nextAiringEpisode { airingAt episode })";
}

std::wstring Service::GetMediaListFields() const {
  return LR"(id
status
score(format: POINT_100)
progress
repeat
notes
startedAt { year month day }
completedAt { year month day }
updatedAt)";
}

}  // namespace anilist
}  // namespace sync
