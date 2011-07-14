/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include "animelist.h"
#include "common.h"
#include "event.h"
#include "http.h"
#include "myanimelist.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"

CMyAnimeList MAL;

#define MAL_USER_PASS Base64Encode(Settings.Account.MAL.User + L":" + Settings.Account.MAL.Password)

// =============================================================================

CMALAnimeValues::CMALAnimeValues() :
  episode(-1),
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
  tags(EMPTY_STR)
{
}

// =============================================================================

void CMyAnimeList::CheckProfile() {
  if (!Taiga.LoggedIn) return;
  MainClient.Get(L"myanimelist.net", 
    L"/editprofile.php?go=privacy", L"", 
    HTTP_MAL_Profile);
}

bool CMyAnimeList::GetAnimeDetails(CAnime* pAnimeItem) {
  if (!pAnimeItem) return false;
  return SearchClient.Connect(L"myanimelist.net", 
    L"/includes/ajax.inc.php?t=64&id=" + ToWSTR(pAnimeItem->Series_ID), 
    L"", L"GET", L"", L"myanimelist.net", 
    L"", HTTP_MAL_AnimeDetails, 
    reinterpret_cast<LPARAM>(pAnimeItem));
}

bool CMyAnimeList::GetList(bool login) {
  if (Settings.Account.MAL.User.empty()) return false;
  return MainClient.Connect(L"myanimelist.net", 
    L"/malappinfo.php?u=" + Settings.Account.MAL.User + L"&status=all", 
    L"", L"GET", L"Accept-Encoding: gzip", L"",
    Taiga.GetDataPath() + Settings.Account.MAL.User + L".xml",
    login ? HTTP_MAL_RefreshAndLogin : HTTP_MAL_RefreshList);
}

bool CMyAnimeList::Login() {
  if (Taiga.LoggedIn || Settings.Account.MAL.User.empty() || Settings.Account.MAL.Password.empty()) return false;
  switch (Settings.Account.MAL.API) {
    case MAL_API_OFFICIAL: {
      return MainClient.Connect(L"myanimelist.net", 
        L"/api/account/verify_credentials.xml", 
        L"", 
        L"GET", 
        MainClient.GetDefaultHeader() + L"Authorization: Basic " + MAL_USER_PASS, 
        L"myanimelist.net", 
        L"", HTTP_MAL_Login);
    }
    case MAL_API_NONE: {
      return MainClient.Post(L"myanimelist.net", L"/login.php",
        L"username=" + Settings.Account.MAL.User + 
        L"&password=" + Settings.Account.MAL.Password + 
        L"&cookie=1&sublogin=Login", 
        L"", HTTP_MAL_Login);
    }
  }
  return false;
}

void CMyAnimeList::DownloadImage(CAnime* pAnimeItem) {
  CUrl url(pAnimeItem->Series_Image);
  wstring file = Taiga.GetDataPath() + L"Image\\" + ToWSTR(pAnimeItem->Series_ID) + L".jpg";
  ImageClient.Get(url, file, HTTP_MAL_Image, reinterpret_cast<LPARAM>(pAnimeItem));
}

BOOL CMyAnimeList::SearchAnime(wstring title, CAnime* pAnimeItem) {
  if (title.empty()) return FALSE;
  Replace(title, L"+", L"%2B", true);

  switch (Settings.Account.MAL.API) {
    case MAL_API_OFFICIAL: {
      if (Settings.Account.MAL.User.empty() || Settings.Account.MAL.Password.empty()) {
        return FALSE;
      }
      return SearchClient.Connect(L"myanimelist.net", 
        L"/api/anime/search.xml?q=" + title, 
        L"", 
        L"GET", 
        SearchClient.GetDefaultHeader() + L"Authorization: Basic " + MAL_USER_PASS, 
        L"myanimelist.net", 
        L"", HTTP_MAL_SearchAnime, 
        reinterpret_cast<LPARAM>(pAnimeItem));
    }
    case MAL_API_NONE: {
      if (!pAnimeItem) {
        ViewAnimeSearch(title); // TEMP
      }
      return TRUE;
    }
  }

  return FALSE;
}

