/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include <map>

#include "base/http.h"
#include "base/log.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_item.h"
#include "sync/kitsu.h"
#include "sync/kitsu_types.h"
#include "sync/kitsu_util.h"
#include "taiga/settings.h"

namespace sync {
namespace kitsu {

// Kitsu requires use of the JSON API media type: http://jsonapi.org/format/
constexpr auto kJsonApiMediaType = L"application/vnd.api+json";

// Kitsu's configuration sets JSON API's maximum_page_size to 20. Asking for
// more results will return an error: "Limit exceeds maximum page size of 20."
// This means that our application could break if the server's configuration is
// changed to reduce maximum_page_size.
constexpr auto kJsonApiMaximumPageSize = 20;

Service::Service() {
  host_ = L"kitsu.io/api";

  id_ = kKitsu;
  canonical_name_ = L"kitsu";
  name_ = L"Kitsu";
}

////////////////////////////////////////////////////////////////////////////////

void Service::BuildRequest(Request& request, HttpRequest& http_request) {
  http_request.url.host = host_;

  if (Settings.GetBool(taiga::kSync_Service_Kitsu_UseHttps))
    http_request.url.protocol = base::http::kHttps;

  http_request.header[L"Accept"] = kJsonApiMediaType;
  http_request.header[L"Accept-Charset"] = L"utf-8";
  http_request.header[L"Accept-Encoding"] = L"gzip";

  // kAuthenticateUser method returns an access token, which is to be used on
  // all methods that require authentication. Some methods don't require the
  // token, but behave differently (e.g. private library entries are included)
  // when it's provided.
  if (RequestNeedsAuthentication(request.type))
    http_request.header[L"Authorization"] = L"Bearer " + access_token_;

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
  http_request.url.path = L"/oauth/token";

  // We will transmit the credentials in the request body rather than as query
  // parameters to avoid accidental logging.
  http_request.method = L"POST";
  http_request.header[L"Content-Type"] = L"application/x-www-form-urlencoded";

  // TODO: Register Taiga as a client and get a new ID
  http_request.data[L"client_id"] =
      L"dd031b32d2f56c990b1425efe6c42ad847e7fe3ab46bf1299f05ecd856bdb7dd";

  // Resource Owner Password Credentials Grant
  // https://tools.ietf.org/html/rfc6749#section-4.3
  http_request.data[L"grant_type"] = L"password";
  http_request.data[L"username"] = request.data[canonical_name_ + L"-username"];
  http_request.data[L"password"] = request.data[canonical_name_ + L"-password"];
}

void Service::GetUser(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/users";

  http_request.url.query[L"filter[name]"] =
      request.data[canonical_name_ + L"-username"];

  UseSparseFieldsetsForUser(http_request);
}

void Service::GetLibraryEntries(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/library-entries";

  http_request.url.query[L"filter[user_id]"] = user_.id;
  http_request.url.query[L"filter[kind]"] = L"anime";

  // We don't need to download the entire library; we just need to know about
  // the entries that have changed since the last download. `filter[since]` is
  // quite useful in this manner, but note that the following scenario results
  // in an undesirable behavior:
  //
  // 1. Get library entries via client
  // 2. Go to website and delete entry from library
  // 3. Get library entries via client, using `filter[since]={...}`
  // 4. Client doesn't know entry was deleted
  //
  // Our "solution" to this is to allow Taiga to download the entire library
  // after each restart (i.e. last_synchronized_ is not saved on exit).
  if (last_synchronized_ &&
      Settings.GetBool(taiga::kSync_Service_Kitsu_PartialLibrary)) {
    const auto date = GetDate(last_synchronized_ - (60 * 60 * 24));  // 1 day before, to be safe
    http_request.url.query[L"filter[since]"] = std::wstring(date);
  }

  http_request.url.query[L"include"] = L"anime";

  // Kitsu's configuration sets JSONAPI's default_page_size to 10. The only way
  // to retrieve the entire library in a single request is to set the limit to a
  // sufficiently big number.
  http_request.url.query[L"page[offset]"] = L"0";
  http_request.url.query[L"page[limit]"] = L"100000";

  UseSparseFieldsetsForAnime(http_request);
  UseSparseFieldsetsForLibraryEntries(http_request);
}

void Service::GetMetadataById(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/anime/" +
                          request.data[canonical_name_ + L"-id"];

  http_request.url.query[L"include"] =
      L"genres,"
      L"animeProductions,"
      L"animeProductions.producer";

  UseSparseFieldsetsForAnime(http_request);
}

void Service::GetSeason(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/anime";

  http_request.url.query[L"filter[season]"] = request.data[L"season"];
  http_request.url.query[L"filter[season_year]"] = request.data[L"year"];

  http_request.url.query[L"page[offset]"] = request.data[L"page_offset"];
  http_request.url.query[L"page[limit]"] = ToWstr(kJsonApiMaximumPageSize);

  // We don't actually need the results to be sorted. But without this
  // parameter, we get inconsistent ordering and duplicate objects.
  http_request.url.query[L"sort"] = L"-user_count";

  UseSparseFieldsetsForAnime(http_request);
}

void Service::SearchTitle(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/anime";

  http_request.url.query[L"filter[text]"] = request.data[L"title"];

  http_request.url.query[L"page[offset]"] = L"0";
  http_request.url.query[L"page[limit]"] = ToWstr(kJsonApiMaximumPageSize);

  UseSparseFieldsetsForAnime(http_request);
}

void Service::AddLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/library-entries";

