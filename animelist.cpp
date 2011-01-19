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
  Item.clear();
  
  // Add default item
  AddItem(4224, L"Toradora!", L"Tiger X Dragon",
    1, 25, 2, L"2008-10-01", L"2009-03-25",
    L"http://cdn.myanimelist.net/images/anime/5/22125.jpg",
    5741141, 26, L"0000-00-00", L"0000-00-00",
    10, 2, 0, 0, L"1238370665", L"comedy, romance, drama");
}

// =============================================================================

BOOL CAnimeList::Read() {
  // Initialize
  Clear();
  User.Clear();
  wstring file = Taiga.GetDataPath() + Settings.Account.MAL.User + L".xml";
  
  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok && result.status != status_file_not_found) {
    MessageBox(NULL, L"Could not read anime list.", file.c_str(), MB_OK | MB_ICONERROR);
    return FALSE;
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

  return TRUE;
}

BOOL CAnimeList::Write(int index, wstring child, wstring value, int mode) {
  // Initialize
  wstring file = Taiga.GetDataPath() + Settings.Account.MAL.User + L".xml";
  
  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok) return FALSE;

  // Read anime list
  xml_node myanimelist = doc.child(L"myanimelist");
  switch (mode) {
    // Edit user data
    case ANIMELIST_EDITUSER: {
      myanimelist.child(L"myinfo").child(child.c_str()).first_child().set_value(value.c_str());
      doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
      return TRUE;
    }
    // Edit anime data
    case ANIMELIST_EDITANIME: {
      for (xml_node anime = myanimelist.child(L"anime"); anime; anime = anime.next_sibling(L"anime")) {
        if (XML_ReadIntValue(anime, L"series_animedb_id") == Item[index].Series_ID) {
          anime.child(child.c_str()).first_child().set_value(value.c_str());
          doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
          return TRUE;
        }
      }
      break;
    }
    // Add anime item
    case ANIMELIST_ADDANIME: {
      xml_node anime = myanimelist.append_child();
      anime.set_name(L"anime");
      XML_WriteIntValue(anime, L"series_animedb_id", Item[index].Series_ID);
      XML_WriteStrValue(anime, L"series_title", Item[index].Series_Title.c_str(), node_cdata);
      XML_WriteStrValue(anime, L"series_synonyms", Item[index].Series_Synonyms.c_str(), node_cdata);
      XML_WriteIntValue(anime, L"series_type", Item[index].Series_Type);
      XML_WriteIntValue(anime, L"series_episodes", Item[index].Series_Episodes);
      XML_WriteIntValue(anime, L"series_status", Item[index].Series_Status);
      XML_WriteStrValue(anime, L"series_start", Item[index].Series_Start.c_str());
      XML_WriteStrValue(anime, L"series_end", Item[index].Series_End.c_str());
      XML_WriteStrValue(anime, L"series_image", Item[index].Series_Image.c_str());
      XML_WriteIntValue(anime, L"my_id", Item[index].My_ID);
      XML_WriteIntValue(anime, L"my_watched_episodes", Item[index].My_WatchedEpisodes);
      XML_WriteStrValue(anime, L"my_start_date", Item[index].My_StartDate.c_str());
      XML_WriteStrValue(anime, L"my_finish_date", Item[index].My_FinishDate.c_str());
      XML_WriteIntValue(anime, L"my_score", Item[index].My_Score);
      XML_WriteIntValue(anime, L"my_status", Item[index].My_Status);
      XML_WriteIntValue(anime, L"my_rewatching", Item[index].My_Rewatching);
      XML_WriteIntValue(anime, L"my_rewatching_ep", Item[index].My_RewatchingEP);
      XML_WriteStrValue(anime, L"my_last_updated", Item[index].My_LastUpdated.c_str());
      XML_WriteStrValue(anime, L"my_tags", Item[index].My_Tags.c_str());
      doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
      return TRUE;
    }
    // Delete anime item
    case ANIMELIST_DELETEANIME: {
      for (xml_node anime = myanimelist.child(L"anime"); anime; anime = anime.next_sibling(L"anime")) {
        if (XML_ReadIntValue(anime, L"series_animedb_id") == Item[index].Series_ID) {
          myanimelist.remove_child(anime);
          doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
          return TRUE;
        }
      }
      break;
    }
  }

  return FALSE;
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
  Item.resize(++Count + 1);
  Item[Count].Series_ID          = series_id;
  Item[Count].Series_Title       = series_title;
  Item[Count].Series_Synonyms    = series_synonyms;
  Item[Count].Series_Type        = series_type;
  Item[Count].Series_Episodes    = series_episodes;
  Item[Count].Series_Status      = series_status;
  Item[Count].Series_Start       = series_start;
  Item[Count].Series_End         = series_end;
  Item[Count].Series_Image       = series_image;
  Item[Count].My_ID              = my_id;
  Item[Count].My_WatchedEpisodes = my_watched_episodes;
  Item[Count].My_StartDate       = my_start_date;
  Item[Count].My_FinishDate      = my_finish_date;
  Item[Count].My_Score           = my_score;
  Item[Count].My_Status          = my_status;
  Item[Count].My_Rewatching      = my_rewatching;
  Item[Count].My_RewatchingEP    = my_rewatching_ep;
  Item[Count].My_LastUpdated     = my_last_updated;
  Item[Count].My_Tags            = my_tags;
  
  Item[Count].Index = Count;
  if (Item[Count].Series_Episodes) {
    Item[Count].EpisodeAvailable.resize(Item[Count].Series_Episodes);
  }
  if (Count <= 0) return;

  DecodeHTML(Item[Count].Series_Title);
  DecodeHTML(Item[Count].Series_Synonyms);

  User.IncreaseItemCount(my_status, 1, false);

  for (size_t i = 0; i < Settings.Anime.Item.size(); i++) {
    if (Item[Count].Series_ID == Settings.Anime.Item[i].ID) {
      Item[Count].FansubGroup = Settings.Anime.Item[i].FansubGroup;
      Item[Count].Folder = Settings.Anime.Item[i].Folder;
      Item[Count].Synonyms = Settings.Anime.Item[i].Titles;
      break;
    }
  }
}

