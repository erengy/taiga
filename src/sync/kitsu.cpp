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

#include <map>

#include "sync/kitsu.h"

#include "base/format.h"
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
#include "sync/kitsu_types.h"
#include "sync/kitsu_util.h"
#include "sync/sync.h"
#include "taiga/http.h"
#include "taiga/settings.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace sync::kitsu {

// API documentation:
// https://kitsu.docs.apiary.io

constexpr auto kBaseUrl = "https://kitsu.io/api/edge";

// Kitsu requires use of the JSON API media type: http://jsonapi.org/format/
constexpr auto kJsonApiMediaType = "application/vnd.api+json";

// Kitsu's configuration sets JSON API's maximum_page_size to 20. Asking for
// more results will return an error: "Limit exceeds maximum page size of 20."
// This means that our application could break if the server's configuration is
// changed to reduce maximum_page_size.
constexpr auto kJsonApiMaximumPageSize = 20;

// Library requests are limited to 500 entries per page.
constexpr auto kLibraryMaximumPageSize = 500;

// Application registration has not yet been implemented, so all requests are
// made with the following client ID and secret.
constexpr auto kClientId =
    "dd031b32d2f56c990b1425efe6c42ad847e7fe3ab46bf1299f05ecd856bdb7dd";
constexpr auto kClientSecret =
    "54d7307928f63414defd96399fc31ba847961ceaecef3a5fd93144e960c0e151";

////////////////////////////////////////////////////////////////////////////////

class Account {
public:
  static std::string username() {
    return WstrToStr(taiga::settings.GetSyncServiceKitsuUsername());
  }
  static void set_username(const std::string& username) {
    return taiga::settings.SetSyncServiceKitsuUsername(StrToWstr(username));
  }

  static std::string display_name() {
    return WstrToStr(taiga::settings.GetSyncServiceKitsuDisplayName());
  }
  static void set_display_name(const std::string& name) {
    return taiga::settings.SetSyncServiceKitsuDisplayName(StrToWstr(name));
  }

  static std::string email() {
    return WstrToStr(taiga::settings.GetSyncServiceKitsuEmail());
  }
  static void set_email(const std::string& email) {
    return taiga::settings.SetSyncServiceKitsuEmail(StrToWstr(email));
  }

  static std::string password() {
    return WstrToStr(taiga::settings.GetSyncServiceKitsuPassword());
  }
  static void set_password(const std::string& password) {
    taiga::settings.SetSyncServiceKitsuPassword(StrToWstr(password));
  }

  static std::string rating_system() {
    return WstrToStr(taiga::settings.GetSyncServiceKitsuRatingSystem());
  }
  static void set_rating_system(const std::string& rating_system) {
    taiga::settings.SetSyncServiceKitsuRatingSystem(StrToWstr(rating_system));
  }

  bool authenticated() const {
    return authenticated_;
  }
  void set_authenticated(const bool authenticated) {
    authenticated_ = authenticated;
  }

  time_t last_synchronized() const {
    return last_synchronized_;
  }
  void set_last_synchronized(const time_t last_synchronized) {
    last_synchronized_ = last_synchronized;
  }

  std::string access_token() const {
    return access_token_;
  }
  void set_access_token(const std::string& token) {
    access_token_ = token;
  }

  std::string id() const {
    return id_;
  }
  void set_id(const std::string& id) {
    id_ = id;
  }

private:
  bool authenticated_ = false;
  time_t last_synchronized_ = 0;
  std::string access_token_;
  std::string id_;
};

static Account account;

bool IsUserAuthenticated() {
  return account.authenticated();
}

////////////////////////////////////////////////////////////////////////////////

taiga::http::Request BuildRequest() {
  taiga::http::Request request;

  request.set_headers({
      {"Accept", kJsonApiMediaType},
      {"Accept-Charset", "utf-8"},
      {"Accept-Encoding", "gzip"}});

  // AuthenticateUser method returns an access token, which is to be used on all
  // methods that require authentication. Some methods don't require the token,
  // but behave differently (e.g. private library entries are included) when
  // it's provided.
  const auto access_token = account.access_token();
  if (!access_token.empty()) {
    request.set_header("Authorization", "Bearer {}"_format(access_token));
  }

  return request;
}

