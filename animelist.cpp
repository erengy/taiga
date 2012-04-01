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
#include "animelist.h"
#include "event.h"
#include "myanimelist.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "xml.h"

class AnimeList AnimeList;

// =============================================================================

AnimeList::AnimeList() {
  Clear();
}

void AnimeList::Clear() {
  // Clear list
  index = 0;
  count = -1;
  items.clear();
  
  // Add default item
  AddItem(-4224, L"Toradora!", L"Tiger X Dragon",
    1, 25, 2, L"2008-10-01", L"2009-03-25",
    L"http://cdn.myanimelist.net/images/anime/5/22125.jpg",
    5741141, 26, L"0000-00-00", L"0000-00-00",
    10, 2, 0, 0, L"1238370665", L"comedy, romance, drama");
}

// =============================================================================

bool AnimeList::Load() {
  // Initialize
  Clear();
  user.Clear();
  if (Settings.Account.MAL.user.empty()) return false;
  wstring file = Taiga.GetDataPath() + Settings.Account.MAL.user + L".xml";
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok && result.status != status_file_not_found) {
    MessageBox(NULL, L"Could not read anime list.", file.c_str(), MB_OK | MB_ICONERROR);
    return false;
  }

  // Read user info
  xml_node myanimelist = doc.child(L"myanimelist");
  xml_node myinfo = myanimelist.child(L"myinfo");
  user.id   = XML_ReadIntValue(myinfo, L"user_id");
  user.name = XML_ReadStrValue(myinfo, L"user_name");
  // Since MAL can be too slow to update these values, 
  // we'll be counting by ourselves at AnimeList::AddItem
  /*
  user.watching      = XML_ReadIntValue(myinfo, L"user_watching");
  user.completed     = XML_ReadIntValue(myinfo, L"user_completed");
  user.on_hold       = XML_ReadIntValue(myinfo, L"user_onhold");
  user.dropped       = XML_ReadIntValue(myinfo, L"user_dropped");
  user.plan_to_watch = XML_ReadIntValue(myinfo, L"user_plantowatch");
  */
  user.days_spent_watching = XML_ReadStrValue(myinfo, L"user_days_spent_watching");

  // Read anime list
  int anime_count = 0;
  for (xml_node node = myanimelist.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    anime_count++;
  }
  if (anime_count) {
    items.resize(anime_count + 1);
  }
  for (xml_node node = myanimelist.child(L"anime"); node; node = node.next_sibling(L"anime")) {
    AddItem(
      XML_ReadIntValue(node, L"series_animedb_id"),
      XML_ReadStrValue(node, L"series_title"),
      XML_ReadStrValue(node, L"series_synonyms"),
      XML_ReadIntValue(node, L"series_type"),
      XML_ReadIntValue(node, L"series_episodes"),
      XML_ReadIntValue(node, L"series_status"),
      XML_ReadStrValue(node, L"series_start"),
      XML_ReadStrValue(node, L"series_end"),
      XML_ReadStrValue(node, L"series_image"),
      XML_ReadIntValue(node, L"my_id"),
      XML_ReadIntValue(node, L"my_watched_episodes"),
      XML_ReadStrValue(node, L"my_start_date"),
      XML_ReadStrValue(node, L"my_finish_date"),
      XML_ReadIntValue(node, L"my_score"),
      XML_ReadIntValue(node, L"my_status"),
      XML_ReadIntValue(node, L"my_rewatching"),
      XML_ReadIntValue(node, L"my_rewatching_ep"),
      XML_ReadStrValue(node, L"my_last_updated"),
      XML_ReadStrValue(node, L"my_tags"),
      anime_count == 0);
  }

  return true;
}

