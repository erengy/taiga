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
#include "event.h"
#include "myanimelist.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "xml.h"

CAnimeList AnimeList;

// =============================================================================

CAnimeList::CAnimeList() {
  Clear();
}

void CAnimeList::Clear() {
  // Clear list
  Index = 0;
  Count = -1;
  Items.clear();
  
  // Add default item
  AddItem(-4224, L"Toradora!", L"Tiger X Dragon",
    1, 25, 2, L"2008-10-01", L"2009-03-25",
    L"http://cdn.myanimelist.net/images/anime/5/22125.jpg",
    5741141, 26, L"0000-00-00", L"0000-00-00",
    10, 2, 0, 0, L"1238370665", L"comedy, romance, drama");
}

// =============================================================================

bool CAnimeList::Read() {
  // Initialize
  Clear();
  User.Clear();
  if (Settings.Account.MAL.User.empty()) return FALSE;
  wstring file = Taiga.GetDataPath() + Settings.Account.MAL.User + L".xml";
  
  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok && result.status != status_file_not_found) {
    MessageBox(NULL, L"Could not read anime list.", file.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Read user info
  xml_node myanimelist = doc.child(L"myanimelist");
  xml_node myinfo = myanimelist.child(L"myinfo");
    User.ID          = XML_ReadIntValue(myinfo, L"user_id");
    User.Name        = XML_ReadStrValue(myinfo, L"user_name");
    // Since MAL is too slow to update these values, 
    // we'll be counting by ourselves at CAnimeList::AddItem
    /*
    User.Watching    = XML_ReadIntValue(myinfo, L"user_watching");
    User.Completed   = XML_ReadIntValue(myinfo, L"user_completed");
    User.OnHold      = XML_ReadIntValue(myinfo, L"user_onhold");
    User.Dropped     = XML_ReadIntValue(myinfo, L"user_dropped");
    User.PlanToWatch = XML_ReadIntValue(myinfo, L"user_plantowatch");
    */
    User.DaysSpent   = XML_ReadStrValue(myinfo, L"user_days_spent_watching");

  // Read anime list
  for (xml_node anime = myanimelist.child(L"anime"); anime; anime = anime.next_sibling(L"anime")) {
    AddItem(XML_ReadIntValue(anime, L"series_animedb_id"),
      XML_ReadStrValue(anime, L"series_title"),
      XML_ReadStrValue(anime, L"series_synonyms"),
      XML_ReadIntValue(anime, L"series_type"),
      XML_ReadIntValue(anime, L"series_episodes"),
      XML_ReadIntValue(anime, L"series_status"),
      XML_ReadStrValue(anime, L"series_start"),
      XML_ReadStrValue(anime, L"series_end"),
      XML_ReadStrValue(anime, L"series_image"),
      XML_ReadIntValue(anime, L"my_id"),
      XML_ReadIntValue(anime, L"my_watched_episodes"),
      XML_ReadStrValue(anime, L"my_start_date"),
      XML_ReadStrValue(anime, L"my_finish_date"),
      XML_ReadIntValue(anime, L"my_score"),
      XML_ReadIntValue(anime, L"my_status"),
      XML_ReadIntValue(anime, L"my_rewatching"),
      XML_ReadIntValue(anime, L"my_rewatching_ep"),
      XML_ReadStrValue(anime, L"my_last_updated"),
      XML_ReadStrValue(anime, L"my_tags"));
  }

  return true;
}

bool CAnimeList::Write(int anime_id, wstring child, wstring value, int mode) {
  CAnime* anime = FindItem(anime_id);

  if (mode != ANIMELIST_EDITUSER && !anime) {
    return false;
  }
  
  // Initialize
  wstring file = Taiga.GetDataPath() + Settings.Account.MAL.User + L".xml";
  
  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok) return false;

  // Read anime list
  xml_node myanimelist = doc.child(L"myanimelist");
  switch (mode) {
    // Edit user data
    case ANIMELIST_EDITUSER: {
      myanimelist.child(L"myinfo").child(child.c_str()).first_child().set_value(value.c_str());
      doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
      return true;
    }
    // Edit anime data
    case ANIMELIST_EDITANIME: {
      for (xml_node node = myanimelist.child(L"anime"); node; node = node.next_sibling(L"anime")) {
        if (XML_ReadIntValue(node, L"series_animedb_id") == anime->Series_ID) {
          node.child(child.c_str()).first_child().set_value(value.c_str());
          doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
          return true;
        }
      }
      break;
    }
    // Add anime item
    case ANIMELIST_ADDANIME: {
      xml_node node = myanimelist.append_child();
      node.set_name(L"anime");
      XML_WriteIntValue(node, L"series_animedb_id", anime->Series_ID);
      XML_WriteStrValue(node, L"series_title", anime->Series_Title.c_str(), node_cdata);
      XML_WriteStrValue(node, L"series_synonyms", anime->Series_Synonyms.c_str(), node_cdata);
      XML_WriteIntValue(node, L"series_type", anime->Series_Type);
      XML_WriteIntValue(node, L"series_episodes", anime->Series_Episodes);
      XML_WriteIntValue(node, L"series_status", anime->Series_Status);
      XML_WriteStrValue(node, L"series_start", anime->Series_Start.c_str());
      XML_WriteStrValue(node, L"series_end", anime->Series_End.c_str());
      XML_WriteStrValue(node, L"series_image", anime->Series_Image.c_str());
      XML_WriteIntValue(node, L"my_id", anime->My_ID);
      XML_WriteIntValue(node, L"my_watched_episodes", anime->My_WatchedEpisodes);
      XML_WriteStrValue(node, L"my_start_date", anime->My_StartDate.c_str());
      XML_WriteStrValue(node, L"my_finish_date", anime->My_FinishDate.c_str());
      XML_WriteIntValue(node, L"my_score", anime->My_Score);
      XML_WriteIntValue(node, L"my_status", anime->My_Status);
      XML_WriteIntValue(node, L"my_rewatching", anime->My_Rewatching);
      XML_WriteIntValue(node, L"my_rewatching_ep", anime->My_RewatchingEP);
      XML_WriteStrValue(node, L"my_last_updated", anime->My_LastUpdated.c_str());
      XML_WriteStrValue(node, L"my_tags", anime->My_Tags.c_str());
      doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
      return true;
    }
    // Delete anime item
    case ANIMELIST_DELETEANIME: {
      for (xml_node node = myanimelist.child(L"anime"); node; node = node.next_sibling(L"anime")) {
        if (XML_ReadIntValue(node, L"series_animedb_id") == anime->Series_ID) {
          myanimelist.remove_child(node);
          doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
          return true;
        }
      }
      break;
    }
  }

  return false;
}