  http_request.method = L"POST";
  http_request.header[L"Content-Type"] = kJsonApiMediaType;

  http_request.url.query[L"include"] =
      L"anime,"
      L"anime.genres,"
      L"anime.animeProductions,"
      L"anime.animeProductions.producer";

  http_request.body = BuildLibraryObject(request);

  UseSparseFieldsetsForAnime(http_request);
  UseSparseFieldsetsForLibraryEntries(http_request);
}

void Service::DeleteLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/library-entries/" +
                          request.data[canonical_name_ + L"-library-id"];

  http_request.method = L"DELETE";
  http_request.header[L"Content-Type"] = kJsonApiMediaType;
}

void Service::UpdateLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/library-entries/" +
                          request.data[canonical_name_ + L"-library-id"];

  http_request.method = L"PATCH";
  http_request.header[L"Content-Type"] = kJsonApiMediaType;

  http_request.url.query[L"include"] =
      L"anime,"
      L"anime.genres,"
      L"anime.animeProductions,"
      L"anime.animeProductions.producer";

  http_request.body = BuildLibraryObject(request);

  UseSparseFieldsetsForAnime(http_request);
  UseSparseFieldsetsForLibraryEntries(http_request);
}

////////////////////////////////////////////////////////////////////////////////
// Response handlers

void Service::AuthenticateUser(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  access_token_ = StrToWstr(root["access_token"]);
}

void Service::GetUser(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  const auto& user = root["data"].front();

  user_.id = StrToWstr(JsonReadStr(user, "id"));
  user_.username = StrToWstr(JsonReadStr(user["attributes"], "name"));
}

void Service::GetLibraryEntries(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  if (!last_synchronized_ ||
      !Settings.GetBool(taiga::kSync_Service_Kitsu_PartialLibrary)) {
    AnimeDatabase.ClearUserData();
  }
  last_synchronized_ = time(nullptr);  // current time

  for (const auto& value : root["data"]) {
    ParseLibraryObject(value);
  }

  for (const auto& value : root["included"]) {
    ParseObject(value);
  }
}

void Service::GetMetadataById(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  const auto anime_id = ParseAnimeObject(root["data"]);

  ParseGenres(root["included"], anime_id);
  ParseProducers(root["included"], anime_id);
}

void Service::GetSeason(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  for (const auto& value : root["data"]) {
    const auto anime_id = ParseAnimeObject(value);
    AppendString(response.data[L"ids"], ToWstr(anime_id), L",");
  }

  const auto next = JsonReadStr(root["links"], "next");
  if (!next.empty()) {
    Url url = StrToWstr(next);
    response.data[L"page_offset"] = url.query[L"page[offset]"];
  }
}