std::string BuildLibraryObject(const library::QueueItem& queue_item) {
  Json json = {
    {"data", {
      {"type", "libraryEntries"},
      {"attributes", {}},
      {"relationships", {
        {"anime", {
          {"data", {
            {"type", "anime"},
            {"id", ToStr(queue_item.anime_id)},
          }}
        }},
        {"user", {
          {"data", {
            {"type", "users"},
            {"id", account.id()},
          }}
        }}
      }}
    }}
  };

  // According to the JSON API specification, "The `id` member is not required
  // when the resource object originates at the client and represents a new
  // resource to be created on the server."
  if (const auto anime_item = anime::db.Find(queue_item.anime_id)) {
    if (const auto library_id = ToInt(anime_item->GetMyId())) {
      json["data"]["id"] = ToStr(library_id);
    }
  }

  auto& attributes = json["data"]["attributes"];

  // Note that we send `null` (rather than `0` or `""`) to remove certain values
  if (queue_item.date_finish) {
    const auto finished_at =
        TranslateMyDateTo(queue_item.date_finish->to_string());
    if (!finished_at.empty()) {
      attributes["finishedAt"] = finished_at;
    } else {
      attributes["finishedAt"] = nullptr;
    }
  }
  if (queue_item.notes)
    attributes["notes"] = WstrToStr(*queue_item.notes);
  if (queue_item.episode)
    attributes["progress"] = *queue_item.episode;
  if (queue_item.score) {
    const auto score = *queue_item.score;
    if (score > 0) {
      attributes["ratingTwenty"] = TranslateMyRatingTo(score);
    } else {
      attributes["ratingTwenty"] = nullptr;
    }
  }
  if (queue_item.rewatched_times)
    attributes["reconsumeCount"] = *queue_item.rewatched_times;
  if (queue_item.enable_rewatching)
    attributes["reconsuming"] = *queue_item.enable_rewatching;
  if (queue_item.date_start) {
    const auto started_at =
        TranslateMyDateTo(queue_item.date_start->to_string());
    if (!started_at.empty()) {
      attributes["startedAt"] = started_at;
    } else {
      attributes["startedAt"] = nullptr;
    }
  }
  if (queue_item.status)
    attributes["status"] = TranslateMyStatusTo(*queue_item.status);

  return json.dump();
}

void UseSparseFieldsetsForAnime(hypr::Params& params, bool minimal) {
  // Requesting a restricted set of fields decreases response and download times
  // in certain cases.
  std::string fields_anime =
      // attributes
      "abbreviatedTitles,"
      "ageRating,"
      "averageRating,"
      "canonicalTitle,"
      "endDate,"
      "episodeCount,"
      "episodeLength,"
      "popularityRank,"
      "posterImage,"
      "slug,"
      "startDate,"
      "subtype,"
      "titles,"
      // relationships
      "animeProductions,"
      "categories";
  if (!minimal) {
    fields_anime += ",synopsis";
  }

  params.add("fields[anime]", fields_anime);
  params.add("fields[animeProductions]", "producer");
  params.add("fields[categories]", "title");
  params.add("fields[producers]", "name");
}

void UseSparseFieldsetsForLibraryEntries(hypr::Params& params) {
  params.add("fields[libraryEntries]",
      // attributes
      "finishedAt,"
      "notes,"
      "progress,"
      "ratingTwenty,"
      "reconsumeCount,"
      "reconsuming,"
      "startedAt,"
      "status,"
      "updatedAt,"
      // relationships
      "anime");
}

void UseSparseFieldsetsForUser(hypr::Params& params) {
  params.add("fields[users]",
      "email,"
      "name,"
      "ratingSystem,"
      "slug");
}

