/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include "base/http.h"
#include "base/json.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_item.h"
#include "sync/hummingbird.h"
#include "sync/hummingbird_types.h"
#include "sync/hummingbird_util.h"

namespace sync {
namespace hummingbird {

Service::Service() {
  host_ = L"hummingbird.me/api/v1";

  id_ = kHummingbird;
  canonical_name_ = L"hummingbird";
  name_ = L"Hummingbird";
}

////////////////////////////////////////////////////////////////////////////////

void Service::BuildRequest(Request& request, HttpRequest& http_request) {
  http_request.url.host = host_;

  // Hummingbird is supposed to return a JSON response for each and every
  // request, so that's what we expect from it
  http_request.header[L"Accept"] = L"application/json";
  http_request.header[L"Accept-Charset"] = L"utf-8";

  // kAuthenticateUser method returns the user's authentication token, which
  // is to be used on all methods that require authentication.
  if (RequestNeedsAuthentication(request.type))
    http_request.url.query[L"auth_token"] = auth_token_;

  switch (request.type) {
    case kGetLibraryEntries:
      // TODO: Make sure username is available
      http_request.header[L"Accept-Encoding"] = L"gzip";
      break;
  }

  switch (request.type) {
    BUILD_HTTP_REQUEST(kAddLibraryEntry, AddLibraryEntry);
    BUILD_HTTP_REQUEST(kAuthenticateUser, AuthenticateUser);
    BUILD_HTTP_REQUEST(kDeleteLibraryEntry, DeleteLibraryEntry);
    BUILD_HTTP_REQUEST(kGetLibraryEntries, GetLibraryEntries);
    BUILD_HTTP_REQUEST(kGetMetadataById, GetMetadataById);
    BUILD_HTTP_REQUEST(kGetMetadataByIdV2, GetMetadataByIdV2);
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
      HANDLE_HTTP_RESPONSE(kGetMetadataByIdV2, GetMetadataByIdV2);
      HANDLE_HTTP_RESPONSE(kSearchTitle, SearchTitle);
      HANDLE_HTTP_RESPONSE(kUpdateLibraryEntry, UpdateLibraryEntry);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Request builders

void Service::AuthenticateUser(Request& request, HttpRequest& http_request) {
  http_request.method = L"POST";
  http_request.url.path = L"/users/authenticate";

  // TODO: Make sure username and password are available
  http_request.url.query[L"username"] =
      request.data[canonical_name_ + L"-username"];
  http_request.url.query[L"password"] =
      request.data[canonical_name_ + L"-password"];
}

void Service::GetLibraryEntries(Request& request, HttpRequest& http_request) {
  http_request.url.path =
      L"/users/" + request.data[canonical_name_ + L"-username"] + L"/library";

  if (request.data.count(L"status"))
    http_request.url.query[L"status"] =
        TranslateMyStatusTo(ToInt(request.data[L"status"]));
}

void Service::GetMetadataById(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/anime/" + request.data[canonical_name_ + L"-id"];
}

void Service::GetMetadataByIdV2(Request& request, HttpRequest& http_request) {
  // Hummingbird APIv2 is not yet ready, but this method can be called for
  // additional data that APIv1 doesn't provide.
  http_request.url.host = L"hummingbird.me/api/v2";
  http_request.url.path = L"/anime/" + request.data[canonical_name_ + L"-id"];
}

void Service::SearchTitle(Request& request, HttpRequest& http_request) {
  // This method is not documented, but otherwise works fine. Note that it will
  // return only 5 results at a time.
  http_request.url.path = L"/search/anime";
  http_request.url.query[L"query"] = request.data[L"title"];
}

void Service::AddLibraryEntry(Request& request, HttpRequest& http_request) {
  UpdateLibraryEntry(request, http_request);
}

void Service::DeleteLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.method = L"POST";
  http_request.url.path =
      L"/libraries/" + request.data[canonical_name_ + L"-id"] + L"/remove";
}

void Service::UpdateLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.method = L"POST";
  http_request.url.path =
      L"/libraries/" + request.data[canonical_name_ + L"-id"];

  // When this undocumented parameter is included, Hummingbird will return a
  // "mal_id" value that identifies the corresponding entry in MyAnimeList, if
  // available.
  http_request.url.query[L"include_mal_id"] = L"true";

  if (request.data.count(L"status"))
    http_request.url.query[L"status"] =
        TranslateMyStatusTo(ToInt(request.data[L"status"]));
  if (request.data.count(L"score"))
    http_request.url.query[L"rating"] =
        TranslateMyRatingTo(ToInt(request.data[L"score"]));
  if (request.data.count(L"episode"))
    http_request.url.query[L"episodes_watched"] = request.data[L"episode"];
}

////////////////////////////////////////////////////////////////////////////////
// Response handlers

void Service::AuthenticateUser(Response& response, HttpResponse& http_response) {
  auth_token_ = http_response.body;
  Trim(auth_token_, L"\"'");
}

void Service::GetLibraryEntries(Response& response, HttpResponse& http_response) {
  Json::Value root;

  if (!ParseResponseBody(response, http_response, root))
    return;

  for (size_t i = 0; i < root.size(); i++)
    ParseLibraryObject(root[i]);
}

void Service::GetMetadataById(Response& response, HttpResponse& http_response) {
  Json::Value root;

  if (!ParseResponseBody(response, http_response, root))
    return;

  ::anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(root["id"].asInt()), this->id());
  anime_item.SetLastModified(time(nullptr));  // current time