void Service::SearchTitle(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  for (const auto& value : root["data"]) {
    const auto anime_id = ParseAnimeObject(value);
    AppendString(response.data[L"ids"], ToWstr(anime_id), L",");
  }
}

void Service::AddLibraryEntry(Response& response, HttpResponse& http_response) {
  UpdateLibraryEntry(response, http_response);
}

void Service::DeleteLibraryEntry(Response& response, HttpResponse& http_response) {
  // Returns "204 No Content" status and empty response body.
}

void Service::UpdateLibraryEntry(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  const auto anime_id = ParseLibraryObject(root["data"]);

  for (const auto& value : root["included"]) {
    ParseObject(value);
  }

  ParseGenres(root["included"], anime_id);
  ParseProducers(root["included"], anime_id);
}

////////////////////////////////////////////////////////////////////////////////

bool Service::RequestNeedsAuthentication(RequestType request_type) const {
  switch (request_type) {
    case kAddLibraryEntry:
    case kDeleteLibraryEntry:
    case kUpdateLibraryEntry:
      return true;
    case kGetLibraryEntries:
    case kGetMetadataById:
    case kGetSeason:
    case kSearchTitle:
      return !access_token_.empty();
  }

  return false;
}

bool Service::RequestSucceeded(Response& response,
                               const HttpResponse& http_response) {
  const auto status_category = http_response.GetStatusCategory();

  // 200 OK
  // 201 Created
  // 202 Accepted
  // 204 No Content
  if (status_category == 200)
    return true;

  // If we try to add a library entry that is already there, Kitsu returns a
  // "422 Unprocessable Entity" response with "animeId - has already been taken"
  // error message. Here we ignore this error and assume that our request
  // succeeded.
  if (response.type == kAddLibraryEntry && status_category == 400)
    return false;

  // Handle invalid anime IDs
  if (response.type == kGetMetadataById && http_response.code == 404)
    response.data[L"invalid_id"] = L"true";

  // Error
  Json root;
  std::wstring error_description;

  if (JsonParseString(http_response.body, root)) {
    if (root.count("error_description")) {
      error_description = StrToWstr(root["error_description"]);
    } else if (root.count("errors")) {
      const auto& errors = root["errors"];
      if (errors.is_array() && !errors.empty()) {
        const auto& error = errors.front();
        error_description = StrToWstr("\"" +
            error["title"].get<std::string>() + ": " +
            error["detail"].get<std::string>() + "\"");
      }
    }
  }

  if (error_description.empty()) {
    error_description = L"Unknown error (" +
        canonical_name() + L"|" +
        ToWstr(response.type) + L"|" +
        ToWstr(http_response.code) + L")";
  }

  response.data[L"error"] = name() + L" returned an error: " +
                            error_description;
  return false;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring Service::BuildLibraryObject(Request& request) const {
  const auto anime_id = ToInt(request.data[canonical_name_ + L"-id"]);
  const auto library_id = ToInt(request.data[canonical_name_ + L"-library-id"]);

  Json json = {
    {"data", {
      {"type", "libraryEntries"},
      {"attributes", {}},
      {"relationships", {
        {"anime", {
          {"data", {
            {"type", "anime"},
            {"id", ToStr(anime_id)},
          }}
        }},
        {"user", {
          {"data", {
            {"type", "users"},
            {"id", WstrToStr(user_.id)},
          }}
        }}
      }}
    }}
  };

  // According to the JSON API specification, "The `id` member is not required
  // when the resource object originates at the client and represents a new
  // resource to be created on the server."
  if (library_id)
    json["data"]["id"] = ToStr(library_id);

  auto& attributes = json["data"]["attributes"];

  if (request.data.count(L"notes"))
    attributes["notes"] = WstrToStr(request.data[L"notes"]);
  if (request.data.count(L"episode"))
    attributes["progress"] = ToInt(request.data[L"episode"]);
  if (request.data.count(L"score")) {
    const auto score = ToInt(request.data[L"score"]);
    if (score > 0) {
      attributes["rating"] = TranslateMyRatingTo(score);
    } else {
      // Kitsu requires the rating value to be greater than zero. To remove the
      // rating, we need to send `null` rather than `0`.
      attributes["rating"] = nullptr;
    }
  }
  if (request.data.count(L"rewatched_times"))
    attributes["reconsumeCount"] = ToInt(request.data[L"rewatched_times"]);
  if (request.data.count(L"enable_rewatching"))
    attributes["reconsuming"] = ToBool(request.data[L"enable_rewatching"]);
  if (request.data.count(L"status"))
    attributes["status"] = TranslateMyStatusTo(ToInt(request.data[L"status"]));

  return StrToWstr(json.dump());
}

void Service::UseSparseFieldsetsForAnime(HttpRequest& http_request) const {
  // Requesting a restricted set of fields decreases response and download times
  // in certain cases.
  http_request.url.query[L"fields[anime]"] =
      // attributes
      L"abbreviatedTitles,"
      L"ageRating,"
      L"averageRating,"
      L"canonicalTitle,"
      L"endDate,"
      L"episodeCount,"
      L"episodeLength,"
      L"popularityRank,"
      L"posterImage,"
      L"slug,"
      L"startDate,"
      L"subtype,"
      L"synopsis,"
      L"titles,"
      // relationships
      L"animeProductions,"
      L"genres";
  http_request.url.query[L"fields[animeProductions]"] = L"producer";
  http_request.url.query[L"fields[genres]"] = L"name";
  http_request.url.query[L"fields[producers]"] = L"name";
}

void Service::UseSparseFieldsetsForLibraryEntries(HttpRequest& http_request) const {
  http_request.url.query[L"fields[libraryEntries]"] =
      // attributes
      L"notes,"
      L"progress,"
      L"rating,"
      L"reconsumeCount,"
      L"reconsuming,"
      L"status,"
      L"updatedAt,"
      // relationships
      L"anime";
}

void Service::UseSparseFieldsetsForUser(HttpRequest& http_request) const {
  // We don't need any user information other than the ID and the name.
  http_request.url.query[L"fields[users]"] = L"name";
}

void Service::ParseObject(const Json& json) const {
  enum class Type {
    Anime,
    Genres,
    LibraryEntries,
    Producers,
    Unknown,
  };

  const std::map<std::string, Type> table{
    {"anime", Type::Anime},
    {"genres", Type::Genres},
    {"libraryEntries", Type::LibraryEntries},
    {"producers", Type::Producers},
  };

  const auto find_type = [&](const std::string& str) {
    const auto it = table.find(str);
    return it != table.end() ? it->second : Type::Unknown;
  };

  switch (find_type(json["type"])) {
    case Type::Anime:
      ParseAnimeObject(json);
      break;
    case Type::Genres:
      break;
    case Type::LibraryEntries:
      ParseLibraryObject(json);
      break;
    case Type::Producers:
      break;
    default:
      LOGD(L"Invalid type: " + StrToWstr(json["type"]));
      break;
  }
}

int Service::ParseAnimeObject(const Json& json) const {
  const auto anime_id = ToInt(json["id"].get<std::string>());
  const auto& attributes = json["attributes"];

  anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_id), this->id());
  anime_item.SetLastModified(time(nullptr));  // current time

  anime_item.SetAgeRating(TranslateAgeRatingFrom(JsonReadStr(attributes, "ageRating")));
  anime_item.SetScore(TranslateSeriesRatingFrom(JsonReadDouble(attributes, "averageRating")));
  anime_item.SetTitle(StrToWstr(JsonReadStr(attributes, "canonicalTitle")));
  anime_item.SetDateEnd(StrToWstr(JsonReadStr(attributes, "endDate")));
  anime_item.SetEpisodeCount(JsonReadInt(attributes, "episodeCount"));
  anime_item.SetEpisodeLength(JsonReadInt(attributes, "episodeLength"));
  anime_item.SetPopularity(JsonReadInt(attributes, "popularityRank"));
  anime_item.SetImageUrl(StrToWstr(JsonReadStr(attributes["posterImage"], "small")));
  anime_item.SetSlug(StrToWstr(JsonReadStr(attributes, "slug")));
  anime_item.SetDateStart(StrToWstr(JsonReadStr(attributes, "startDate")));
  anime_item.SetType(TranslateSeriesTypeFrom(JsonReadStr(attributes, "subtype")));
  anime_item.SetSynopsis(DecodeSynopsis(JsonReadStr(attributes, "synopsis")));

  for (const auto& title : attributes["abbreviatedTitles"]) {
    anime_item.InsertSynonym(StrToWstr(title));
  }

  enum class TitleLanguage {
    en,     // English
    en_jp,  // Romaji
    ja_jp,  // Japanese
  };
  static const std::map<std::string, TitleLanguage> title_languages{
    {"en", TitleLanguage::en},
    {"en_jp", TitleLanguage::en_jp},
    {"ja_jp", TitleLanguage::ja_jp},
  };
  auto& titles = attributes["titles"];
  for (auto it = titles.cbegin(); it != titles.cend(); ++it) {
    auto language = title_languages.find(it.key());
    if (language == title_languages.end() || !it->is_string())
      continue;
    switch (language->second) {
      case TitleLanguage::en:
        anime_item.SetEnglishTitle(StrToWstr(it.value()));
        break;
      case TitleLanguage::en_jp:
        anime_item.SetTitle(StrToWstr(it.value()));
        break;
      case TitleLanguage::ja_jp:
        anime_item.SetJapaneseTitle(StrToWstr(it.value()));
        break;
    }
  }

  return AnimeDatabase.UpdateItem(anime_item);
}

