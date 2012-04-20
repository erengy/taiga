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
#include "event.h"
#include "http.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "time.h"
#include "xml.h"

namespace mal {

// =============================================================================

AnimeValues::AnimeValues()
    : episode(-1),
      status(-1),
      score(-1),
      downloaded_episodes(-1),
      storage_type(-1),
      storage_value(-1.0f),
      times_rewatched(-1),
      rewatch_value(-1),
      date_start(EMPTY_STR),
      date_finish(EMPTY_STR),
      priority(-1),
      enable_discussion(-1),
      enable_rewatching(-1),
      comments(EMPTY_STR),
      fansub_group(EMPTY_STR),
      tags(EMPTY_STR) {
}

// =============================================================================

wstring GetUserPassEncoded() {
  return Base64Encode(Settings.Account.MAL.user + L":" + Settings.Account.MAL.password);
}

// =============================================================================

bool AskToDiscuss(int anime_id, int episode_number) {
  return MainClient.Post(L"myanimelist.net",
    L"/includes/ajax.inc.php?t=50",
    L"epNum=" + ToWstr(episode_number) + 
    L"&aid=" + ToWstr(anime_id) + 
    L"&id=" + ToWstr(anime_id), L"", 
    HTTP_MAL_AnimeAskToDiscuss, 
    anime_id);
}

void CheckProfile() {
  if (!Taiga.logged_in) return;
  MainClient.Get(L"myanimelist.net", 
    L"/editprofile.php?go=privacy", L"", 
    HTTP_MAL_Profile);
}

bool GetAnimeDetails(int anime_id, class HttpClient* client) {
  if (!client) client = &SearchClient;
  return client->Connect(L"myanimelist.net", 
    L"/includes/ajax.inc.php?t=64&id=" + ToWstr(anime_id), 
    L"", L"GET", L"", L"myanimelist.net", L"", 
    HTTP_MAL_AnimeDetails, 
    anime_id);
}

bool GetList(bool login) {
  if (Settings.Account.MAL.user.empty()) return false;
  return MainClient.Connect(L"myanimelist.net", 
    L"/malappinfo.php?u=" + Settings.Account.MAL.user + L"&status=all", 
    L"", L"GET", L"Accept-Encoding: gzip", L"",
    Taiga.GetDataPath() + L"user\\" + Settings.Account.MAL.user + L".xml",
    login ? HTTP_MAL_RefreshAndLogin : HTTP_MAL_RefreshList);
}

bool Login() {
  if (Taiga.logged_in || Settings.Account.MAL.user.empty() || Settings.Account.MAL.password.empty()) return false;
  switch (Settings.Account.MAL.api) {
    case MAL_API_OFFICIAL: {
      return MainClient.Connect(L"myanimelist.net", 
        L"/api/account/verify_credentials.xml", 
        L"", 
        L"GET", 
        MainClient.GetDefaultHeader() + L"Authorization: Basic " + GetUserPassEncoded(), 
        L"myanimelist.net", L"", 
        HTTP_MAL_Login);
    }
    case MAL_API_NONE: {
      return MainClient.Post(L"myanimelist.net", L"/login.php",
        L"username=" + Settings.Account.MAL.user + 
        L"&password=" + Settings.Account.MAL.password + 
        L"&cookie=1&sublogin=Login", L"", 
        HTTP_MAL_Login);
    }
  }
  return false;
}

bool DownloadImage(int anime_id, const wstring& image_url, class HttpClient* client) {
  if (image_url.empty()) return false;
  if (!client) client = &ImageClient;
  return client->Get(win32::Url(image_url), 
    anime::GetImagePath(anime_id), 
    HTTP_MAL_Image, 
    anime_id);
}

bool DownloadUserImage(bool thumb) {
  if (!AnimeDatabase.user.GetId()) return false;
  win32::Url url;
  wstring path = Taiga.GetDataPath() + L"user\\image\\";
  if (thumb) {
    url.Crack(L"http://myanimelist.net/images/userimages/thumbs/" + ToWstr(AnimeDatabase.user.GetId()) + L"_thumb.jpg");
    path += ToWstr(AnimeDatabase.user.GetId()) + L"_thumb.jpg";
  } else {
    url.Crack(L"http://cdn.myanimelist.net/images/userimages/" + ToWstr(AnimeDatabase.user.GetId()) + L".jpg");
    path += ToWstr(AnimeDatabase.user.GetId()) + L".jpg";
  }
  return ImageClient.Get(url, path, HTTP_MAL_UserImage);
}

bool ParseAnimeDetails(const wstring& data) {
  if (data.empty()) return false;

  int anime_id = ToInt(InStr(data, L"myanimelist.net/anime/", L"/"));
  if (!anime_id) return false;

  anime::Item anime_item;
  anime_item.SetId(anime_id);
  anime_item.SetGenres(InStr(data, L"Genres:</span> ", L"<br />"));
  anime_item.SetRank(InStr(data, L"Ranked:</span> ", L"<br />"));
  anime_item.SetPopularity(InStr(data, L"Popularity:</span> ", L"<br />"));
  anime_item.SetScore(StripHtml(InStr(data, L"Score:</span> ", L"<br />")));
  anime_item.last_modified = time(nullptr);
  AnimeDatabase.UpdateItem(anime_item);

  return true;
}

bool ParseSearchResult(const wstring& data, int anime_id) {
  if (data.empty()) return false;
  bool found_item = false;
  
  xml_document doc;
  xml_parse_result result = doc.load(data.c_str());
  
  if (result.status == status_ok) {
    xml_node node = doc.child(L"anime");
    for (xml_node entry = node.child(L"entry"); entry; entry = entry.next_sibling(L"entry")) {
      anime::Item anime_item;
      anime_item.SetId(XML_ReadIntValue(entry, L"id"));
      anime_item.SetTitle(mal::DecodeText(XML_ReadStrValue(entry, L"title")));
      anime_item.SetSynonyms(mal::DecodeText(XML_ReadStrValue(entry, L"synonyms")));
      anime_item.SetEpisodeCount(XML_ReadIntValue(entry, L"episodes"));
      anime_item.SetScore(XML_ReadStrValue(entry, L"score"));
      anime_item.SetType(mal::TranslateType(XML_ReadStrValue(entry, L"type")));
      anime_item.SetAiringStatus(mal::TranslateStatus(XML_ReadStrValue(entry, L"status")));
      anime_item.SetDate(anime::DATE_START, XML_ReadStrValue(entry, L"start_date"));
      anime_item.SetDate(anime::DATE_END, XML_ReadStrValue(entry, L"end_date"));
      anime_item.SetSynopsis(mal::DecodeText(XML_ReadStrValue(entry, L"synopsis")));
      anime_item.SetImageUrl(XML_ReadStrValue(entry, L"image"));
      anime_item.last_modified = time(nullptr);
      AnimeDatabase.UpdateItem(anime_item);
      if (!anime_id || anime_id == anime_item.GetId()) {
        found_item = true;
      }
    }
  }
  
  return found_item;
}

bool SearchAnime(int anime_id, wstring title, class HttpClient* client) {
  if (!client) client = &SearchClient;
  if (title.empty()) {
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    title = anime_item->GetTitle();
  }

  Replace(title, L"+", L"%2B", true);
  ReplaceChar(title, L'\u2729', L'\u2606'); // stress outlined white star to white star

  switch (Settings.Account.MAL.api) {
    case MAL_API_OFFICIAL: {
      if (Settings.Account.MAL.user.empty() || Settings.Account.MAL.password.empty()) {
        return false;
      }
      return client->Connect(L"myanimelist.net", 
        L"/api/anime/search.xml?q=" + title, 
        L"", 
        L"GET", 
        client->GetDefaultHeader() + L"Authorization: Basic " + GetUserPassEncoded(), 
        L"myanimelist.net", 
        L"", HTTP_MAL_SearchAnime, 
        static_cast<LPARAM>(anime_id));
    }
    case MAL_API_NONE: {
      if (anime_id <= anime::ID_UNKNOWN) {
        ViewAnimeSearch(title); // TEMP
      }
      return true;
    }
  }

  return false;
}

bool Update(AnimeValues& anime_values, int anime_id, int update_mode) {
  #define ADD_DATA_F(name, value) \
    if (value > -1.0f) { data += L"\r\n\t<" ##name L">" + ToWstr(value) + L"</" ##name L">"; }
  #define ADD_DATA_I(name, value) \
    if (value > -1) { data += L"\r\n\t<" ##name L">" + ToWstr(value) + L"</" ##name L">"; }
  #define ADD_DATA_S(name, value) \
    if (value != EMPTY_STR) { data += L"\r\n\t<" ##name L">" + value + L"</" ##name L">"; }
  
  auto anime_item = AnimeDatabase.FindItem(anime_id);
  if (!anime_item) return false;

  switch (Settings.Account.MAL.api) {
    // Use official MAL API
    case MAL_API_OFFICIAL: {
      // Build XML data
      wstring data = L"data=<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<entry>";
      ADD_DATA_I(L"episode", anime_values.episode);
      ADD_DATA_I(L"status", anime_values.status);
      ADD_DATA_I(L"score", anime_values.score);
      ADD_DATA_I(L"downloaded_episodes", anime_values.downloaded_episodes);
      ADD_DATA_I(L"storage_type", anime_values.storage_type);
      ADD_DATA_F(L"storage_value", anime_values.storage_value);
      ADD_DATA_I(L"times_rewatched", anime_values.times_rewatched);
      ADD_DATA_I(L"rewatch_value", anime_values.rewatch_value);
      switch (update_mode) { // TODO: Move to EventList::Check() or somewhere else
        case HTTP_MAL_AnimeEdit: {
          const Date& date_start = anime_item->GetDate(anime::DATE_START);
          const Date& date_end = anime_item->GetDate(anime::DATE_END);
          if (mal::IsValidDate(date_start)) {
            anime_values.date_start = PadChar(ToWstr(date_start.month), '0', 2) + 
                                      PadChar(ToWstr(date_start.day),   '0', 2) + 
                                      PadChar(ToWstr(date_start.year),  '0', 4);
          }
          if (mal::IsValidDate(date_end)) {
            anime_values.date_finish = PadChar(ToWstr(date_end.month), '0', 2) + 
                                       PadChar(ToWstr(date_end.day),   '0', 2) + 
                                       PadChar(ToWstr(date_end.year),  '0', 4);
          }
          break;
        }
      }
      ADD_DATA_S(L"date_start", anime_values.date_start);
      ADD_DATA_S(L"date_finish", anime_values.date_finish);
      ADD_DATA_I(L"priority", anime_values.priority);
      ADD_DATA_I(L"enable_discussion", anime_values.enable_discussion);
      ADD_DATA_I(L"enable_rewatching", anime_values.enable_rewatching);
      ADD_DATA_S(L"comments", anime_values.comments);
      ADD_DATA_S(L"fansub_group", anime_values.fansub_group);
      ADD_DATA_S(L"tags", anime_values.tags);
      data += L"\r\n</entry>";
      
      win32::Url url;
      switch (update_mode) {
        // Add anime
        case HTTP_MAL_AnimeAdd: {
          url = L"myanimelist.net/api/animelist/add/" + ToWstr(anime_id) + L".xml";
          break;
        }
        // Delete anime
        case HTTP_MAL_AnimeDelete: {
          url = L"myanimelist.net/api/animelist/delete/" + ToWstr(anime_id) + L".xml";
          break;
        }
        // Update anime
        default: {
          url = L"myanimelist.net/api/animelist/update/" + ToWstr(anime_id) + L".xml";
          break;
        }
      }

      MainClient.Connect(url, data, L"POST", 
        MainClient.GetDefaultHeader() + L"Authorization: Basic " + GetUserPassEncoded(), 
        L"myanimelist.net", L"", update_mode, static_cast<LPARAM>(anime_id));
      break;
    }
  
    // Use default update method  
    case MAL_API_NONE: {
      switch (update_mode) {
        // Update episode
        case HTTP_MAL_AnimeUpdate:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=79", 
            L"anime_id=" + ToWstr(anime_id) + 
            L"&ep_val="  + ToWstr(anime_values.episode), 
            L"", update_mode, static_cast<LPARAM>(anime_id));
          break;
        // Update score
        case HTTP_MAL_ScoreUpdate:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=63", 
            L"id="     + ToWstr(anime_id) + 
            L"&score=" + ToWstr(anime_values.score), 
            L"", update_mode, static_cast<LPARAM>(anime_id));
          break;
        // Update tags
        case HTTP_MAL_TagUpdate:
          MainClient.Get(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=22" \
            L"&aid="  + ToWstr(anime_id) + 
            L"&tags=" + EncodeUrl(anime_values.tags), 
            L"", update_mode, static_cast<LPARAM>(anime_id));
          break;
        // Add anime
        case HTTP_MAL_AnimeAdd:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=61", 
            L"aid="      + ToWstr(anime_id) + 
            L"&score=0"
            L"&status="  + ToWstr(anime_values.status) + 
            L"&epsseen=" + ToWstr(anime_values.episode), 
            L"", update_mode, static_cast<LPARAM>(anime_id));
          break;
        // Delete anime
        case HTTP_MAL_AnimeDelete: 
          MainClient.Post(L"myanimelist.net", 
            L"/editlist.php?type=anime&id=" + ToWstr(anime_id), 
            L"series_id=" + ToWstr(anime_id) +
            L"&anime_db_series_id=" + ToWstr(anime_id) + 
            L"&series_title=" + ToWstr(anime_id) + 
            L"&submitIt=3" + 
            L"&hideLayout",
            L"", update_mode, static_cast<LPARAM>(anime_id));
          break;
        // Update status
        case HTTP_MAL_StatusUpdate:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=62", 
            L"aid="      + ToWstr(anime_id) + 
            L"&alistid=" + ToWstr(anime_id) + 
            L"&score="   + ToWstr(anime_item->GetMyScore(false)) + 
            L"&status="  + ToWstr(anime_values.status) + 
            L"&epsseen=" + ToWstr(anime_values.episode > -1 ? anime_values.episode : anime_item->GetMyLastWatchedEpisode(false)), 
            L"", update_mode, static_cast<LPARAM>(anime_id));
          break;
        // Edit anime
        case HTTP_MAL_AnimeEdit: {
          wstring buffer = 
            L"series_id="           + ToWstr(anime_id) + 
            L"&anime_db_series_id=" + ToWstr(anime_id) + 
            L"&series_title="       + ToWstr(anime_id) + 
            L"&aeps="               + ToWstr(anime_item->GetEpisodeCount(false)) + 
            L"&astatus="            + ToWstr(anime_item->GetAiringStatus()) + 
            L"&close_on_update=true"
            L"&status="             + ToWstr(anime_values.status);
          if (anime_values.enable_rewatching > -1) {
            buffer += 
            L"&rewatching="         + ToWstr(anime_values.enable_rewatching);
          }
          buffer += 
            L"&last_status="        + ToWstr(anime_item->GetMyStatus(false)) + 
            L"&completed_eps="      + ToWstr(anime_values.episode > -1 ? anime_values.episode : anime_item->GetMyLastWatchedEpisode(false)) + 
            L"&last_completed_eps=" + ToWstr(anime_item->GetMyLastWatchedEpisode(false)) + 
            L"&score="              + ToWstr(anime_values.score > -1 ? anime_values.score : anime_item->GetMyScore(false));
          if (anime_values.tags != EMPTY_STR) {
            buffer += 
            L"&tags=" + EncodeUrl(anime_values.tags);
          }
          const Date& date_start = anime_item->GetMyDate(anime::DATE_START);
          if (mal::IsValidDate(date_start)) {
            buffer += 
            L"&startMonth=" + ToWstr(date_start.month) + 
            L"&startDay="   + ToWstr(date_start.day) + 
            L"&startYear="  + ToWstr(date_start.year);
          } else {
            buffer += 
            L"&unknownStart=1";
          }
          const Date& date_end = anime_item->GetMyDate(anime::DATE_END);
          if (mal::IsValidDate(date_end)) {
            buffer += 
            L"&endMonth=" + ToWstr(date_end.month) + 
            L"&endDay="   + ToWstr(date_end.day) + 
            L"&endYear="  + ToWstr(date_end.year);
          } else {
            buffer += 
            L"&unknownEnd=1";
          }
          buffer += L"&submitIt=2";
          MainClient.Post(L"myanimelist.net", 
            L"/editlist.php?type=anime&id=" + ToWstr(anime_id) + L"&hideLayout=true", 
            buffer, L"", update_mode, static_cast<LPARAM>(anime_id));
          break;
        }
      }
      break;
    }
  }

