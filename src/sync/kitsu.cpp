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

#include "base/http.h"
#include "base/json.h"
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
  host_ = L"staging.kitsu.io/api";  // TODO: Change after launch

  id_ = kKitsu;
  canonical_name_ = L"kitsu";
  name_ = L"Kitsu";
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
  // token, but behave differently when it's provided.
  if (RequestNeedsAuthentication(request.type))
    http_request.header[L"Authorization"] = L"Bearer " + access_token_;

  switch (request.type) {
    BUILD_HTTP_REQUEST(kAddLibraryEntry, AddLibraryEntry);
    BUILD_HTTP_REQUEST(kAuthenticateUser, AuthenticateUser);
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
  http_request.method = L"POST";
  http_request.header[L"Content-Type"] = L"application/x-www-form-urlencoded";
  http_request.url.path = L"/oauth/token";

  // TODO: Register Taiga as a client and get a new ID
  http_request.data[L"client_id"] =
      L"dd031b32d2f56c990b1425efe6c42ad847e7fe3ab46bf1299f05ecd856bdb7dd";

  // Resource Owner Password Credentials Grant
  // https://tools.ietf.org/html/rfc6749#section-4.3
  http_request.data[L"grant_type"] = L"password";
  http_request.data[L"username"] = request.data[canonical_name_ + L"-username"];
  http_request.data[L"password"] = request.data[canonical_name_ + L"-password"];
}

void Service::GetLibraryEntries(Request& request, HttpRequest& http_request) {
  http_request.url.path =
      L"/users/" + request.data[canonical_name_ + L"-username"] + L"/library";

  if (request.data.count(L"status"))
    http_request.url.query[L"status"] =
        TranslateMyStatusTo(ToInt(request.data[L"status"]));
}

void Service::GetMetadataById(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/anime/" +
                          request.data[canonical_name_ + L"-id"];
}

void Service::SearchTitle(Request& request, HttpRequest& http_request) {
  http_request.url.path = L"/edge/anime";

  http_request.url.query[L"filter[text]"] = request.data[L"title"];

  // Kitsu's configuration sets JSONAPI's maximum_page_size to 20.
  // See: /config/initializers/jsonapi-resources.rb
  // Note that asking for more results will return an error: "Limit exceeds
  // maximum page size of 20." This means that our application could break if
  // the server's configuration is changed to reduce maximum_page_size.
  http_request.url.query[L"page[offset]"] = L"0";
  http_request.url.query[L"page[limit]"] = L"20";
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
  http_request.header[L"Content-Type"] = L"application/x-www-form-urlencoded";
  http_request.url.path =
      L"/libraries/" + request.data[canonical_name_ + L"-id"];

  if (request.data.count(L"status"))
    http_request.data[L"status"] =
        TranslateMyStatusTo(ToInt(request.data[L"status"]));
  if (request.data.count(L"score"))
    http_request.data[L"sane_rating_update"] =
        TranslateMyRatingTo(ToInt(request.data[L"score"]));
  if (request.data.count(L"enable_rewatching"))
    http_request.data[L"rewatching"] =
        request.data[L"enable_rewatching"] == L"0" ? L"false" : L"true";
  if (request.data.count(L"rewatched_times"))
    http_request.data[L"rewatched_times"] = request.data[L"rewatched_times"];
  if (request.data.count(L"episode"))
    http_request.data[L"episodes_watched"] = request.data[L"episode"];
}

////////////////////////////////////////////////////////////////////////////////
// Response handlers

void Service::AuthenticateUser(Response& response, HttpResponse& http_response) {
  Json::Value root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  access_token_ = StrToWstr(root["access_token"].asString());
}

void Service::GetLibraryEntries(Response& response, HttpResponse& http_response) {
  Json::Value root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  AnimeDatabase.ClearUserData();

  for (size_t i = 0; i < root.size(); i++)
    ParseLibraryObject(root[i]);
}

void Service::GetMetadataById(Response& response, HttpResponse& http_response) {
  Json::Value root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  ParseAnimeObject(root["data"]);
}