  ParseAnimeObject(root, anime_item);

  AnimeDatabase.UpdateItem(anime_item);
}

void Service::GetMetadataByIdV2(Response& response, HttpResponse& http_response) {
  Json::Value root;

  if (!ParseResponseBody(response, http_response, root))
    return;

  ::anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(root["id"].asInt()), this->id());
  anime_item.SetLastModified(time(nullptr));  // current time

  ParseAnimeObjectV2(root, anime_item);

  AnimeDatabase.UpdateItem(anime_item);
}

void Service::SearchTitle(Response& response, HttpResponse& http_response) {
  Json::Value root;

  if (!ParseResponseBody(response, http_response, root))
    return;

  for (size_t i = 0; i < root.size(); i++) {
    ::anime::Item anime_item;
    anime_item.SetSource(this->id());
    anime_item.SetId(ToWstr(root[i]["id"].asInt()), this->id());
    anime_item.SetLastModified(time(nullptr));  // current time

    ParseAnimeObject(root[i], anime_item);

    int anime_id = AnimeDatabase.UpdateItem(anime_item);

    // We return a list of IDs so that we can display the results afterwards
    AppendString(response.data[L"ids"], ToWstr(anime_id), L",");
  }
}

void Service::AddLibraryEntry(Response& response, HttpResponse& http_response) {
  // Returns "false"
}

void Service::DeleteLibraryEntry(Response& response, HttpResponse& http_response) {
  // Returns "true"
}

void Service::UpdateLibraryEntry(Response& response, HttpResponse& http_response) {
  Json::Value root;

  if (!ParseResponseBody(response, http_response, root))
    return;

  ParseLibraryObject(root);
}

////////////////////////////////////////////////////////////////////////////////