// =============================================================================

void CAnimeList::AddItem(CAnime& anime_item) {
  AddItem(anime_item.Series_ID,
    anime_item.Series_Title,
    anime_item.Series_Synonyms,
    anime_item.Series_Type,
    anime_item.Series_Episodes,
    anime_item.Series_Status,
    anime_item.Series_Start,
    anime_item.Series_End,
    anime_item.Series_Image,
    anime_item.My_ID,
    anime_item.My_WatchedEpisodes,
    anime_item.My_StartDate,
    anime_item.My_FinishDate,
    anime_item.My_Score,
    anime_item.My_Status,
    anime_item.My_Rewatching,
    anime_item.My_RewatchingEP,
    anime_item.My_LastUpdated,
    anime_item.My_Tags);
}

void CAnimeList::AddItem(
  int series_id, 
  wstring series_title, 
  wstring series_synonyms, 
  int series_type, 
  int series_episodes, 
  int series_status, 
  wstring series_start, 
  wstring series_end, 
  wstring series_image, 
  int my_id, 
  int my_watched_episodes, 
  wstring my_start_date, 
  wstring my_finish_date, 
  int my_score, 
  int my_status, 
  int my_rewatching, 
  int my_rewatching_ep, 
  wstring my_last_updated, 
  wstring my_tags)
{
  Items.resize(++Count + 1);
  Items[Count].Series_ID          = series_id;
  Items[Count].Series_Title       = series_title;
  Items[Count].Series_Synonyms    = series_synonyms;
  Items[Count].Series_Type        = series_type;
  Items[Count].Series_Episodes    = series_episodes;
  Items[Count].Series_Status      = series_status;
  Items[Count].Series_Start       = series_start;
  Items[Count].Series_End         = series_end;
  Items[Count].Series_Image       = series_image;
  Items[Count].My_ID              = my_id;
  Items[Count].My_WatchedEpisodes = my_watched_episodes;
  Items[Count].My_StartDate       = my_start_date;
  Items[Count].My_FinishDate      = my_finish_date;
  Items[Count].My_Score           = my_score;
  Items[Count].My_Status          = my_status;
  Items[Count].My_Rewatching      = my_rewatching;
  Items[Count].My_RewatchingEP    = my_rewatching_ep;
  Items[Count].My_LastUpdated     = my_last_updated;
  Items[Count].My_Tags            = my_tags;
  
  Items[Count].Index = Count;
  if (Items[Count].Series_Episodes) {
    Items[Count].EpisodeAvailable.resize(Items[Count].Series_Episodes);
  }
  if (Count <= 0) return;

  DecodeHTML(Items[Count].Series_Title);
  DecodeHTML(Items[Count].Series_Synonyms);

  User.IncreaseItemCount(my_status, 1, false);

  for (size_t i = 0; i < Settings.Anime.Items.size(); i++) {
    if (Items[Count].Series_ID == Settings.Anime.Items[i].ID) {
      Items[Count].Folder = Settings.Anime.Items[i].Folder;
      Items[Count].Synonyms = Settings.Anime.Items[i].Titles;
      break;
    }
  }
}