  #undef ADD_DATA_F
  #undef ADD_DATA_I
  #undef ADD_DATA_S
  #undef ANIME

  return true;
}

bool UpdateSucceeded(EventItem& item, const wstring& data, int status_code) {
  int update_mode = item.mode;
  bool success = false;
  
  switch (Settings.Account.MAL.api) {
    case MAL_API_OFFICIAL: {
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
      break;
    }
    case MAL_API_NONE: {
      switch (update_mode) {
        case HTTP_MAL_AnimeAdd:
          success = true; // TODO
          break;
        case HTTP_MAL_AnimeDelete:
          success = (InStr(data, L"Success", 0) > -1) || (InStr(data, L"This is not your entry", 0) > -1);
          break;
        case HTTP_MAL_AnimeUpdate:
          success = ToInt(data) == item.episode;
          break;
        case HTTP_MAL_ScoreUpdate:
          success = InStr(data, L"Updated score", 0) > -1;
          break;
        case HTTP_MAL_TagUpdate:
          success = item.tags.empty() ? data.empty() : InStr(data, L"/animelist/", 0) > -1;
          break;
        case HTTP_MAL_AnimeEdit:
        case HTTP_MAL_StatusUpdate:
          success = InStr(data, L"Success", 0) > -1;
          break;
      }
      break;
    }
  }
  if (success) return true;

  // Set error message on failure     
  switch (Settings.Account.MAL.api) {
    case MAL_API_OFFICIAL:
      item.reason = data;
      break;
    case MAL_API_NONE:
      switch (update_mode) {
        case HTTP_MAL_AnimeDelete:
          item.reason = InStr(data, L"class=\"badresult\">", L"</div>");
          if (item.reason.empty()) item.reason = data;
          break;
        default:
          item.reason = data;
          break;
      }
  }
  Replace(item.reason, L"</div><div>", L"\r\n");
  item.reason = StripHtml(item.reason);
  return false;
}