bool Service::RequestNeedsAuthentication(RequestType request_type) const {
  switch (request_type) {
    case kAddLibraryEntry:
    case kDeleteLibraryEntry:
    case kUpdateLibraryEntry:
      return true;
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
      Json::Value root;
      Json::Reader reader;
      bool parsed = reader.parse(WstrToStr(http_response.body), root);
      response.data[L"error"] = name() + L" returned an error: ";
      if (parsed) {
        response.data[L"error"] += StrToWstr(root["error"].asString());
      } else {
        response.data[L"error"] += L"Unknown error (" +
            ToWstr(static_cast<int>(http_response.code)) + L")";
      }
      return false;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void Service::ParseAnimeObject(Json::Value& value, anime::Item& anime_item) {
  anime_item.SetSlug(StrToWstr(value["slug"].asString()));
  anime_item.SetAiringStatus(TranslateSeriesStatusFrom(StrToWstr(value["status"].asString())));
  anime_item.SetTitle(StrToWstr(value["title"].asString()));
  anime_item.SetSynonyms(StrToWstr(value["alternate_title"].asString()));
  anime_item.SetEpisodeCount(value["episode_count"].asInt());
  anime_item.SetImageUrl(StrToWstr(value["cover_image"].asString()));
  anime_item.SetSynopsis(StrToWstr(value["synopsis"].asString()));
  anime_item.SetType(TranslateSeriesTypeFrom(StrToWstr(value["show_type"].asString())));

  std::vector<std::wstring> genres;
  auto& genres_value = value["genres"];
  for (size_t i = 0; i < genres_value.size(); i++)
    genres.push_back(StrToWstr(genres_value[i]["name"].asString()));

  if (!genres.empty())
    anime_item.SetGenres(genres);
}

void Service::ParseAnimeObjectV2(Json::Value& value, anime::Item& anime_item) {
  anime_item.SetSlug(StrToWstr(value["slug"].asString()));
  anime_item.SetTitle(StrToWstr(value["canonical_title"].asString()));
  anime_item.SetEnglishTitle(StrToWstr(value["english_title"].asString()));
  anime_item.SetSynonyms(StrToWstr(value["romaji_title"].asString()));
  anime_item.SetSynopsis(StrToWstr(value["synopsis"].asString()));
  anime_item.SetImageUrl(StrToWstr(value["poster_image"].asString()));
  anime_item.SetType(TranslateSeriesTypeFrom(StrToWstr(value["type"].asString())));
  anime_item.SetDateStart(StrToWstr(value["started_airing"].asString()));
  anime_item.SetDateEnd(StrToWstr(value["finished_airing"].asString()));
  anime_item.SetScore(TranslateSeriesRatingFrom(value["community_rating"].asFloat()));
  anime_item.SetAgeRating(TranslateAgeRatingFrom(StrToWstr(value["age_rating"].asString())));
  anime_item.SetEpisodeCount(value["episode_count"].asInt());
  anime_item.SetEpisodeLength(value["episode_length"].asInt());

  std::vector<std::wstring> genres;
  auto& genres_value = value["genres"];
  for (size_t i = 0; i < genres_value.size(); i++)
    genres.push_back(StrToWstr(genres_value[i].asString()));

  if (!genres.empty())
    anime_item.SetGenres(genres);
}

void Service::ParseLibraryObject(Json::Value& value) {
  auto& anime_value = value["anime"];
  auto& rating_value = value["rating"];

  ::anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_value["id"].asInt()), this->id());
  anime_item.SetLastModified(time(nullptr));  // current time

  int mal_id = value["mal_id"].asInt();
  if (mal_id > 0)
    anime_item.SetId(ToWstr(mal_id), sync::kMyAnimeList);

  ParseAnimeObject(anime_value, anime_item);

  anime_item.AddtoUserList();
  anime_item.SetMyLastWatchedEpisode(value["episodes_watched"].asInt());
  anime_item.SetMyStatus(TranslateMyStatusFrom(StrToWstr(value["status"].asString())));
  anime_item.SetMyRewatching(value["rewatching"].asBool());
  anime_item.SetMyScore(TranslateMyRatingFrom(StrToWstr(rating_value["value"].asString()),
                                              StrToWstr(rating_value["type"].asString())));

  AnimeDatabase.UpdateItem(anime_item);
}

bool Service::ParseResponseBody(Response& response, HttpResponse& http_response,
                                Json::Value& root) {
  Json::Reader reader;

  if (reader.parse(WstrToStr(http_response.body), root))
    return true;

  switch (response.type) {
    case kGetLibraryEntries:
      response.data[L"error"] = L"Could not parse the list";
      break;
    case kGetMetadataById:
    case kGetMetadataByIdV2:
      response.data[L"error"] = L"Could not parse the anime object";
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

}  // namespace hummingbird
}  // namespace sync