bool CAnimeList::DeleteItem(int anime_id) {
  CAnime* anime = FindItem(anime_id);
  if (!anime) return false;

  User.IncreaseItemCount(anime->My_Status, -1);
  Write(anime_id, L"", L"", ANIMELIST_DELETEANIME);
  
  Count--;
  Items.erase(Items.begin() + anime->Index);
  for (int i = 1; i <= Count; i++) {
    Items[i].Index = i;
  }

  return true;
}

CAnime* CAnimeList::FindItem(int anime_id) {
  if (anime_id)
    for (int i = 0; i <= Count; i++)
      if (Items[i].Series_ID == anime_id)
        return &Items[i];
  return nullptr;
}

int CAnimeList::FindItemIndex(int anime_id) {
  if (anime_id)
    for (int i = 0; i <= Count; i++)
      if (Items[i].Series_ID == anime_id)
        return i;
  return -1;
}

// =============================================================================

CAnimeList::CFilter::CFilter() {
  Reset();
}

bool CAnimeList::CFilter::Check(CAnime& anime) {
  // Filter my status
  for (int i = 0; i < 6; i++) {
    if (!MyStatus[i] && anime.GetStatus() == i + 1) {
      return false;
    }
  }
  // Filter status
  for (int i = 0; i < 3; i++) {
    if (!Status[i] && anime.GetAiringStatus() == i + 1) {
      return false;
    }
  }
  // Filter type
  for (int i = 0; i < 6; i++) {
    if (!Type[i] && anime.Series_Type == i + 1) {
      return false;
    }
  }
  // Filter new episodes
  if (NewEps && !anime.NewEps) {
    return false;
  }
  // Filter text
  vector<wstring> text_vector;
  Split(Text, L" ", text_vector);
  for (auto it = text_vector.begin(); it != text_vector.end(); ++it) {
    if (InStr(anime.Series_Title,  *it, 0, true) == -1 && 
      InStr(anime.Series_Synonyms, *it, 0, true) == -1 && 
      InStr(anime.Synonyms,        *it, 0, true) == -1 && 
      InStr(anime.GetTags(),       *it, 0, true) == -1) {
        return false;
    }
  }

  return true;
}

void CAnimeList::CFilter::Reset() {
  for (int i = 0; i < 6; i++) MyStatus[i] = true;
  for (int i = 0; i < 3; i++) Status[i] = true;
  for (int i = 0; i < 6; i++) Type[i] = true;
  NewEps = false;
  Text = L"";
}

// =============================================================================

CUser::CUser() {
  Clear();
}

void CUser::Clear() {
  ID          = 0;
  Watching    = 0;
  Completed   = 0;
  OnHold      = 0;
  Dropped     = 0;
  PlanToWatch = 0;
  Name        = L"";
  DaysSpent   = L"";
}

int CUser::GetItemCount(int status) {
  int count = 0;
  
  // Get current count
  switch (status) {
    case MAL_WATCHING:
      count = Watching;
      break;
    case MAL_COMPLETED:
      count = Completed;
      break;
    case MAL_ONHOLD:
      count = OnHold;
      break;
    case MAL_DROPPED:
      count = Dropped;
      break;
    case MAL_PLANTOWATCH:
      count = PlanToWatch;
      break;
  }

  // Search event queue for status changes
  int user_index = EventQueue.GetUserIndex();
  if (user_index > -1) {
    for (auto it = EventQueue.List[user_index].Items.begin(); 
      it != EventQueue.List[user_index].Items.end(); ++it) {
        if (it->Mode == HTTP_MAL_AnimeAdd) continue;
        if (it->status > -1) {
          if (status == it->status) {
            count++;
          } else {
            CAnime* anime = AnimeList.FindItem(it->AnimeId);
            if (anime && status == anime->My_Status) {
              count--;
            }
          }
        }
    }
  }

  // Return final value
  return count;
}

void CUser::IncreaseItemCount(int status, int count, bool write) {
  switch (status) {
    case MAL_WATCHING:
      Watching += count;
      if (write) AnimeList.Write(-1, L"user_watching", ToWSTR(Watching), ANIMELIST_EDITUSER);
      break;
    case MAL_COMPLETED:
      Completed += count;
      if (write) AnimeList.Write(-1, L"user_completed", ToWSTR(Completed), ANIMELIST_EDITUSER);
      break;
    case MAL_ONHOLD:
      OnHold += count;
      if (write) AnimeList.Write(-1, L"user_onhold", ToWSTR(OnHold), ANIMELIST_EDITUSER);
      break;
    case MAL_DROPPED:
      Dropped += count;
      if (write) AnimeList.Write(-1, L"user_dropped", ToWSTR(Dropped), ANIMELIST_EDITUSER);
      break;
    case MAL_PLANTOWATCH:
      PlanToWatch += count;
      if (write) AnimeList.Write(-1, L"user_plantowatch", ToWSTR(PlanToWatch), ANIMELIST_EDITUSER);
      break;
  }
}