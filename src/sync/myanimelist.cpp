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

#include <set>

#include "base/base64.h"
#include "base/html.h"
#include "base/http.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime_db.h"
#include "library/anime_item.h"
#include "library/anime_util.h"
#include "sync/myanimelist.h"
#include "sync/myanimelist_util.h"
#include "taiga/settings.h"

namespace sync {
namespace myanimelist {

Service::Service() {
  host_ = L"myanimelist.net";

  id_ = kMyAnimeList;
  canonical_name_ = L"myanimelist";
  name_ = L"MyAnimeList";
}

////////////////////////////////////////////////////////////////////////////////

void Service::BuildRequest(Request& request, HttpRequest& http_request) {
  http_request.url.host = host_;
  http_request.url.protocol = base::http::Protocol::Https;

  // This doesn't quite help; MAL returns whatever it pleases
  http_request.header[L"Accept"] = L"text/xml, text/*";
  http_request.header[L"Accept-Charset"] = L"utf-8";
  http_request.header[L"Accept-Encoding"] = L"gzip";

  // Since October 2013, third-party applications need to identify themselves
  // with a unique user-agent string that is whitelisted by MAL. Using a generic
  // value (e.g. "Mozilla/5.0") or an arbitrary one (e.g. "Taiga/1.0") will
  // result in invalid text/html responses, courtesy of Incapsula.
  // To get your own whitelisted user-agent string, follow the registration link
  // at the official MAL API club page. If, for any reason, you'd like to use
  // Taiga's instead, I will appreciate it if you ask beforehand.
  http_request.header[L"User-Agent"] =
      L"api-taiga-32864c09ef538453b4d8110734ee355b";

  if (RequestNeedsAuthentication(request.type)) {
    // TODO: Make sure username and password are available
    http_request.header[L"Authorization"] = L"Basic " +
        Base64Encode(request.data[canonical_name_ + L"-username"] + L":" +
                     request.data[canonical_name_ + L"-password"]);
  }

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
  http_request.url.path = L"/api/account/verify_credentials.xml";
}

void Service::GetLibraryEntries(Request& request, HttpRequest& http_request) {
  // malappinfo.php is an undocumented feature of MAL. While it's not a part
  // of their official API, it's the easiest way to get user lists.
  http_request.url.path = L"/malappinfo.php";
  http_request.url.query[L"u"] = request.data[canonical_name_ + L"-username"];
  // Changing the status parameter to some other value such as "1" or "watching"
  // doesn't seem to make any difference.
  http_request.url.query[L"status"] = L"all";
}

void Service::GetMetadataById(Request& request, HttpRequest& http_request) {
  // As MAL API doesn't have a method to get information by ID, we're using an
  // undocumented call that is normally used to display information bubbles
  // when hovering over anime/manga titles at the website. The downside is,
  // it doesn't provide all the information we need.
  http_request.url.path = L"/includes/ajax.inc.php";
  http_request.url.query[L"t"] = L"64";
  http_request.url.query[L"id"] = request.data[canonical_name_ + L"-id"];
}

void Service::SearchTitle(Request& request, HttpRequest& http_request) {
  // MAL's search method is far from perfect. Missing a punctuation mark
  // (e.g. "A B" instead of "A: B") or searching for a title that is too short
  // (e.g. "C", "K") can return irrelevant or no results.
  http_request.url.path = L"/api/anime/search.xml";
  // TODO: We might have to do some encoding on the title here
  http_request.url.query[L"q"] = request.data[L"title"];
}

void Service::AddLibraryEntry(Request& request, HttpRequest& http_request) {
  request.data[L"action"] = L"add";
  UpdateLibraryEntry(request, http_request);
}

void Service::DeleteLibraryEntry(Request& request, HttpRequest& http_request) {
  request.data[L"action"] = L"delete";
  UpdateLibraryEntry(request, http_request);
}

void Service::UpdateLibraryEntry(Request& request, HttpRequest& http_request) {
  http_request.method = L"POST";
  http_request.header[L"Content-Type"] = L"application/x-www-form-urlencoded";

  if (!request.data.count(L"action"))
    request.data[L"action"] = L"update";

  http_request.url.path = L"/api/animelist/" + request.data[L"action"] + L"/" +
                          request.data[canonical_name_ + L"-id"] + L".xml";

  // Delete method doesn't require us to provide additional data
  if (request.data[L"action"] == L"delete")
    return;

  XmlDocument document;
  auto node_declaration = document.append_child(pugi::node_declaration);
  node_declaration.append_attribute(L"version") = L"1.0";
  node_declaration.append_attribute(L"encoding") = L"UTF-8";
  auto node_entry = document.append_child(L"entry");

  // MAL allows setting a new value to "times_rewatched" and others, but
  // there's no way to get the current value. So we avoid them altogether.
  const std::set<std::wstring> valid_tags{
      L"episode",
      L"status",
      L"score",
//    L"downloaded_episodes",
//    L"storage_type",
//    L"storage_value",
//    L"times_rewatched",
//    L"rewatch_value",
      L"date_start",
      L"date_finish",
//    L"priority",
//    L"enable_discussion",
      L"enable_rewatching",
//    L"comments",
//    L"fansub_group",
      L"tags"
  };
  for (const auto& pair : request.data) {
    auto tag = valid_tags.find(TranslateKeyTo(pair.first));
    if (tag != valid_tags.end()) {
      std::wstring value = pair.second;
      if (*tag == L"score") {
        value = ToWstr(TranslateMyRatingTo(ToInt(value)));
      } else if (*tag == L"status") {
        value = ToWstr(TranslateMyStatusTo(ToInt(value)));
      } else if (StartsWith(*tag, L"date")) {
        value = TranslateMyDateTo(value);
      }
      XmlWriteStr(node_entry, *tag, value);
    }
  }

  http_request.data[L"data"] = XmlDump(document);
}

////////////////////////////////////////////////////////////////////////////////
// Response handlers

void Service::AuthenticateUser(Response& response, HttpResponse& http_response) {
  user_.id = InStr(http_response.body, L"<id>", L"</id>");
  user_.username = InStr(http_response.body, L"<username>", L"</username>");
}

void Service::GetLibraryEntries(Response& response, HttpResponse& http_response) {
  XmlDocument document;
  const auto parse_result = document.load_string(http_response.body.c_str());

  if (!parse_result) {
    response.data[L"error"] = L"Could not parse the list";
    return;
  }

  auto node_myanimelist = document.child(L"myanimelist");

  // Available tags:
  // - user_id
  // - user_name
  // - user_watching
  // - user_completed
  // - user_onhold
  // - user_dropped
  // - user_plantowatch
  // - user_days_spent_watching
  auto node_myinfo = node_myanimelist.child(L"myinfo");
  user_.id = XmlReadStr(node_myinfo, L"user_id");
  user_.username = XmlReadStr(node_myinfo, L"user_name");
  // We ignore the remaining tags, because MAL can be very slow at updating
  // their values, and we can easily calculate them ourselves anyway.

  AnimeDatabase.ClearUserData();

  // Available tags:
  // - series_animedb_id
  // - series_title
  // - series_synonyms (separated by "; ")
  // - series_type
  // - series_episodes
  // - series_status
  // - series_start
  // - series_end
  // - series_image
  // - my_id (deprecated)
  // - my_watched_episodes
  // - my_start_date
  // - my_finish_date
  // - my_score
  // - my_status
  // - my_rewatching
  // - my_rewatching_ep
  // - my_last_updated
  // - my_tags
  for (auto node : node_myanimelist.children(L"anime")) {
    ::anime::Item anime_item;
    anime_item.SetSource(this->id());
    anime_item.SetId(XmlReadStr(node, L"series_animedb_id"), this->id());
    anime_item.SetLastModified(time(nullptr));  // current time

    anime_item.SetTitle(XmlReadStr(node, L"series_title"));
    anime_item.SetSynonyms(XmlReadStr(node, L"series_synonyms"));
    anime_item.SetType(TranslateSeriesTypeFrom(XmlReadInt(node, L"series_type")));
    anime_item.SetEpisodeCount(XmlReadInt(node, L"series_episodes"));
    anime_item.SetAiringStatus(TranslateSeriesStatusFrom(XmlReadInt(node, L"series_status")));
    anime_item.SetDateStart(XmlReadStr(node, L"series_start"));
    anime_item.SetDateEnd(XmlReadStr(node, L"series_end"));
    anime_item.SetImageUrl(XmlReadStr(node, L"series_image"));

    anime_item.AddtoUserList();
    anime_item.SetMyLastWatchedEpisode(XmlReadInt(node, L"my_watched_episodes"));
    anime_item.SetMyDateStart(XmlReadStr(node, L"my_start_date"));
    anime_item.SetMyDateEnd(XmlReadStr(node, L"my_finish_date"));
    anime_item.SetMyScore(TranslateMyRatingFrom(XmlReadInt(node, L"my_score")));
    anime_item.SetMyStatus(TranslateMyStatusFrom(XmlReadInt(node, L"my_status")));
    anime_item.SetMyRewatching(XmlReadInt(node, L"my_rewatching"));
    anime_item.SetMyRewatchingEp(XmlReadInt(node, L"my_rewatching_ep"));
    anime_item.SetMyLastUpdated(XmlReadStr(node, L"my_last_updated"));
    anime_item.SetMyTags(XmlReadStr(node, L"my_tags"));

    AnimeDatabase.UpdateItem(anime_item);
  }
}

void Service::GetMetadataById(Response& response, HttpResponse& http_response) {
  // Available data:
  // - ID
  // - Title (truncated, followed by year aired)
  // - Synopsis (limited to 200 characters)
  // - Genres
  // - Status (in string form)
  // - Type (in string form)
  // - Episodes
  // - Score
  // - Rank
  // - Popularity
  // - Members
  std::wstring id = InStr(http_response.body,
      L"/anime/", L"/");
  std::wstring title = InStr(http_response.body,
      L"class=\"hovertitle\">", L"</a>");
  std::wstring genres = InStr(http_response.body,
      L"Genres:</span> ", L"<br />");
  std::wstring status = InStr(http_response.body,
      L"Status:</span> ", L"<br />");
  std::wstring type = InStr(http_response.body,
      L"Type:</span> ", L"<br />");
  std::wstring episodes = InStr(http_response.body,
      L"Episodes:</span> ", L"<br />");
  std::wstring score = InStr(http_response.body,
      L"Score:</span> ", L"<br />");
  std::wstring popularity = InStr(http_response.body,
      L"Popularity:</span> ", L"<br />");

  bool title_is_truncated = false;

  title = DecodeText(title);
  if (EndsWith(title, L")") && title.length() > 7)
    title = title.substr(0, title.length() - 7);
  if (EndsWith(title, L"...") && title.length() > 3) {
    title = title.substr(0, title.length() - 3);
    title_is_truncated = true;
  }

  std::vector<std::wstring> genres_vector;
  Split(genres, L", ", genres_vector);

  StripHtmlTags(score);
  int pos = InStr(score, L" (", 0);
  if (pos > -1)
    score.resize(pos);

  TrimLeft(popularity, L"#");

  ::anime::Item anime_item;
  anime_item.SetSource(this->id());
  anime_item.SetId(id, this->id());
  if (!title_is_truncated)
    anime_item.SetTitle(title);
  anime_item.SetAiringStatus(TranslateSeriesStatusFrom(status));
  anime_item.SetType(TranslateSeriesTypeFrom(type));
  anime_item.SetEpisodeCount(ToInt(episodes));
  anime_item.SetGenres(genres_vector);
  anime_item.SetPopularity(ToInt(popularity));
  anime_item.SetScore(ToDouble(score));
  anime_item.SetLastModified(time(nullptr));  // current time

  AnimeDatabase.UpdateItem(anime_item);
}

void Service::SearchTitle(Response& response, HttpResponse& http_response) {
  XmlDocument document;
  const auto parse_result = document.load_string(http_response.body.c_str());

  if (!parse_result) {
    response.data[L"error"] = L"Could not parse search results";
    return;
  }

  auto node_anime = document.child(L"anime");

  // Available tags:
  // - id
  // - title (must be decoded)
  // - english (must be decoded)
  // - synonyms (must be decoded)
  // - episodes
  // - score
  // - type
  // - status
  // - start_date
  // - end_date
  // - synopsis (must be decoded)
  // - image
  for (auto node : node_anime.children(L"entry")) {
    ::anime::Item anime_item;
    anime_item.SetSource(this->id());
    anime_item.SetId(XmlReadStr(node, L"id"), this->id());
    anime_item.SetTitle(DecodeText(XmlReadStr(node, L"title")));
    anime_item.SetEnglishTitle(DecodeText(XmlReadStr(node, L"english")));
    anime_item.SetSynonyms(DecodeText(XmlReadStr(node, L"synonyms")));
    anime_item.SetEpisodeCount(XmlReadInt(node, L"episodes"));
    anime_item.SetScore(ToDouble(XmlReadStr(node, L"score")));
    anime_item.SetType(TranslateSeriesTypeFrom(XmlReadStr(node, L"type")));
    anime_item.SetAiringStatus(TranslateSeriesStatusFrom(XmlReadStr(node, L"status")));
    anime_item.SetDateStart(XmlReadStr(node, L"start_date"));
    anime_item.SetDateEnd(XmlReadStr(node, L"end_date"));
    std::wstring synopsis = EraseBbcode(DecodeText(XmlReadStr(node, L"synopsis")));
    if (!StartsWith(synopsis, L"No synopsis has been added for this series yet"))
      anime_item.SetSynopsis(synopsis);
    anime_item.SetImageUrl(XmlReadStr(node, L"image"));
    anime_item.SetLastModified(time(nullptr));  // current time

    int anime_id = AnimeDatabase.UpdateItem(anime_item);

    // We return a list of IDs so that we can display the results afterwards
    AppendString(response.data[L"ids"], ToWstr(anime_id), L",");
  }
}

void Service::AddLibraryEntry(Response& response, HttpResponse& http_response) {
  // Nothing to do here
}

void Service::DeleteLibraryEntry(Response& response, HttpResponse& http_response) {
  // Nothing to do here
}

void Service::UpdateLibraryEntry(Response& response, HttpResponse& http_response) {
  // Nothing to do here
}

////////////////////////////////////////////////////////////////////////////////

bool Service::RequestNeedsAuthentication(RequestType request_type) const {
  switch (request_type) {
    case kAddLibraryEntry:
    case kAuthenticateUser:
    case kDeleteLibraryEntry:
    case kSearchTitle:
    case kUpdateLibraryEntry:
      return true;
  }

  return false;
}

bool Service::RequestSucceeded(Response& response,
                               const HttpResponse& http_response) {
  // No content
  if (http_response.code == 204 || http_response.body.empty()) {
    response.data[L"error"] = name() + L" returned an empty response";
    return false;
  }

  // Unauthorized
  if (http_response.code == 401) {
    // MAL doesn't return a meaningful explanation, so we'll just assume that
    // either the username or the password is wrong
    response.data[L"error"] =
        L"Authorization failed (invalid username or password)";
    return false;
  }

  // Website login required
  if (http_response.code == 403) {
    // Users that haven't logged in to MAL in the past 90 days are required to
    // do so and clear the captcha first.
    if (InStr(http_response.body, L"Website login required") > -1) {
      response.data[L"error"] = http_response.body;
      response.data[L"website_login_required"] = L"true";
      HandleError(http_response, response);
      return false;
    }
  }

  // Not approved
  // TODO: Remove when MAL fixes its API
  if (http_response.code == 400) {
    if (InStr(http_response.body, L"This anime has not been approved yet") > -1) {
      response.data[L"error"] = http_response.body;
      response.data[L"not_approved"] = L"true";
      return false;
    }
  }

  // API is down
  // See: https://github.com/erengy/taiga/issues/588
  if (http_response.code == 404) {
    if (InStr(http_response.body, L"<title>404 Not Found</title>") > -1) {
      response.data[L"error"] =
          L"API is unavailable. Please contact MyAnimeList's customer service.";
      HandleError(http_response, response);
      return false;
    }
  }

  switch (response.type) {
    case kAddLibraryEntry:
      if (StartsWith(http_response.body, L"Created"))
        return true;
      // According to a previous documentation, this method was supposed to
      // "return the unique ID of the row generated by the insert"...
      if (IsNumericString(http_response.body))
        return true;
      // ...but it returned some HTML code instead. We're keeping these lines in
      // case MAL suddenly reverts to the old behavior.
      if (InStr(http_response.body, L"<title>201 Created</title>") > -1)
        return true;
      // If we try to add an anime that is already in user's list, MyAnimeList
      // returns a "400 Bad Request" response with "The anime (id: 12345) is
      // already in the list." error message. Here we ignore this error and
      // assume that our request succeeded.
      if (InStr(http_response.body, L"is already in the list") > -1)
        return true;
      break;
    case kAuthenticateUser:
      if (InStr(http_response.body, L"<username>") > -1)
        return true;
      break;
    case kDeleteLibraryEntry:
      if (StartsWith(http_response.body, L"Deleted"))
        return true;
      break;
    case kGetLibraryEntries:
      if (InStr(http_response.body, L"<myanimelist>", 0, true) > -1 &&
          InStr(http_response.body, L"<myinfo>", 0, true) > -1)
        return true;
      break;
    case kGetMetadataById:
      if (!InStr(http_response.body, L"/anime/", L"/").empty())
        return true;
      if (InStr(http_response.body, L"No such series found") > -1 ||
          InStr(http_response.body, L"/anime//") > -1) {
        response.data[L"error"] = L"Invalid anime ID";
        response.data[L"invalid_id"] = L"true";
        return false;
      }
      break;
    case kSearchTitle:
      return true;
    case kUpdateLibraryEntry:
      if (StartsWith(http_response.body, L"Updated"))
        return true;
      break;
  }

  // Set the error message on failure
  switch (response.type) {
    case kAuthenticateUser:
      response.data[L"error"] = http_response.body;
      break;
    case kAddLibraryEntry:
    case kDeleteLibraryEntry:
    case kUpdateLibraryEntry: {
      std::wstring error_message = http_response.body;
      ReplaceString(error_message, L"</div><div>", L"\r\n");
      StripHtmlTags(error_message);
      response.data[L"error"] = error_message;
      break;
    }
  }

  HandleError(http_response, response);

  return false;
}

}  // namespace myanimelist
}  // namespace sync
