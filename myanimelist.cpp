/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "std.h"

#include "myanimelist.h"

#include "anime.h"
#include "anime_db.h"
#include "common.h"
#include "history.h"
#include "http.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "time.h"
#include "xml.h"

namespace mal {

// =============================================================================

wstring GetUserPassEncoded() {
  return Base64Encode(Settings.Account.MAL.user + L":" + 
                      Settings.Account.MAL.password);
}

// =============================================================================

bool AskToDiscuss(int anime_id, int episode_number) {
  wstring data =
    L"epNum=" + ToWstr(episode_number) +
    L"&aid=" + ToWstr(anime_id) +
    L"&id=" + ToWstr(anime_id);

  return Clients.service.list.Post(
    L"myanimelist.net",
    L"/includes/ajax.inc.php?t=50",
    data, L"",
    HTTP_MAL_AnimeAskToDiscuss,
    anime_id);
}

bool GetAnimeDetails(int anime_id) {
  HttpClient* client;
  if (anime_id > anime::ID_UNKNOWN) {
    client = Clients.anime.GetClient(HTTP_Client_Search, anime_id);
  } else {
    client = &Clients.service.search;
  }
  
  return client->Connect(
    L"myanimelist.net",
    L"/includes/ajax.inc.php?t=64&id=" + ToWstr(anime_id),
    L"", L"GET", L"", L"myanimelist.net", L"",
    HTTP_MAL_AnimeDetails,
    anime_id);
}

bool GetList() {
  if (Settings.Account.MAL.user.empty())
    return false;

  return Clients.service.list.Connect(
    L"myanimelist.net",
    L"/malappinfo.php?u=" + Settings.Account.MAL.user + L"&status=all",
    L"", L"GET", L"Accept-Encoding: gzip", L"", L"",
    HTTP_MAL_RefreshList);
}

bool Login() {
  if (Taiga.logged_in || 
      Settings.Account.MAL.user.empty() || 
      Settings.Account.MAL.password.empty())
    return false;

  wstring header = Clients.service.list.GetDefaultHeader() +
                   L"Authorization: Basic " + GetUserPassEncoded();

  return Clients.service.list.Connect(
    L"myanimelist.net", 
    L"/api/account/verify_credentials.xml", 
    L"", L"GET", header, L"myanimelist.net", L"", 
    HTTP_MAL_Login);
}

bool DownloadImage(int anime_id, const wstring& image_url) {
  if (image_url.empty())
    return false;
  
  HttpClient* client;
  if (anime_id > anime::ID_UNKNOWN) {
    client = Clients.anime.GetClient(HTTP_Client_Image, anime_id);
  } else {
    client = &Clients.service.image;
  }
  
  return client->Get(win32::Url(image_url), 
                     anime::GetImagePath(anime_id), 
                     HTTP_MAL_Image, 
                     anime_id);
}

bool DownloadUserImage(bool thumb) {
  if (!AnimeDatabase.user.GetId())
    return false;
  
  win32::Url url;
  wstring path = Taiga.GetDataPath() + L"user\\" + 
                 AnimeDatabase.user.GetName() + L"\\";
  
  if (thumb) {
    url.Crack(L"http://myanimelist.net/images/userimages/thumbs/" + 
              ToWstr(AnimeDatabase.user.GetId()) + L"_thumb.jpg");
    path += ToWstr(AnimeDatabase.user.GetId()) + L"_thumb.jpg";
  } else {
    url.Crack(L"http://cdn.myanimelist.net/images/userimages/" + 
              ToWstr(AnimeDatabase.user.GetId()) + L".jpg");
    path += ToWstr(AnimeDatabase.user.GetId()) + L".jpg";
  }
  
  return Clients.service.image.Get(url, path, HTTP_MAL_UserImage);
}

bool ParseAnimeDetails(const wstring& data) {
  if (data.empty())
    return false;

  int anime_id = ToInt(InStr(data, L"myanimelist.net/anime/", L"/"));
  if (!anime_id)
    return false;

  anime::Item anime_item;
  anime_item.SetId(anime_id);
  anime_item.SetGenres(InStr(data, L"Genres:</span> ", L"<br />"));
  anime_item.SetRank(InStr(data, L"Ranked:</span> ", L"<br />"));
  anime_item.SetPopularity(InStr(data, L"Popularity:</span> ", L"<br />"));
  wstring score = InStr(data, L"Score:</span> ", L"<br />");
  StripHtmlTags(score);
  anime_item.SetScore(score);
  anime_item.last_modified = time(nullptr);
  AnimeDatabase.UpdateItem(anime_item);

  return true;
}

bool ParseSearchResult(const wstring& data, int anime_id) {
  if (data.empty())
    return false;
  
  bool found_item = false;
  
  xml_document doc;
  xml_parse_result result = doc.load(data.c_str());
  
  if (result.status == status_ok) {
    xml_node node = doc.child(L"anime");
    for (xml_node entry = node.child(L"entry"); entry; entry = entry.next_sibling(L"entry")) {
      auto current_anime_item = AnimeDatabase.FindItem(anime_id);

      anime::Item anime_item;
      anime_item.SetId(XML_ReadIntValue(entry, L"id"));
      if (!current_anime_item || !current_anime_item->keep_title)
        anime_item.SetTitle(mal::DecodeText(XML_ReadStrValue(entry, L"title")));
      anime_item.SetEnglishTitle(mal::DecodeText(XML_ReadStrValue(entry, L"english")));
      anime_item.SetSynonyms(mal::DecodeText(XML_ReadStrValue(entry, L"synonyms")));
      anime_item.SetEpisodeCount(XML_ReadIntValue(entry, L"episodes"));
      anime_item.SetScore(XML_ReadStrValue(entry, L"score"));
      anime_item.SetType(mal::TranslateType(XML_ReadStrValue(entry, L"type")));
      anime_item.SetAiringStatus(mal::TranslateStatus(XML_ReadStrValue(entry, L"status")));
      anime_item.SetDate(anime::DATE_START, XML_ReadStrValue(entry, L"start_date"));
      anime_item.SetDate(anime::DATE_END, XML_ReadStrValue(entry, L"end_date"));
      wstring synopsis = mal::DecodeText(XML_ReadStrValue(entry, L"synopsis"));
      if (!StartsWith(synopsis, L"No synopsis has been added for this series yet."))
        anime_item.SetSynopsis(synopsis);
      anime_item.SetImageUrl(XML_ReadStrValue(entry, L"image"));
      anime_item.last_modified = time(nullptr);
      AnimeDatabase.UpdateItem(anime_item);
      
      if (!anime_id || anime_id == anime_item.GetId())
        found_item = true;
    }
  }
  
  return found_item;
}

bool SearchAnime(int anime_id, wstring title) {
  HttpClient* client;
  if (anime_id > anime::ID_UNKNOWN) {
    client = Clients.anime.GetClient(HTTP_Client_Search, anime_id);
  } else {
    client = &Clients.service.search;
  }

  if (title.empty()) {
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    title = anime_item->GetTitle();
  }

  Replace(title, L"+", L"%2B", true);
  ReplaceChar(title, L'\u2729', L'\u2606'); // stress outlined white star to white star

  if (Settings.Account.MAL.user.empty() || 
      Settings.Account.MAL.password.empty())
    return false;

  wstring header = client->GetDefaultHeader() + 
                   L"Authorization: Basic " + GetUserPassEncoded();

  return client->Connect(
    L"myanimelist.net",
    L"/api/anime/search.xml?q=" + title,
    L"", L"GET", header, L"myanimelist.net", L"",
    HTTP_MAL_SearchAnime,
    anime_id);
}

bool Update(AnimeValues& anime_values, int anime_id, int update_mode) {
  auto anime_item = AnimeDatabase.FindItem(anime_id);
  if (!anime_item)
    return false;

  #define ADD_DATA_N(name, value) \
    if (anime_values.value) \
      data += L"\r\n\t<" ##name L">" + ToWstr(*anime_values.value) + L"</" ##name L">";
  #define ADD_DATA_S(name, value) \
    if (anime_values.value) \
      data += L"\r\n\t<" ##name L">" + *anime_values.value + L"</" ##name L">";

  // Build XML data
  wstring data;
  if (update_mode != HTTP_MAL_AnimeDelete) {
    data = L"data=<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<entry>";
    ADD_DATA_N(L"episode", episode);
    ADD_DATA_N(L"status", status);
    ADD_DATA_N(L"score", score);
    ADD_DATA_N(L"downloaded_episodes", downloaded_episodes);
    ADD_DATA_N(L"storage_type", storage_type);
    ADD_DATA_N(L"storage_value", storage_value);
    ADD_DATA_N(L"times_rewatched", times_rewatched);
    ADD_DATA_N(L"rewatch_value", rewatch_value);
    ADD_DATA_S(L"date_start", date_start);
    ADD_DATA_S(L"date_finish", date_finish);
    ADD_DATA_N(L"priority", priority);
    ADD_DATA_N(L"enable_discussion", enable_discussion);
    ADD_DATA_N(L"enable_rewatching", enable_rewatching);
    ADD_DATA_S(L"comments", comments);
    ADD_DATA_S(L"fansub_group", fansub_group);
    ADD_DATA_S(L"tags", tags);
    data += L"\r\n</entry>";
  }

  #undef ADD_DATA_N
  #undef ADD_DATA_S
  #undef ANIME

  win32::Url url;
  switch (update_mode) {
    // Add anime
    case HTTP_MAL_AnimeAdd:
      url = L"myanimelist.net/api/animelist/add/" + ToWstr(anime_id) + L".xml";
      break;
    // Delete anime
    case HTTP_MAL_AnimeDelete:
      url = L"myanimelist.net/api/animelist/delete/" + ToWstr(anime_id) + L".xml";
      break;
    // Update anime
    default:
      url = L"myanimelist.net/api/animelist/update/" + ToWstr(anime_id) + L".xml";
      break;
  }

  wstring header = Clients.service.list.GetDefaultHeader() + 
                   L"Authorization: Basic " + GetUserPassEncoded();

  Clients.service.list.Connect(
    url, data, L"POST", header, L"myanimelist.net", L"", 
    update_mode, static_cast<LPARAM>(anime_id));

  return true;
}

bool UpdateSucceeded(EventItem& item, const wstring& data, int status_code) {
  int update_mode = item.mode;
  bool success = false;

  switch (update_mode) {
    case HTTP_MAL_AnimeAdd:
      success = IsNumeric(data) || 
        InStr(data, L"This anime is already on your list") > -1 || 
        InStr(data, L"<title>201 Created</title>") > -1; // TODO: Remove when MAL fixes its API
      break;
    case HTTP_MAL_AnimeDelete:
      success = data == L"Deleted";
      break;
    default:
      success = data == L"Updated";
      break;
  }

  if (success)
    return true;

  // Set error message on failure     
  item.reason = data;
  Replace(item.reason, L"</div><div>", L"\r\n");
  StripHtmlTags(item.reason);

  return false;
}

// =============================================================================

wstring DecodeText(wstring text) {
  Replace(text, L"<br />", L"\r", true);
  Replace(text, L"\n\n", L"\r\n\r\n", true);
  
  StripHtmlTags(text);
  DecodeHtmlEntities(text);
  
  return text;
}

bool IsValidDate(const Date& date) {
  return date.year > 0;
}

bool IsValidDate(const wstring& date) {
  return date.length() == 10 && !StartsWith(date, L"0000");
}

bool IsValidEpisode(int episode, int watched, int total) {
  if ((episode < 0) ||
      (episode < watched) ||
      (episode == watched && total != 1) ||
      (episode > total && total != 0))
    return false;

  return true;
}

Date ParseDateString(const wstring& str) {
  Date date;

  if (IsValidDate(str)) {
    date.year  = ToInt(str.substr(0, 4));
    date.month = ToInt(str.substr(5, 2));
    date.day   = ToInt(str.substr(8, 2));
  }

  return date;
}

void GetSeasonInterval(const wstring& season, Date& date_start, Date& date_end) {
  std::map<wstring, std::pair<int, int>> interval;
  interval[L"Spring"] = std::make_pair<int, int>(3, 5);
  interval[L"Summer"] = std::make_pair<int, int>(6, 8);
  interval[L"Fall"] = std::make_pair<int, int>(9, 11);
  interval[L"Winter"] = std::make_pair<int, int>(12, 2);

  const int days_in_months[] = 
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  vector<wstring> season_year;
  Split(season, L" ", season_year);

  date_start.year = ToInt(season_year.at(1));
  date_end.year = ToInt(season_year.at(1));
  if (season_year.at(0) == L"Winter") date_start.year--;
  date_start.month = interval[season_year.at(0)].first;
  date_end.month = interval[season_year.at(0)].second;
  date_start.day = 1;
  date_end.day = days_in_months[date_end.month - 1];
}

// =============================================================================

wstring TranslateDate(const Date& date) {
  if (!mal::IsValidDate(date))
    return L"?";

  wstring result;

  if (date.month > 0 && date.month <= 12) {
    const wchar_t* months[] = {
      L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", 
      L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
    };
    result += months[date.month - 1];
    result += L" ";
  }
  if (date.day > 0)
    result += ToWstr(date.day) + L", ";
  result += ToWstr(date.year);

  return result;
}

wstring TranslateDateForApi(const Date& date) {
  if (!mal::IsValidDate(date))
    return L"";

  return PadChar(ToWstr(date.month), '0', 2) + // MM
         PadChar(ToWstr(date.day),   '0', 2) + // DD
         PadChar(ToWstr(date.year),  '0', 4);  // YYYY
}
Date TranslateDateFromApi(const wstring& date) {
  if (date.size() != 8)
    return Date();

  return Date(ToInt(date.substr(4, 4)), 
              ToInt(date.substr(0, 2)), 
              ToInt(date.substr(2, 2)));
}

wstring TranslateDateToSeason(const Date& date) {
  if (!mal::IsValidDate(date))
    return L"Unknown";

  wstring season;
  unsigned short year = date.year;

  if (date.month == 0) {
    season = L"Unknown";
  } else if (date.month < 3) {  // Jan-Feb
    season = L"Winter";
  } else if (date.month < 6) {  // Mar-May
    season = L"Spring";
  } else if (date.month < 9) {  // Jun-Aug
    season = L"Summer";
  } else if (date.month < 12) { // Sep-Nov
    season = L"Fall";
  } else {                      // Dec
    season = L"Winter";
    year++;
  }
    
  return season + L" " + ToWstr(year);
}

wstring TranslateSeasonToMonths(const wstring& season) {
  const wchar_t* months[] = {
    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", 
    L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
  };

  Date date_start, date_end;
  GetSeasonInterval(season, date_start, date_end);

  wstring result = months[date_start.month - 1];
  result += L" " + ToWstr(date_start.year) + L" to ";
  result += months[date_end.month - 1];
  result += L" " + ToWstr(date_end.year);
  
  return result;
}

wstring TranslateMyStatus(int value, bool add_count) {
  #define ADD_COUNT() (add_count ? L" (" + ToWstr(AnimeDatabase.user.GetItemCount(value)) + L")" : L"")
  switch (value) {
    case mal::MYSTATUS_NOTINLIST: return L"Not in list";
    case mal::MYSTATUS_WATCHING: return L"Currently watching" + ADD_COUNT();
    case mal::MYSTATUS_COMPLETED: return L"Completed" + ADD_COUNT();
    case mal::MYSTATUS_ONHOLD: return L"On hold" + ADD_COUNT();
    case mal::MYSTATUS_DROPPED: return L"Dropped" + ADD_COUNT();
    case mal::MYSTATUS_PLANTOWATCH: return L"Plan to watch" + ADD_COUNT();
    default: return L"";
  }
  #undef ADD_COUNT
}
int TranslateMyStatus(const wstring& value) {
  if (IsEqual(value, L"Currently watching")) {
    return mal::MYSTATUS_WATCHING;
  } else if (IsEqual(value, L"Completed")) {
    return mal::MYSTATUS_COMPLETED;
  } else if (IsEqual(value, L"On hold")) {
    return mal::MYSTATUS_ONHOLD;
  } else if (IsEqual(value, L"Dropped")) {
    return mal::MYSTATUS_DROPPED;
  } else if (IsEqual(value, L"Plan to watch")) {
    return mal::MYSTATUS_PLANTOWATCH;
  } else {
    return 0;
  }
}

wstring TranslateNumber(int value, const wstring& default_char) {
  return value > 0 ? ToWstr(value) : default_char;
}

wstring TranslateRewatchValue(int value) {
  switch (value) {
    case mal::REWATCH_VERYLOW: return L"Very low";
    case mal::REWATCH_LOW: return L"Low";
    case mal::REWATCH_MEDIUM: return L"Medium";
    case mal::REWATCH_HIGH: return L"High";
    case mal::REWATCH_VERYHIGH: return L"Very high";
    default: return L"";
  }
}

wstring TranslateStatus(int value) {
  switch (value) {
    case mal::STATUS_AIRING: return L"Currently airing";
    case mal::STATUS_FINISHED: return L"Finished airing";
    case mal::STATUS_NOTYETAIRED: return L"Not yet aired";
    default: return ToWstr(value);
  }
}
int TranslateStatus(const wstring& value) {
  if (IsEqual(value, L"Currently airing")) {
    return mal::STATUS_AIRING;
  } else if (IsEqual(value, L"Finished airing")) {
    return mal::STATUS_FINISHED;
  } else if (IsEqual(value, L"Not yet aired")) {
    return mal::STATUS_NOTYETAIRED;
  } else {
    return 0;
  }
}

wstring TranslateStorageType(int value) {
  switch (value) {
    case mal::STORAGE_HARDDRIVE: return L"Hard drive";
    case mal::STORAGE_DVDCD: return L"DVD/CD";
    case mal::STORAGE_NONE: return L"None";
    case mal::STORAGE_RETAILDVD: return L"Retail DVD";
    case mal::STORAGE_VHS: return L"VHS";
    case mal::STORAGE_EXTERNALHD: return L"External HD";
    case mal::STORAGE_NAS: return L"NAS";
    default: return L"";
  }
}

wstring TranslateType(int value) {
  switch (value) {
    case mal::TYPE_TV: return L"TV";
    case mal::TYPE_OVA: return L"OVA";
    case mal::TYPE_MOVIE: return L"Movie";
    case mal::TYPE_SPECIAL: return L"Special";
    case mal::TYPE_ONA: return L"ONA";
    case mal::TYPE_MUSIC: return L"Music";
    default: return L"";
  }
}
int TranslateType(const wstring& value) {
  if (IsEqual(value, L"TV")) {
    return mal::TYPE_TV;
  } else if (IsEqual(value, L"OVA")) {
    return mal::TYPE_OVA;
  } else if (IsEqual(value, L"Movie")) {
    return mal::TYPE_MOVIE;
  } else if (IsEqual(value, L"Special")) {
    return mal::TYPE_SPECIAL;
  } else if (IsEqual(value, L"ONA")) {
    return mal::TYPE_ONA;
  } else if (IsEqual(value, L"Music")) {
    return mal::TYPE_MUSIC;
  } else {
    return 0;
  }
}

// =============================================================================

void ViewAnimePage(int anime_id) {
  ExecuteLink(L"http://myanimelist.net/anime/" + ToWstr(anime_id) + L"/");
}

void ViewAnimeSearch(const wstring& title) {
  ExecuteLink(L"http://myanimelist.net/anime.php?q=" + title + L"&referer=" + APP_NAME);
}

void ViewHistory() {
  ExecuteLink(L"http://myanimelist.net/history/" + Settings.Account.MAL.user);
}

void ViewMessages() {
  ExecuteLink(L"http://myanimelist.net/mymessages.php");
}

void ViewPanel() {
  ExecuteLink(L"http://myanimelist.net/panel.php");
}

void ViewProfile() {
  ExecuteLink(L"http://myanimelist.net/profile/" + Settings.Account.MAL.user);
}

void ViewSeasonGroup() {
  ExecuteLink(L"http://myanimelist.net/clubs.php?cid=743");
}

void ViewUpcomingAnime() {
  Date date = GetDate();

  ExecuteLink(L"http://myanimelist.net/anime.php"
              L"?sd=" + ToWstr(date.day) + 
              L"&sm=" + ToWstr(date.month) + 
              L"&sy=" + ToWstr(date.year) + 
              L"&em=0&ed=0&ey=0&o=2&w=&c[]=a&c[]=d&cv=1");
}

} // namespace mal