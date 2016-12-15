/*
** Taiga
** Copyright (C) 2010-2016, Eren Okka
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

Service::Service() {
  host_ = L"kitsu.io/api";

  id_ = kKitsu;
  canonical_name_ = L"kitsu";
  name_ = L"Kitsu";

  user_id_ = 0;
}

////////////////////////////////////////////////////////////////////////////////

void Service::BuildRequest(Request& request, HttpRequest& http_request) {
  http_request.url.host = host_;

  if (Settings.GetBool(taiga::kSync_Service_Kitsu_UseHttps))
    http_request.url.protocol = base::http::kHttps;

  // Kitsu requires use of the JSON API media type: http://jsonapi.org/format/
  http_request.header[L"Accept"] = L"application/vnd.api+json";
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
}

void Service::GetLibraryEntries(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/users";

  http_request.url.query[L"filter[name]"] =
      request.data[canonical_name_ + L"-username"];

  http_request.url.query[L"include"] = L"libraryEntries.media";
}

void Service::GetMetadataById(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/anime/" +
                          request.data[canonical_name_ + L"-id"];
}

void Service::SearchTitle(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/anime";

  http_request.url.query[L"filter[text]"] = request.data[L"title"];

  // Kitsu's configuration sets JSONAPI's maximum_page_size to 20. Asking for
  // more results will return an error: "Limit exceeds maximum page size of 20."
  // This means that our application could break if the server's configuration
  // is changed to reduce maximum_page_size.
  http_request.url.query[L"page[offset]"] = L"0";
  http_request.url.query[L"page[limit]"] = L"20";
}

void Service::AddLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/library-entries";

  http_request.method = L"POST";
  http_request.header[L"Content-Type"] = L"application/vnd.api+json";

  http_request.body = BuildLibraryObject(request);
}

void Service::DeleteLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/library-entries/" +
                          request.data[canonical_name_ + L"-library-id"];

  http_request.method = L"DELETE";
  http_request.header[L"Content-Type"] = L"application/vnd.api+json";
}

void Service::UpdateLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/library-entries/" +
                          request.data[canonical_name_ + L"-library-id"];

  http_request.method = L"PATCH";
  http_request.header[L"Content-Type"] = L"application/vnd.api+json";

  http_request.body = BuildLibraryObject(request);
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

  user_id_ = ToInt(user["id"].get<std::string>());

  response.data[canonical_name_ + L"-username"] =
      StrToWstr(user["attributes"]["name"]);
}

void Service::GetLibraryEntries(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  AnimeDatabase.ClearUserData();

  for (const auto& value : root["included"]) {
    ParseObject(value);
  }
}

void Service::GetMetadataById(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  ParseAnimeObject(root["data"]);
}

void Service::SearchTitle(Response& response, HttpResponse& http_response) {
  Json root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  for (const auto& value : root["data"]) {
    const auto anime_id = ParseAnimeObject(value);

    // We return a list of IDs so that we can display the results afterwards
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

  ParseLibraryObject(root["data"]);
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
    case kSearchTitle:
      return !access_token_.empty();
  }

  return false;
}

bool Service::RequestSucceeded(Response& response,
                               const HttpResponse& http_response) {
  switch (http_response.code) {
    // OK
    case 200:
    case 201:
      return true;

    // Error
    default: {
      response.data[L"error"] = name() + L" returned an error: ";

      Json root;
      if (!JsonParseString(http_response.body, root)) {
        const auto error = StrToWstr(root["error"]);  // TODO: error_description
        response.data[L"error"] += error;
        if (response.type == kGetMetadataById) {
          if (InStr(error, L"Couldn't find Anime with 'id'=") > -1)  // TODO
            response.data[L"invalid_id"] = L"true";
        }

      } else {
        response.data[L"error"] += L"Unknown error (" +
            canonical_name() + L"|" +
            ToWstr(response.type) + L"|" +
            ToWstr(http_response.code) + L")";
      }

      return false;
    }
  }
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
        {"media", {
          {"data", {
            {"type", "anime"},
            {"id", ToStr(anime_id)},
          }}
        }},
        {"user", {
          {"data", {
            {"type", "users"},
            {"id", ToStr(user_id_)},
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

  if (request.data.count(L"episode"))
    attributes["progress"] = ToInt(request.data[L"episode"]);
  if (request.data.count(L"score"))
    attributes["rating"] = TranslateMyRatingTo(ToInt(request.data[L"score"]));
  if (request.data.count(L"rewatched_times"))
    attributes["reconsumeCount"] = ToInt(request.data[L"rewatched_times"]);
  if (request.data.count(L"enable_rewatching"))
    attributes["reconsuming"] = ToBool(request.data[L"enable_rewatching"]);
  if (request.data.count(L"status"))
    attributes["status"] = TranslateMyStatusTo(ToInt(request.data[L"status"]));

  return StrToWstr(json.dump());
}

void Service::ParseObject(const Json& json) const {
  enum class Type {
    Anime,
    LibraryEntries,
    Unknown,
  };

  const std::map<std::string, Type> table{
    {"anime", Type::Anime},
    {"libraryEntries", Type::LibraryEntries},
  };

  const auto find_type = [&](const std::string& str) {
    const auto it = table.find(str);
    return it != table.end() ? it->second : Type::Unknown;
  };

  switch (find_type(json["type"])) {
    case Type::Anime:
      ParseAnimeObject(json);
      break;
    case Type::LibraryEntries:
      ParseLibraryObject(json);
      break;
    default:
      LOG(LevelDebug, L"Invalid type: " + StrToWstr(json["type"]));
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
  anime_item.SetImageUrl(StrToWstr(JsonReadStr(attributes["posterImage"], "small")));
  anime_item.SetType(TranslateSeriesTypeFrom(JsonReadStr(attributes, "showType")));
  anime_item.SetSlug(StrToWstr(JsonReadStr(attributes, "slug")));
  anime_item.SetDateStart(StrToWstr(JsonReadStr(attributes, "startDate")));
  anime_item.SetSynopsis(StrToWstr(JsonReadStr(attributes, "synopsis")));

  for (const auto& title : attributes["abbreviatedTitles"]) {
    anime_item.InsertSynonym(StrToWstr(title));
  }
  if (attributes["titles"]["en"].is_string()) {
    anime_item.SetEnglishTitle(StrToWstr(attributes["titles"]["en"]));
  }
  if (attributes["titles"]["en_jp"].is_string()) {
    anime_item.SetTitle(StrToWstr(attributes["titles"]["en_jp"]));
  }
  if (attributes["titles"]["ja_jp"].is_string()) {
    anime_item.InsertSynonym(StrToWstr(attributes["titles"]["ja_jp"]));
  }

  return AnimeDatabase.UpdateItem(anime_item);
}

void Service::ParseLibraryObject(const Json& json) const {
  const auto& media = json["relationships"]["media"];
  const std::string media_type = media["data"]["type"];

  if (media_type != "anime") {
    LOG(LevelDebug, L"Invalid type: " + StrToWstr(media_type));
    return;  // ignore other types of media
  }

  const auto anime_id = ToInt(media["data"]["id"].get<std::string>());
  const auto library_id = ToInt(json["id"].get<std::string>());
  const auto& attributes = json["attributes"];

  anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_id), this->id());
  anime_item.AddtoUserList();

  anime_item.SetMyId(ToWstr(library_id));
  anime_item.SetMyLastWatchedEpisode(JsonReadInt(attributes, "progress"));
  anime_item.SetMyScore(TranslateMyRatingFrom(JsonReadStr(attributes, "rating")));
  anime_item.SetMyRewatchedTimes(JsonReadInt(attributes, "reconsumeCount"));
  anime_item.SetMyRewatching(JsonReadBool(attributes, "reconsuming"));
  anime_item.SetMyStatus(TranslateMyStatusFrom(JsonReadStr(attributes, "status")));
  anime_item.SetMyLastUpdated(TranslateMyLastUpdatedFrom(JsonReadStr(attributes, "updatedAt")));

  AnimeDatabase.UpdateItem(anime_item);
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
      response.data[L"error"] = L"Could not parse media object";
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