void CAnimeList::DeleteItem(int index) {
  if (index <= 0 || index > Count) return;

  User.IncreaseItemCount(Item[index].My_Status, -1);
  Write(index, L"", L"", ANIMELIST_DELETEANIME);
  
  Count--;
  Item.erase(Item.begin() + index);
  for (int i = 1; i <= Count; i++) {
    Item[i].Index = i;
  }
}

int CAnimeList::FindItemByID(int anime_id) {
  if (anime_id)
    for (int i = 1; i <= Count; i++)
      if (Item[i].Series_ID == anime_id)
        return i;
  return -1;
}

// =============================================================================

CAnimeList::CFilter::CFilter() {
  Reset();
}

BOOL CAnimeList::CFilter::Check(int item_index) {
  // Filter my status
  for (int i = 0; i < 6; i++) {
    if (!MyStatus[i] && AnimeList.Item[item_index].GetStatus() == i + 1) {
      return FALSE;
    }
  }
  // Filter status
  for (int i = 0; i < 3; i++) {
    if (!Status[i] && AnimeList.Item[item_index].Series_Status == i + 1) {
      return FALSE;
    }
  }
  // Filter type
  for (int i = 0; i < 6; i++) {
    if (!Type[i] && AnimeList.Item[item_index].Series_Type == i + 1) {
      return FALSE;
    }
  }
  // Filter new episodes
  if (NewEps && !AnimeList.Item[item_index].NewEps) {
    return FALSE;
  }
  // Filter text
  vector<wstring> text_vector;
  Split(Text, L" ", text_vector);
  for (size_t i = 0; i < text_vector.size(); i++) {
    if (InStr(AnimeList.Item[item_index].Series_Title,  text_vector[i], 0, true) == -1 && 
      InStr(AnimeList.Item[item_index].Series_Synonyms, text_vector[i], 0, true) == -1 && 
      InStr(AnimeList.Item[item_index].Synonyms,        text_vector[i], 0, true) == -1 && 
      InStr(AnimeList.Item[item_index].GetTags(),       text_vector[i], 0, true) == -1) {
        return FALSE;
    }
  }

  return TRUE;
}

void CAnimeList::CFilter::Reset() {
  for (int i = 0; i < 6; i++) MyStatus[i] = TRUE;
  for (int i = 0; i < 3; i++) Status[i] = TRUE;
  for (int i = 0; i < 6; i++) Type[i] = TRUE;
  NewEps = FALSE;
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
    #define ITEM EventQueue.List[user_index].Item
    for (unsigned int i = 0; i < ITEM.size(); i++) {
      if (ITEM[i].status > -1) {
        if (status == ITEM[i].status) {
          count++;
        } else if (status == AnimeList.Item[ITEM[i].AnimeIndex].My_Status) {
          count--;
        }
      }
    }
    #undef ITEM
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