void Service::SearchTitle(Response& response, HttpResponse& http_response) {
  Json::Value root;

  if (!ParseResponseBody(http_response.body, response, root))
    return;

  for (const auto& value : root["data"]) {
    const auto anime_id = ParseAnimeObject(value);

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

  if (!ParseResponseBody(http_response.body, response, root))
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
      Json::Value root;
      Json::Reader reader;
      bool parsed = reader.parse(WstrToStr(http_response.body), root);
      response.data[L"error"] = name() + L" returned an error: ";
      if (parsed) {
        auto error = StrToWstr(root["error"].asString());
        response.data[L"error"] += error;
        if (response.type == kGetMetadataById) {
          if (InStr(error, L"Couldn't find Anime with 'id'=") > -1)
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

int Service::ParseAnimeObject(const Json::Value& value) const {
  const auto anime_id = ToInt(value["id"].asString());
  const auto& attributes = value["attributes"];

  anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_id), this->id());
  anime_item.SetLastModified(time(nullptr));  // current time

  anime_item.SetAgeRating(TranslateAgeRatingFrom(StrToWstr(
      attributes["ageRating"].asString())));
  anime_item.SetScore(TranslateSeriesRatingFrom(
      attributes["averageRating"].asFloat()));
  anime_item.SetTitle(StrToWstr(
      attributes["canonicalTitle"].asString()));
  anime_item.SetDateEnd(StrToWstr(
      attributes["endDate"].asString()));
  anime_item.SetEpisodeCount(
      attributes["episodeCount"].asInt());
  anime_item.SetEpisodeLength(
      attributes["episodeLength"].asInt());
  anime_item.SetImageUrl(StrToWstr(
      attributes["posterImage"]["small"].asString()));
  anime_item.SetType(TranslateSeriesTypeFrom(StrToWstr(
      attributes["showType"].asString())));
  anime_item.SetSlug(StrToWstr(
      attributes["slug"].asString()));
  anime_item.SetDateStart(StrToWstr(
      attributes["startDate"].asString()));
  anime_item.SetSynopsis(StrToWstr(
      attributes["synopsis"].asString()));

  for (const auto& title : attributes["abbreviatedTitles"]) {
    anime_item.InsertSynonym(StrToWstr(title.asString()));
  }
  if (attributes["titles"]["en"].isString()) {
    anime_item.SetEnglishTitle(StrToWstr(attributes["titles"]["en"].asString()));
  }
  if (attributes["titles"]["en_jp"].isString()) {
    anime_item.SetTitle(StrToWstr(attributes["titles"]["en_jp"].asString()));
  }
  if (attributes["titles"]["ja_jp"].isString()) {
    anime_item.InsertSynonym(StrToWstr(attributes["titles"]["ja_jp"].asString()));
  }

  return AnimeDatabase.UpdateItem(anime_item);
}

void Service::ParseLibraryObject(Json::Value& value) {
  auto& anime_value = value["anime"];
  auto& rating_value = value["rating"];

  ::anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(ToWstr(anime_value["id"].asInt()), this->id());
  anime_item.SetLastModified(time(nullptr));  // current time

  ParseAnimeObject(anime_value, anime_item);

  anime_item.AddtoUserList();
  anime_item.SetMyLastWatchedEpisode(value["episodes_watched"].asInt());
  anime_item.SetMyLastUpdated(TranslateMyLastUpdatedFrom(StrToWstr(value["updated_at"].asString())));
  anime_item.SetMyRewatchedTimes(value["rewatched_times"].asInt());
  anime_item.SetMyStatus(TranslateMyStatusFrom(StrToWstr(value["status"].asString())));
  anime_item.SetMyRewatching(value["rewatching"].asBool());
  anime_item.SetMyScore(TranslateMyRatingFrom(StrToWstr(rating_value["value"].asString()),
                                              StrToWstr(rating_value["type"].asString())));

  AnimeDatabase.UpdateItem(anime_item);
}

bool Service::ParseResponseBody(const std::wstring& body,
                                Response& response, Json::Value& root) {
  Json::Reader reader;

  if (reader.parse(WstrToStr(body), root))
    return true;

  switch (response.type) {
    case kGetLibraryEntries:
      response.data[L"error"] = L"Could not parse the list";
      break;
    case kGetMetadataById:
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

}  // namespace kitsu
}  // namespace sync