void CMyAnimeList::Update(CMALAnimeValues anime, int list_index, int anime_id, int update_mode) {
  #define ADD_DATA_F(name, value) if (value > -1.0f) { data += L"\r\n\t<" ##name L">" + ToWSTR(value) + L"</" ##name L">"; }
  #define ADD_DATA_I(name, value) if (value > -1) { data += L"\r\n\t<" ##name L">" + ToWSTR(value) + L"</" ##name L">"; }
  #define ADD_DATA_S(name, value) if (value != EMPTY_STR) { data += L"\r\n\t<" ##name L">" + value + L"</" ##name L">"; }
  #define ANIME AnimeList.Item[list_index]

  switch (Settings.Account.MAL.API) {
    // Use official MAL API
    case MAL_API_OFFICIAL: {
      // Build XML data
      wstring data = L"data=<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<entry>";
        ADD_DATA_I(L"episode", anime.episode);
        ADD_DATA_I(L"status", anime.status);
        ADD_DATA_I(L"score", anime.score);
        ADD_DATA_I(L"downloaded_episodes", anime.downloaded_episodes);
        ADD_DATA_I(L"storage_type", anime.storage_type);
        ADD_DATA_F(L"storage_value", anime.storage_value);
        ADD_DATA_I(L"times_rewatched", anime.times_rewatched);
        ADD_DATA_I(L"rewatch_value", anime.rewatch_value);
        switch (update_mode) { // TODO: Move to CEventList::Check() or somewhere else
          case HTTP_MAL_AnimeEdit: {
            if (MAL.IsValidDate(ANIME.My_StartDate)) {
              anime.date_start = ANIME.My_StartDate.substr(5, 2) + 
                                 ANIME.My_StartDate.substr(8, 2) + 
                                 ANIME.My_StartDate.substr(0, 4);
            }
            if (MAL.IsValidDate(ANIME.My_FinishDate)) {
              anime.date_finish = ANIME.My_FinishDate.substr(5, 2) + 
                                  ANIME.My_FinishDate.substr(8, 2) + 
                                  ANIME.My_FinishDate.substr(0, 4);
            }
            break;
          }
        }
        ADD_DATA_S(L"date_start", anime.date_start);
        ADD_DATA_S(L"date_finish", anime.date_finish);
        ADD_DATA_I(L"priority", anime.priority);
        ADD_DATA_I(L"enable_discussion", anime.enable_discussion);
        ADD_DATA_I(L"enable_rewatching", anime.enable_rewatching);
        ADD_DATA_S(L"comments", anime.comments);
        ADD_DATA_S(L"fansub_group", anime.fansub_group);
        ADD_DATA_S(L"tags", anime.tags);
      data += L"\r\n</entry>";
      
      switch (update_mode) {
        // Add anime
        case HTTP_MAL_AnimeAdd: {
          MainClient.Connect(L"myanimelist.net", 
            L"/api/animelist/add/" + ToWSTR(anime_id) + L".xml", 
            data, L"POST", 
            MainClient.GetDefaultHeader() + L"Authorization: Basic " + MAL_USER_PASS, 
            L"myanimelist.net", 
            L"", update_mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        }
        // Delete anime
        case HTTP_MAL_AnimeDelete: {
          MainClient.Connect(L"myanimelist.net", 
            L"/api/animelist/delete/" + ToWSTR(ANIME.Series_ID) + L".xml", 
            data, L"POST", 
            MainClient.GetDefaultHeader() + L"Authorization: Basic " + MAL_USER_PASS, 
            L"myanimelist.net", 
            L"", update_mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        }
        // Update anime
        default: {
          MainClient.Connect(L"myanimelist.net", 
            L"/api/animelist/update/" + ToWSTR(ANIME.Series_ID) + L".xml", 
            data, L"POST", 
            MainClient.GetDefaultHeader() + L"Authorization: Basic " + MAL_USER_PASS, 
            L"myanimelist.net", 
            L"", update_mode, reinterpret_cast<LPARAM>(&ANIME));
        }
      }
      break;
    }
  
    // Use default update method  
    case MAL_API_NONE: {
      switch (update_mode) {
        // Update episode
        case HTTP_MAL_AnimeUpdate:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=79", 
            L"anime_id=" + ToWSTR(ANIME.Series_ID) + 
            L"&ep_val="  + ToWSTR(anime.episode), 
            L"", update_mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        // Update score
        case HTTP_MAL_ScoreUpdate:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=63", 
            L"id="     + ToWSTR(ANIME.My_ID) + 
            L"&score=" + ToWSTR(anime.score), 
            L"", update_mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        // Update tags
        case HTTP_MAL_TagUpdate:
          MainClient.Get(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=22" \
            L"&aid="  + ToWSTR(ANIME.Series_ID) + 
            L"&tags=" + EncodeURL(anime.tags), 
            L"", update_mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        // Add anime
        case HTTP_MAL_AnimeAdd:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=61", 
            L"aid="      + ToWSTR(anime_id) + 
            L"&score=0"
            L"&status="  + ToWSTR(anime.status) + 
            L"&epsseen=" + ToWSTR(anime.episode), 
            L"", update_mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        // Delete anime
        case HTTP_MAL_AnimeDelete: 
          MainClient.Post(L"myanimelist.net", 
            L"/editlist.php?type=anime&id=" + ToWSTR(ANIME.My_ID), 
            L"series_id=" + ToWSTR(ANIME.My_ID) +
            L"&anime_db_series_id=" + ToWSTR(anime_id) + 
            L"&series_title=" + ToWSTR(anime_id) + 
            L"&submitIt=3" + 
            L"&hideLayout",
            L"", update_mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        // Update status
        case HTTP_MAL_StatusUpdate:
          MainClient.Post(L"myanimelist.net", 
            L"/includes/ajax.inc.php?t=62", 
            L"aid="      + ToWSTR(ANIME.Series_ID) + 
            L"&alistid=" + ToWSTR(ANIME.My_ID) + 
            L"&score="   + ToWSTR(ANIME.My_Score) + 
            L"&status="  + ToWSTR(anime.status) + 
            L"&epsseen=" + ToWSTR(anime.episode > -1 ? anime.episode : ANIME.My_WatchedEpisodes), 
            L"", update_mode, reinterpret_cast<LPARAM>(&ANIME));
          break;
        // Edit anime
        case HTTP_MAL_AnimeEdit: {
          wstring buffer = 
            L"series_id="           + ToWSTR(ANIME.My_ID) + 
            L"&anime_db_series_id=" + ToWSTR(ANIME.Series_ID) + 
            L"&series_title="       + ToWSTR(ANIME.Series_ID) + 
            L"&aeps="               + ToWSTR(ANIME.Series_Episodes) + 
            L"&astatus="            + ToWSTR(ANIME.Series_Status) + 
            L"&close_on_update=true"
            L"&status="             + ToWSTR(anime.status) + 
            L"&rewatch_ep=0"
            L"&last_status="        + ToWSTR(ANIME.My_Status) + 
            L"&completed_eps="      + ToWSTR(anime.episode > -1 ? anime.episode : ANIME.My_WatchedEpisodes) + 
            L"&last_completed_eps=" + ToWSTR(ANIME.My_WatchedEpisodes) + 
            L"&score="              + ToWSTR(anime.score > -1 ? anime.score : ANIME.My_Score);
          if (MAL.IsValidDate(ANIME.My_StartDate)) {
            unsigned short year, month, day;
            MAL.ParseDateString(ANIME.My_StartDate, year, month, day);
            buffer += 
            L"&startMonth=" + ToWSTR(month) + 
            L"&startDay="   + ToWSTR(day) + 
            L"&startYear="  + ToWSTR(year);
          } else {
            buffer += 
            L"&unknownStart=1";
          }
          if (MAL.IsValidDate(ANIME.My_FinishDate)) {
            unsigned short year, month, day;
            MAL.ParseDateString(ANIME.My_FinishDate, year, month, day);
            buffer += 
            L"&endMonth=" + ToWSTR(month) + 
            L"&endDay="   + ToWSTR(day) + 
            L"&endYear="  + ToWSTR(year);
          } else {
            buffer += 
            L"&unknownEnd=1";
          }
          buffer += L"&submitIt=2";
          MainClient.Post(L"myanimelist.net", 
            L"/editlist.php?type=anime&id=" + ToWSTR(ANIME.My_ID),
            //L"/panel.php?keepThis=true&go=edit&id=" + ToWSTR(ANIME.My_ID) + L"&hidenav=true", 
            buffer, L"", update_mode, reinterpret_cast<LPARAM>(&ANIME));
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
}

bool CMyAnimeList::UpdateSucceeded(const wstring& data, CEventItem& item) {
  int update_mode = item.Mode;
  bool success = false;
  
  switch (Settings.Account.MAL.API) {
    case MAL_API_OFFICIAL: {
      switch (update_mode) {
        case HTTP_MAL_AnimeAdd:
          success = IsNumeric(data);
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
          success = ToINT(data) == item.episode;
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
  switch (Settings.Account.MAL.API) {
    case MAL_API_OFFICIAL:
      item.Reason = data;
      break;
    case MAL_API_NONE:
      switch (update_mode) {
        case HTTP_MAL_AnimeDelete:
          item.Reason = InStr(data, L"class=\"badresult\">", L"</div>");
          if (item.Reason.empty()) item.Reason = data;
          break;
        default:
          item.Reason = data;
          break;
      }
  }
  Replace(item.Reason, L"</div><div>", L"\r\n");
  StripHTML(item.Reason);
  return false;
}

// =============================================================================

void CMyAnimeList::DecodeText(wstring& text) {
  // TODO: Remove when MAL fixes its encoding >_<
  #define HTMLCHARCOUNT 33
  static const wchar_t* html_chars[HTMLCHARCOUNT][2] = {
    /* Extreme measures */
    // black star (black and white stars are encoded the same in API >_<)
    {L"k&acirc;\uFFFD\uFFFDR", L"k\u2605R"},
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
    {L"&Acirc;&frac12;",        L"\u00BD"},   // fraction 1/2
    {L"&Atilde;&cent;",         L"\u00E2"},   // small a, circumflex accent
    {L"&Atilde;&curren;",       L"\u00E4"},   // small a, umlaut mark
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
  StripHTML(text);
  DecodeHTML(text);
}

bool CMyAnimeList::IsValidDate(const wstring& date) {
  return date.length() == 10 && !StartsWith(date, L"0000");
}

bool CMyAnimeList::IsValidEpisode(int episode, int watched, int total) {
  if (episode < 0) {
    return false;
  } else if (episode < watched) {
    return false;
  } else if (episode == watched && total != 1) {
    return false;
  } else if (episode > total && total != 0) {
    return false;
  } else {
    return true;
  }
}

void CMyAnimeList::ParseDateString(const wstring& date, unsigned short& year, unsigned short& month, unsigned short& day) {
  if (date.length() == 10) {
    year  = ToINT(date.substr(0, 4));
    month = ToINT(date.substr(5, 2));
    day   = ToINT(date.substr(8, 2));
  }
}

wstring CMyAnimeList::TranslateDate(wstring value, bool reverse) {
  if (!IsValidDate(value)) {
    return L"Unknown";
  } else {
    unsigned short year, month, day;
    ParseDateString(value, year, month, day);

    if (month == 0) {
      value = L"Unknown";
    } else if (month < 3) {
      value = L"Winter";
      year--;
    } else if (month < 6) {
      value = L"Spring";
    } else if (month < 9) {
      value = L"Summer";
    } else if (month < 11) {
      value = L"Fall";
    } else {
      value = L"Winter";
    }
    
    if (reverse) {
      value = ToWSTR(year) + L" " + value;
    } else {
      value = value + L" " + ToWSTR(year);
    }
    
    return value;
  }
}

wstring CMyAnimeList::TranslateMyStatus(int value, bool add_count) {
  #define ADD_COUNT() (add_count ? L" (" + ToWSTR(AnimeList.User.GetItemCount(value)) + L")" : L"")
  switch (value) {
    case MAL_NOTINLIST: return L"Not in list";
    case MAL_WATCHING: return L"Currently watching" + ADD_COUNT();
    case MAL_COMPLETED: return L"Completed" + ADD_COUNT();
    case MAL_ONHOLD: return L"On hold" + ADD_COUNT();
    case MAL_DROPPED: return L"Dropped" + ADD_COUNT();
    case MAL_PLANTOWATCH: return L"Plan to watch" + ADD_COUNT();
    default: return L"";
  }
  #undef ADD_COUNT
}
int CMyAnimeList::TranslateMyStatus(const wstring& value) {
  if (IsEqual(value, L"Currently watching")) {
    return MAL_WATCHING;
  } else if (IsEqual(value, L"Completed")) {
    return MAL_COMPLETED;
  } else if (IsEqual(value, L"On hold")) {
    return MAL_ONHOLD;
  } else if (IsEqual(value, L"Dropped")) {
    return MAL_DROPPED;
  } else if (IsEqual(value, L"Plan to watch")) {
    return MAL_PLANTOWATCH;
  } else {
    return 0;
  }
}

wstring CMyAnimeList::TranslateNumber(int value, LPCWSTR default_char) {
  return value == 0 ? default_char : ToWSTR(value);
}

wstring CMyAnimeList::TranslateRewatchValue(int value) {
  switch (value) {
    case MAL_REWATCH_VERYLOW: return L"Very low";
    case MAL_REWATCH_LOW: return L"Low";
    case MAL_REWATCH_MEDIUM: return L"Medium";
    case MAL_REWATCH_HIGH: return L"High";
    case MAL_REWATCH_VERYHIGH: return L"Very high";
    default: return L"";
  }
}

wstring CMyAnimeList::TranslateStatus(int value) {
  switch (value) {
    case MAL_AIRING: return L"Currently airing";
    case MAL_FINISHED: return L"Finished airing";
    case MAL_NOTYETAIRED: return L"Not yet aired";
    default: return ToWSTR(value);
  }
}
int CMyAnimeList::TranslateStatus(const wstring& value) {
  if (IsEqual(value, L"Currently airing")) {
    return MAL_AIRING;
  } else if (IsEqual(value, L"Finished airing")) {
    return MAL_FINISHED;
  } else if (IsEqual(value, L"Not yet aired")) {
    return MAL_NOTYETAIRED;
  } else {
    return 0;
  }
}

wstring CMyAnimeList::TranslateStorageType(int value) {
  switch (value) {
    case MAL_STORAGE_HARDDRIVE: return L"Hard drive";
    case MAL_STORAGE_DVDCD: return L"DVD/CD";
    case MAL_STORAGE_NONE: return L"None";
    case MAL_STORAGE_RETAILDVD: return L"Retail DVD";
    case MAL_STORAGE_VHS: return L"VHS";
    case MAL_STORAGE_EXTERNALHD: return L"External HD";
    case MAL_STORAGE_NAS: return L"NAS";
    default: return L"";
  }
}

wstring CMyAnimeList::TranslateType(int value) {
  switch (value) {
    case MAL_TV: return L"TV";
    case MAL_OVA: return L"OVA";
    case MAL_MOVIE: return L"Movie";
    case MAL_SPECIAL: return L"Special";
    case MAL_ONA: return L"ONA";
    case MAL_MUSIC: return L"Music";
    default: return L"";
  }
}
int CMyAnimeList::TranslateType(const wstring& value) {
  if (IsEqual(value, L"TV")) {
    return MAL_TV;
  } else if (IsEqual(value, L"OVA")) {
    return MAL_OVA;
  } else if (IsEqual(value, L"Movie")) {
    return MAL_MOVIE;
  } else if (IsEqual(value, L"Special")) {
    return MAL_SPECIAL;
  } else if (IsEqual(value, L"ONA")) {
    return MAL_ONA;
  } else if (IsEqual(value, L"Music")) {
    return MAL_MUSIC;
  } else {
    return 0;
  }
}

// =============================================================================

void CMyAnimeList::ViewAnimePage(int series_id) {
  ExecuteLink(L"http://myanimelist.net/anime/" + ToWSTR(series_id) + L"/");
}

void CMyAnimeList::ViewAnimeSearch(wstring title) {
  ExecuteLink(L"http://myanimelist.net/anime.php?q=" + title + L"&referer=" + APP_NAME);
}

void CMyAnimeList::ViewHistory() {
  ExecuteLink(L"http://myanimelist.net/history/" + Settings.Account.MAL.User);
}

void CMyAnimeList::ViewMessages() {
  ExecuteLink(L"http://myanimelist.net/mymessages.php");
}

void CMyAnimeList::ViewPanel() {
  ExecuteLink(L"http://myanimelist.net/panel.php");
}

void CMyAnimeList::ViewProfile() {
  ExecuteLink(L"http://myanimelist.net/profile/" + Settings.Account.MAL.User);
}