// =============================================================================

wstring DecodeText(wstring text) {
  // TODO: Remove when MAL fixes its encoding >_<
  #define HTMLCHARCOUNT 36
  static const wchar_t* html_chars[HTMLCHARCOUNT][2] = {
    /* Extreme measures */
    // black star (black and white stars are encoded the same in API >_<)
    {L"k&acirc;\uFFFD\uFFFDR", L"k\u2605R"},
    // right single quotation mark (followed by an 's')
    {L"&acirc;\uFFFD\uFFFDs ", L"\u2019s "},
    // surname of the Yellow Emperor (don't ask why, I just got the name from FileFormat.info)
    {L"&egrave;&raquo;\uFFFD&aring;", L"\u8ED2"},
    // Onegai My Melody Kirara complete encode (spare the star, because I can't crack that damn encoding 
    // so it will work with the rest of this crappy encoding mal employs in their API)...
    {L"&atilde;\uFFFD\uFFFD&atilde;\uFFFD&shy;&atilde;\uFFFD\uFFFD&atilde;\uFFFD\uFFFD&atilde;\uFFFD\uFFFD"
     L"&atilde;\uFFFD&curren;&atilde;\uFFFD&iexcl;&atilde;\uFFFD&shy;&atilde;\uFFFD\uFFFD&atilde;\uFFFD&pound; "
     L"&atilde;\uFFFD\uFFFD&atilde;\uFFFD\uFFFD&atilde;\uFFFD\uFFFD&atilde;\uFFFD&pound;", 
     L"\u304A\u306D\u304C\u3044\u30DE\u30A4\u30E1\u30ED\u30C7\u30A3 \u304D\u3089\u3089\u3063"},
    /* Characters are sorted by their Unicode value */
    {L"&Acirc;&sup2;",          L"\u00B2"},   // superscript 2
    {L"&Acirc;&sup3;",          L"\u00B3"},   // superscript 3
    {L"&Acirc;&frac12;",        L"\u00BD"},   // fraction 1/2
    {L"&Atilde;&cent;",         L"\u00E2"},   // small a, circumflex accent
    {L"&Atilde;&curren;",       L"\u00E4"},   // small a, umlaut mark
    {L"&Atilde;&brvbar",        L"\u00E6"},   // latin small letter ae
    {L"&Atilde;&uml;",          L"\u00E8"},   // small e, grave accent
    {L"&Atilde;&copy;",         L"\u00E9"},   // small e, acute accent
    {L"&Atilde;&frac14;",       L"\u00FC"},   // small u, umlaut mark
    {L"&Aring;&laquo;",         L"\u016B"},   // small u, macron mark
    {L"&Icirc;&nbsp;",          L"\u03A0"},   // greek capital letter pi
    {L"&Icirc;&pound;",         L"\u03A3"},   // greek capital letter sigma
    {L"&acirc;\uFFFD&frac14;",  L"\u203C"},   // double exclamation mark
    {L"&acirc;\uFFFD&yen;",     L"\u2665"},   // heart
    {L"&acirc;\uFFFD&ordf;",    L"\u266A"},   // eighth note
    {L"&acirc;\uFFFD\uFFFD",    L"\u2729"},   // white star
    {L"&atilde;\uFFFD&ordf;",   L"\u30AA"},   // katakana letter o
    {L"&atilde;\uFFFD&not;",    L"\u30AC"},   // katakana letter ga
    {L"&atilde;\uFFFD&macr;",   L"\u30AF"},   // katakana letter ku
    {L"&atilde;\uFFFD&iquest;", L"\u30BF"},   // katakana letter ta
    {L"&atilde;\uFFFD\uFFFD",   L"\u30CD"},   // katakana letter ne
    {L"&atilde;\uFFFD&iexcl;",  L"\u30E1"},   // katakana letter me
    {L"&aring;\uFFFD&shy;",     L"\u516D"},   // han character 'number six'
    {L"&sup3;&para;",           L"\u5CF6"},   // han character 'island'
    {L"&aring;&iquest;\uFFFD",  L"\u5FCD"},   // endure
    {L"&aelig;\uFFFD\uFFFD",    L"\u624B"},   // hand
    {L"&aelig;\uFFFD&frac14;",  L"\u62BC"},   // mortgage
    {L"&ccedil;&copy;&ordm;",   L"\u7A7A"},   // empty
    {L"&eacute;\uFFFD&uml;",    L"\u90E8"},   // division
    /* Keep these at the end so they get replaced after others that include \uFFFD */
    {L"&Atilde;\uFFFD",         L"\u00DF"},   // small sharp s, German
    {L"&Aring;\uFFFD",          L"\u014D"},   // small o, macron mark
    {L"&acirc;\uFFFD",          L"\u2020"},   // dagger
  };
  if (InStr(text, L"&") > -1) {
    for (int i = 0; i < HTMLCHARCOUNT; i++) {
      Replace(text, html_chars[i][0], html_chars[i][1], true, false);
    }
  }
  #undef HTMLCHARCOUNT
  
  Replace(text, L"<br />", L"\r", true);
  Replace(text, L"\n\n", L"\r\n\r\n", true);
  text = DecodeHtml(StripHtml(text));

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
      (episode > total && total != 0)) {
        return false;
  }
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

// =============================================================================

wstring TranslateDate(const Date& date) {
  if (!mal::IsValidDate(date)) return L"?";

  wstring result;
  
  if (date.month > 0 && date.month <= 12) {
    const wchar_t* months[] = {
      L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", 
      L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
    };
    result += months[date.month - 1];
    result += L" ";
  }
  if (date.day > 0) result += ToWstr(date.day) + L", ";
  result += ToWstr(date.year);

  return result;
}

wstring TranslateDateToSeason(const Date& date) {
  if (!mal::IsValidDate(date)) {
    return L"Unknown";
  } else {
    wstring season;
    unsigned short year = date.year;

    if (date.month == 0) {
      season = L"Unknown";
    } else if (date.month < 3) {  // Jan-Feb
      season = L"Winter";
      year--;
    } else if (date.month < 6) {  // Mar-May
      season = L"Spring";
    } else if (date.month < 9) {  // Jun-Aug
      season = L"Summer";
    } else if (date.month < 12) { // Sep-Nov
      season = L"Fall";
    } else {                      // Dec
      season = L"Winter";
    }
    
    return season + L" " + ToWstr(year);
  }
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
  return value == 0 ? default_char : ToWstr(value);
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

} // namespace mal