int ParseAnimeObject(const Json& json) {
  const auto anime_id = ToInt(JsonReadStr(json, "id"));
  const auto& attributes = json["attributes"];

  if (!anime_id) {
    LOGW(L"Could not parse anime object:\n{}", StrToWstr(json.dump()));
    return anime::ID_UNKNOWN;
  }

  anime::Item anime_item;
  anime_item.SetSource(kKitsu);
  anime_item.SetId(ToWstr(anime_id), kKitsu);
  anime_item.SetLastModified(time(nullptr));  // current time

  anime_item.SetAgeRating(
      TranslateAgeRatingFrom(JsonReadStr(attributes, "ageRating")));
  anime_item.SetScore(
      TranslateSeriesRatingFrom(JsonReadStr(attributes, "averageRating")));
  anime_item.SetTitle(StrToWstr(JsonReadStr(attributes, "canonicalTitle")));
  anime_item.SetDateEnd(StrToWstr(JsonReadStr(attributes, "endDate")));
  anime_item.SetEpisodeCount(JsonReadInt(attributes, "episodeCount"));
  anime_item.SetEpisodeLength(JsonReadInt(attributes, "episodeLength"));
  anime_item.SetPopularity(JsonReadInt(attributes, "popularityRank"));
  anime_item.SetImageUrl(
      StrToWstr(JsonReadStr(attributes["posterImage"], "small")));
  anime_item.SetSlug(StrToWstr(JsonReadStr(attributes, "slug")));
  anime_item.SetDateStart(StrToWstr(JsonReadStr(attributes, "startDate")));
  anime_item.SetType(
      TranslateSeriesTypeFrom(JsonReadStr(attributes, "subtype")));
  anime_item.SetSynopsis(
      anime::NormalizeSynopsis(StrToWstr(JsonReadStr(attributes, "synopsis"))));

  for (const auto& title : attributes["abbreviatedTitles"]) {
    if (title.is_string())
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

  return anime::db.UpdateItem(anime_item);
}

void ParseCategories(const Json& json, const int anime_id) {
  const auto anime_item = anime::db.Find(anime_id);

  if (!anime_item)
    return;

  std::vector<std::wstring> categories;

  // Here we assume that all listed categories are related to this anime. In order
  // to parse an array of anime objects with categories, we would need to extract
  // category IDs from `relationships/categories/data` for each anime.
  for (const auto& value : json) {
    if (value["type"] == "categories") {
      categories.push_back(StrToWstr(value["attributes"]["title"]));
    }
  }

  anime_item->SetGenres(categories);
}

void ParseProducers(const Json& json, const int anime_id) {
  const auto anime_item = anime::db.Find(anime_id);

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

int ParseLibraryObject(const Json& json) {
  const auto& media = json["relationships"]["anime"];
  const auto& attributes = json["attributes"];

  const auto anime_id = ToInt(JsonReadStr(media["data"], "id"));
  const auto library_id = ToInt(JsonReadStr(json, "id"));

  if (!anime_id) {
    LOGW(L"Could not parse library entry #{}", library_id);
    return anime::ID_UNKNOWN;
  }

  anime::Item anime_item;
  anime_item.SetSource(kKitsu);
  anime_item.SetId(ToWstr(anime_id), kKitsu);
  anime_item.AddtoUserList();

  anime_item.SetMyId(ToWstr(library_id));
  anime_item.SetMyDateEnd(
      TranslateMyDateFrom(JsonReadStr(attributes, "finishedAt")));
  anime_item.SetMyNotes(StrToWstr(JsonReadStr(attributes, "notes")));
  anime_item.SetMyLastWatchedEpisode(JsonReadInt(attributes, "progress"));
  anime_item.SetMyScore(
      TranslateMyRatingFrom(JsonReadInt(attributes, "ratingTwenty")));
  anime_item.SetMyRewatchedTimes(JsonReadInt(attributes, "reconsumeCount"));
  anime_item.SetMyRewatching(JsonReadBool(attributes, "reconsuming"));
  anime_item.SetMyDateStart(
      TranslateMyDateFrom(JsonReadStr(attributes, "startedAt")));
  anime_item.SetMyStatus(
      TranslateMyStatusFrom(JsonReadStr(attributes, "status")));
  anime_item.SetMyLastUpdated(
      TranslateMyLastUpdatedFrom(JsonReadStr(attributes, "updatedAt")));

  return anime::db.UpdateItem(anime_item);
}

std::optional<int> GetOffset(const Json& json, const std::string& name) {
  if (const auto link = JsonReadStr(json["links"], name); !link.empty()) {
    Url url = StrToWstr(link);
    return ToInt(url.query[L"offset"]);
  }
  return std::nullopt;
}

void ParseObject(const Json& json) {
  enum class Type {
    Anime,
    Categories,
    LibraryEntries,
    Producers,
    Unknown,
  };

  const std::map<std::string, Type> table{
    {"anime", Type::Anime},
    {"categories", Type::Categories},
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
    case Type::Categories:
      break;
    case Type::LibraryEntries:
      ParseLibraryObject(json);
      break;
    case Type::Producers:
      break;
    default:
      LOGD(L"Invalid type: {}", StrToWstr(json["type"]));
      break;
  }
}

bool IsPartialLibraryRequest() {
  return account.last_synchronized() &&
         taiga::settings.GetSyncServiceKitsuPartialLibrary();
}

bool HasError(const taiga::http::Response& response) {
  const auto status_category = hypp::status::to_class(response.status_code());

  // 200 OK
  // 201 Created
  // 202 Accepted
  // 204 No Content
  if (status_category == 200)
    return true;

  if (Json root; JsonParseString(response.body(), root)) {
    std::string error_description;
    if (root.count("error_description")) {
      error_description = root["error_description"];
    } else if (root.count("errors")) {
      const auto& errors = root["errors"];
      if (errors.is_array()) {
        for (const auto error : errors) {
          error_description = "\"{}: {}\""_format(
              JsonReadStr(error, "title"), JsonReadStr(error, "detail"));
          break;
        }
      }
    }
    if (!error_description.empty()) {
      LOGE(StrToWstr(error_description));
      return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

void AuthenticateUser() {
  auto request = BuildRequest();
  request.set_method("POST");
  request.set_target("https://kitsu.io/api/oauth/token");
  request.set_header("Content-Type", "application/x-www-form-urlencoded");

  auto username = Account::email();
  if (username.empty())
    username = Account::username();

  // Resource Owner Password Credentials Grant
  // https://tools.ietf.org/html/rfc6749#section-4.3
  request.set_body({
      {"grant_type", "password"},
      {"username", username},
      {"password", Account::password()},
      {"client_id", kClientId},
      {"client_secret", kClientSecret}});

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer, L"Kitsu: Authenticating user...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      account.set_authenticated(false);
      ui::OnLogout();
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"Kitsu: Could not parse authentication data.");
      return;
    }

    account.set_access_token(JsonReadStr(root, "access_token"));
    account.set_authenticated(true);

    ui::OnLogin();
    GetUser();  // We need to make an additional request to get the user ID
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void GetUser() {
  auto request = BuildRequest();
  request.set_target("{}/edge/users"_format(kBaseUrl));

  hypr::Params params;
  if (account.authenticated()) {
    params.add("filter[self]", "true");
  } else {
    params.add("filter[slug]", Account::username());
  }
  UseSparseFieldsetsForUser(params);
  request.set_query(params);

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer, L"Kitsu: Retrieving user information...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"Kitsu: Could not parse user data.");
      return;
    }

    if (root["data"].empty()) {
      ui::ChangeStatusText(L"Kitsu: Invalid username");
      return;
    }

    const auto& user = root["data"].front();

    account.set_authenticated(true);
    account.set_id(JsonReadStr(user, "id"));

    Account::set_username(JsonReadStr(user["attributes"], "slug"));
    Account::set_rating_system(JsonReadStr(user["attributes"], "ratingSystem"));
    Account::set_display_name(JsonReadStr(user["attributes"], "name"));
    Account::set_email(JsonReadStr(user["attributes"], "email"));

    ui::OnLogin();
    if (account.authenticated()) {
      sync::Synchronize();
    } else {
      GetLibraryEntries();
    }
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void GetLibraryEntries(const int page) {
  if (account.id().empty()) {
    ui::ChangeStatusText(
        L"Kitsu: Cannot get anime list. User ID is unavailable.");
    return;
  }

  auto request = BuildRequest();
  request.set_target("{}/edge/library-entries"_format(kBaseUrl));

  hypr::Params params;
  params.add("filter[user_id]", account.id());
  params.add("filter[kind]", "anime");

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
  // after each restart (i.e. last_synchronized is not saved on exit).
  if (IsPartialLibraryRequest()) {
    const auto date = GetDate(account.last_synchronized() - (60 * 60 * 24));  // 1 day before, to be safe
    params.add("filter[since]", WstrToStr(date.to_string()));
  }

  params.add("include", "anime");

  // We would prefer retrieving the entire library in a single request. However,
  // Kitsu's server fails to respond in time for large libraries.
  params.add("page[offset]", ToStr(page));
  params.add("page[limit]", ToStr(kLibraryMaximumPageSize));

  UseSparseFieldsetsForAnime(params, true);
  UseSparseFieldsetsForLibraryEntries(params);
  request.set_query(params);

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer, L"Kitsu: Retrieving anime list...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"Kitsu: Could not parse anime list.");
      return;
    }

    const auto prev_page = GetOffset(root, "prev");
    const auto next_page = GetOffset(root, "next");

    if (!IsPartialLibraryRequest() && !prev_page) {
      anime::db.ClearUserData();
    }

    for (const auto& value : root["data"]) {
      ParseLibraryObject(value);
    }
    for (const auto& value : root["included"]) {
      ParseObject(value);
    }

    if (next_page) {
      GetLibraryEntries(*next_page);
    } else {
      account.set_last_synchronized(time(nullptr));  // current time
      sync::AfterGetLibrary();
    }
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void GetMetadataById(const int id) {
  auto request = BuildRequest();
  request.set_target("{}/edge/anime/{}"_format(kBaseUrl, id));

  hypr::Params params{{"include",
      "categories,"
      "animeProductions,"
      "animeProductions.producer"}};

  UseSparseFieldsetsForAnime(params, false);
  request.set_query(params);

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer, L"Kitsu: Retrieving anime information...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      return;
    }

    if (response.status_code() == 404) {
      // @TODO: Invalid anime ID
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root))  {
      ui::ChangeStatusText(L"Kitsu: Could not parse anime data.");
      return;
    }

    const auto anime_id = ParseAnimeObject(root["data"]);
    ParseCategories(root["included"], anime_id);
    ParseProducers(root["included"], anime_id);

    ui::OnLibraryEntryChange(anime_id);
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void GetSeason(const anime::Season season, const int page) {
  auto request = BuildRequest();
  request.set_target("{}/edge/anime"_format(kBaseUrl));

  hypr::Params params{
      {"filter[season]",
       WstrToStr(ToLower_Copy(ui::TranslateSeasonName(season.name)))},
      {"filter[season_year]", ToStr(season.year)},
      {"page[offset]", ToStr(page)},
      {"page[limit]", ToStr(kJsonApiMaximumPageSize)}};

  // We don't actually need the results to be sorted. But without this
  // parameter, we get inconsistent ordering and duplicate objects.
  params.add("sort", "-user_count");

  UseSparseFieldsetsForAnime(params, false);
  request.set_query(params);

  const auto on_transfer = [season](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer,
        L"Kitsu: Retrieving {} anime season..."_format(
            ui::TranslateSeason(season)));
  };

  const auto on_response = [season](const taiga::http::Response& response) {
    if (HasError(response)) {
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"Kitsu: Could not parse season data.");
      return;
    }

    const auto prev_page = GetOffset(root, "prev");
    const auto next_page = GetOffset(root, "next");

    if (!prev_page) {  // first page
      anime::season_db.items.clear();
    }

    for (const auto& value : root["data"]) {
      const auto anime_id = ParseAnimeObject(value);
      anime::season_db.items.push_back(anime_id);
      ui::OnLibraryEntryChange(anime_id);
    }

    if (next_page) {
      GetSeason(season, *next_page);
    } else {
      sync::AfterGetSeason();
    }
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void SearchTitle(const std::wstring& title) {
  auto request = BuildRequest();
  request.set_target("{}/edge/anime"_format(kBaseUrl));

  hypr::Params params{
      {"filter[text]", WstrToStr(title)},
      {"page[offset]", "0"},
      {"page[limit]", ToStr(kJsonApiMaximumPageSize)}};

  UseSparseFieldsetsForAnime(params, false);
  request.set_query(params);

  const auto on_transfer = [title](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer,
        L"Kitsu: Searching for \"{}\"..."_format(title));
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"Kitsu: Could not parse search results.");
      return;
    }

    std::wstring ids;
    for (const auto& value : root["data"]) {
      const auto anime_id = ParseAnimeObject(value);
      AppendString(ids, ToWstr(anime_id), L",");
    }
    ui::OnLibrarySearchTitle(anime::ID_UNKNOWN, ids);
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void AddLibraryEntry(const library::QueueItem& queue_item) {
  auto request = BuildRequest();
  request.set_method("POST");
  request.set_target("{}/edge/library-entries"_format(kBaseUrl));
  request.set_header("Content-Type", kJsonApiMediaType);

  hypr::Params params{{"include",
      "anime,"
      "anime.categories,"
      "anime.animeProductions,"
      "anime.animeProductions.producer"}};

  UseSparseFieldsetsForAnime(params, false);
  UseSparseFieldsetsForLibraryEntries(params);
  request.set_query(params);

  request.set_body(hypr::Body{BuildLibraryObject(queue_item)});

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer, L"Kitsu: Updating anime list...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      return;
    }

    // If we try to add a library entry that is already there, Kitsu returns a
    // "422 Unprocessable Entity" response with "animeId - has already been taken"
    // error message. Here we ignore this error and assume that our request
    // succeeded.
    if (hypp::status::to_class(response.status_code()) == 400) {
      return;
    }

    sync::AfterLibraryUpdate();
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void DeleteLibraryEntry(const int id) {
  auto request = BuildRequest();
  request.set_method("DELETE");
  request.set_target("{}/edge/library-entries/{}"_format(kBaseUrl, id));
  request.set_header("Content-Type", kJsonApiMediaType);

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer, L"Kitsu: Deleting anime from list...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      return;
    }

    // Returns "204 No Content" status and empty response body.

    sync::AfterLibraryUpdate();
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void UpdateLibraryEntry(const library::QueueItem& queue_item) {
  const auto anime_item = anime::db.Find(queue_item.anime_id);
  const auto library_id = ToInt(anime_item->GetMyId());

  auto request = BuildRequest();
  request.set_method("PATCH");
  request.set_target("{}/edge/library-entries/{}"_format(kBaseUrl, library_id));
  request.set_header("Content-Type", kJsonApiMediaType);

  hypr::Params params{{"include",
      "anime,"
      "anime.categories,"
      "anime.animeProductions,"
      "anime.animeProductions.producer"}};

  UseSparseFieldsetsForAnime(params, false);
  UseSparseFieldsetsForLibraryEntries(params);
  request.set_query(params);

  request.set_body(hypr::Body{BuildLibraryObject(queue_item)});

  const auto on_transfer = [](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer, L"Kitsu: Updating anime list...");
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (HasError(response)) {
      return;
    }

    Json root;
    if (!JsonParseString(response.body(), root)) {
      ui::ChangeStatusText(L"Kitsu: Could not parse anime list entry.");
      return;
    }

    const auto anime_id = ParseLibraryObject(root["data"]);

    if (!anime::IsValidId(anime_id))
      return;

    for (const auto& value : root["included"]) {
      ParseObject(value);
    }
    ParseCategories(root["included"], anime_id);
    ParseProducers(root["included"], anime_id);

    sync::AfterLibraryUpdate();
  };

  taiga::http::Send(request, on_transfer, on_response);
}

}  // namespace sync::kitsu