void Service::ParseGenres(const Json& json, const int anime_id) const {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return;

  std::vector<std::wstring> genres;
  
  // Here we assume that all listed genres are related to this anime. In order
  // to parse an array of anime objects with genres, we would need to extract
  // genre IDs from `relationships/genres/data` for each anime.
  for (const auto& value : json) {
    if (value["type"] == "genres") {
      genres.push_back(StrToWstr(value["attributes"]["name"]));
    }
  }

  anime_item->SetGenres(genres);
}

void Service::ParseProducers(const Json& json, const int anime_id) const {
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item)
    return;

  std::vector<std::wstring> producers;

  // Here we assume that all listed producers are related to this anime.
  for (const auto& value : json) {
    if (value["type"] == "producers") {
      producers.push_back(StrToWstr(value["attributes"]["name"]));
    }
  }

  anime_item->SetProducers(producers);
}

int Service::ParseLibraryObject(const Json& json) const {
  const auto& media = json["relationships"]["anime"];
  const auto& attributes = json["attributes"];

  const auto anime_id = ToInt(media["data"]["id"].get<std::string>());
  const auto library_id = ToInt(json["id"].get<std::string>());

  anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_id), this->id());
  anime_item.AddtoUserList();

  anime_item.SetMyId(ToWstr(library_id));
  anime_item.SetMyNotes(StrToWstr(JsonReadStr(attributes, "notes")));
  anime_item.SetMyLastWatchedEpisode(JsonReadInt(attributes, "progress"));
  anime_item.SetMyScore(TranslateMyRatingFrom(JsonReadStr(attributes, "rating")));
  anime_item.SetMyRewatchedTimes(JsonReadInt(attributes, "reconsumeCount"));
  anime_item.SetMyRewatching(JsonReadBool(attributes, "reconsuming"));
  anime_item.SetMyStatus(TranslateMyStatusFrom(JsonReadStr(attributes, "status")));
  anime_item.SetMyLastUpdated(TranslateMyLastUpdatedFrom(JsonReadStr(attributes, "updatedAt")));

  return AnimeDatabase.UpdateItem(anime_item);
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

}  // namespace kitsu
}  // namespace sync