bool AnimeList::Save(int anime_id, wstring child, wstring value, int mode) {
  Anime* anime = FindItem(anime_id);

  if (mode != ANIMELIST_EDITUSER && !anime) {
    return false;
  }
  
  // Initialize
  wstring file = Taiga.GetDataPath() + Settings.Account.MAL.user + L".xml";
  
  // Load XML file
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
        if (XML_ReadIntValue(node, L"series_animedb_id") == anime->series_id) {
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
      XML_WriteIntValue(node, L"series_animedb_id", anime->series_id);
      XML_WriteStrValue(node, L"series_title", anime->series_title.c_str(), node_cdata);
      XML_WriteStrValue(node, L"series_synonyms", anime->series_synonyms.c_str(), node_cdata);
      XML_WriteIntValue(node, L"series_type", anime->series_type);
      XML_WriteIntValue(node, L"series_episodes", anime->series_episodes);
      XML_WriteIntValue(node, L"series_status", anime->series_status);
      XML_WriteStrValue(node, L"series_start", anime->series_start.c_str());
      XML_WriteStrValue(node, L"series_end", anime->series_end.c_str());
      XML_WriteStrValue(node, L"series_image", anime->series_image.c_str());
      XML_WriteIntValue(node, L"my_id", anime->my_id);
      XML_WriteIntValue(node, L"my_watched_episodes", anime->my_watched_episodes);
      XML_WriteStrValue(node, L"my_start_date", anime->my_start_date.c_str());
      XML_WriteStrValue(node, L"my_finish_date", anime->my_finish_date.c_str());
      XML_WriteIntValue(node, L"my_score", anime->my_score);
      XML_WriteIntValue(node, L"my_status", anime->my_status);
      XML_WriteIntValue(node, L"my_rewatching", anime->my_rewatching);
      XML_WriteIntValue(node, L"my_rewatching_ep", anime->my_rewatching_ep);
      XML_WriteStrValue(node, L"my_last_updated", anime->my_last_updated.c_str());
      XML_WriteStrValue(node, L"my_tags", anime->my_tags.c_str());
      doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
      return true;
    }
    // Delete anime item
    case ANIMELIST_DELETEANIME: {
      for (xml_node node = myanimelist.child(L"anime"); node; node = node.next_sibling(L"anime")) {
        if (XML_ReadIntValue(node, L"series_animedb_id") == anime->series_id) {
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

void AnimeList::AddItem(Anime& anime) {
  AddItem(
    anime.series_id,
    anime.series_title,
    anime.series_synonyms,
    anime.series_type,
    anime.series_episodes,
    anime.series_status,
    anime.series_start,
    anime.series_end,
    anime.series_image,
    anime.my_id,
    anime.my_watched_episodes,
    anime.my_start_date,
    anime.my_finish_date,
    anime.my_score,
    anime.my_status,
    anime.my_rewatching,
    anime.my_rewatching_ep,
    anime.my_last_updated,
    anime.my_tags);
}

void AnimeList::AddItem(
  int series_id, 
  const wstring& series_title, 
  const wstring& series_synonyms, 
  int series_type, 
  int series_episodes, 
  int series_status, 
  const wstring& series_start, 
  const wstring& series_end, 
  const wstring& series_image, 
  int my_id, 
  int my_watched_episodes, 
  const wstring& my_start_date, 
  const wstring& my_finish_date, 
  int my_score, 
  int my_status, 
  int my_rewatching, 
  int my_rewatching_ep, 
  const wstring& my_last_updated, 
  const wstring& my_tags, 
  bool resize)
{
  if (resize) {
    items.resize(++count + 1);
  } else {
    count++;
  }

  items[count].series_id           = series_id;
  items[count].series_title        = series_title;
  items[count].series_synonyms     = series_synonyms;
  items[count].series_type         = series_type;
  items[count].series_episodes     = series_episodes;
  items[count].series_status       = series_status;
  items[count].series_start        = series_start;
  items[count].series_end          = series_end;
  items[count].series_image        = series_image;
  items[count].my_id               = my_id;
  items[count].my_watched_episodes = my_watched_episodes;
  items[count].my_start_date       = my_start_date;
  items[count].my_finish_date      = my_finish_date;
  items[count].my_score            = my_score;
  items[count].my_status           = my_status;
  items[count].my_rewatching       = my_rewatching;
  items[count].my_rewatching_ep    = my_rewatching_ep;
  items[count].my_last_updated     = my_last_updated;
  items[count].my_tags             = my_tags;
  
  items[count].index = count;
  if (items[count].series_episodes) {
    items[count].episode_available.resize(items[count].series_episodes);
  }
  if (count <= 0) return;

  DecodeHTML(items[count].series_title);
  DecodeHTML(items[count].series_synonyms);

  user.IncreaseItemCount(my_status, 1, false);

  for (size_t i = 0; i < Settings.Anime.items.size(); i++) {
    if (items[count].series_id == Settings.Anime.items[i].id) {
      items[count].folder = Settings.Anime.items[i].folder;
      items[count].synonyms = Settings.Anime.items[i].titles;
      break;
    }
  }
}

bool AnimeList::DeleteItem(int anime_id) {
  Anime* anime = FindItem(anime_id);
  if (!anime) return false;

  user.IncreaseItemCount(anime->my_status, -1);
  Save(anime_id, L"", L"", ANIMELIST_DELETEANIME);
  
  count--;
  items.erase(items.begin() + anime->index);
  for (int i = 1; i <= count; i++) {
    items[i].index = i;
  }

  return true;
}

Anime* AnimeList::FindItem(int anime_id) {
  if (anime_id)
    for (int i = 0; i <= count; i++)
      if (items[i].series_id == anime_id)
        return &items[i];
  return nullptr;
}

int AnimeList::FindItemIndex(int anime_id) {
  if (anime_id)
    for (int i = 0; i <= count; i++)
      if (items[i].series_id == anime_id)
        return i;
  return -1;
}

// =============================================================================

AnimeList::ListFilters::ListFilters() {
  Reset();
}

bool AnimeList::ListFilters::Check(Anime& anime) {
  // Filter my status
  for (int i = 0; i < 6; i++) {
    if (!my_status[i] && anime.GetStatus() == i + 1) {
      return false;
    }
  }
  // Filter status
  for (int i = 0; i < 3; i++) {
    if (!status[i] && anime.GetAiringStatus() == i + 1) {
      return false;
    }
  }
  // Filter type
  for (int i = 0; i < 6; i++) {
    if (!type[i] && anime.series_type == i + 1) {
      return false;
    }
  }
  // Filter new episodes
  if (new_episodes && !anime.new_episode_available) {
    return false;
  }
  // Filter text
  vector<wstring> words;
  Split(text, L" ", words);
  for (auto it = words.begin(); it != words.end(); ++it) {
    if (InStr(anime.series_title,    *it, 0, true) == -1 && 
        InStr(anime.series_synonyms, *it, 0, true) == -1 && 
        InStr(anime.synonyms,        *it, 0, true) == -1 && 
        InStr(anime.GetTags(),       *it, 0, true) == -1) {
          return false;
    }
  }

  return true;
}

void AnimeList::ListFilters::Reset() {
  for (int i = 0; i < 6; i++) my_status[i] = true;
  for (int i = 0; i < 3; i++) status[i] = true;
  for (int i = 0; i < 6; i++) type[i] = true;
  new_episodes = false;
  text = L"";
}

// =============================================================================

User::User() {
  Clear();
}

void User::Clear() {
  id = 0;
  watching = 0;
  completed = 0;
  on_hold = 0;
  dropped = 0;
  plan_to_watch = 0;
  name = L"";
  days_spent_watching = L"";
}

int User::GetItemCount(int status) {
  int count = 0;
  
  // Get current count
  switch (status) {
    case MAL_WATCHING:
      count = watching;
      break;
    case MAL_COMPLETED:
      count = completed;
      break;
    case MAL_ONHOLD:
      count = on_hold;
      break;
    case MAL_DROPPED:
      count = dropped;
      break;
    case MAL_PLANTOWATCH:
      count = plan_to_watch;
      break;
  }

  // Search event queue for status changes
  EventList* event_list = EventQueue.FindList();
  if (event_list) {
    for (auto it = event_list->items.begin(); it != event_list->items.end(); ++it) {
      if (it->mode == HTTP_MAL_AnimeAdd) continue;
      if (it->status > -1) {
        if (status == it->status) {
          count++;
        } else {
          Anime* anime = AnimeList.FindItem(it->anime_id);
          if (anime && status == anime->my_status) {
            count--;
          }
        }
      }
    }
  }

  // Return final value
  return count;
}

void User::IncreaseItemCount(int status, int count, bool write) {
  switch (status) {
    case MAL_WATCHING:
      watching += count;
      if (write) AnimeList.Save(-1, L"user_watching", ToWSTR(watching), ANIMELIST_EDITUSER);
      break;
    case MAL_COMPLETED:
      completed += count;
      if (write) AnimeList.Save(-1, L"user_completed", ToWSTR(completed), ANIMELIST_EDITUSER);
      break;
    case MAL_ONHOLD:
      on_hold += count;
      if (write) AnimeList.Save(-1, L"user_onhold", ToWSTR(on_hold), ANIMELIST_EDITUSER);
      break;
    case MAL_DROPPED:
      dropped += count;
      if (write) AnimeList.Save(-1, L"user_dropped", ToWSTR(dropped), ANIMELIST_EDITUSER);
      break;
    case MAL_PLANTOWATCH:
      plan_to_watch += count;
      if (write) AnimeList.Save(-1, L"user_plantowatch", ToWSTR(plan_to_watch), ANIMELIST_EDITUSER);
      break;
  }
}