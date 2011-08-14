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

#ifndef ANIME_H
#define ANIME_H

#include "std.h"

enum AnimeId {
  ANIMEID_NOTINLIST = -1,
  ANIMEID_UNKNOWN = 0
};

class EventItem;

// =============================================================================

class Episode {
public:
  Episode();
  virtual ~Episode() {}

  void Clear();

public:
  int anime_id;
  wstring file, folder, format, title, name, group, number, version, 
    resolution, audio_type, video_type, checksum, extras;
};

extern Episode CurrentEpisode;

// =============================================================================

class MalAnime {
public:
  MalAnime();
  virtual ~MalAnime() {}

public:
  int series_id, series_type, series_episodes, series_status,
    my_id, my_watched_episodes, my_score, my_status, my_rewatching, my_rewatching_ep;
  wstring series_title, series_synonyms, series_start, series_end, series_image,
    my_start_date, my_finish_date, my_last_updated, my_tags;
};

class Anime : public MalAnime {
public:
  Anime();
  virtual ~Anime() {}

  int Ask(Episode episode);
  void End(Episode episode, bool end_watching, bool update_list);
  void Start(Episode episode);
  void Update(Episode episode, bool change_status);
  
  bool CheckFolder();
  void SetFolder(const wstring& folder, bool save_settings, bool check_episodes);
  
  bool CheckEpisodes(int number = -1, bool check_folder = false);
  bool IsEpisodeAvailable(int number);
  bool PlayEpisode(int number);
  bool SetEpisodeAvailability(int number, bool available, const wstring& path = L"");
  
  wstring GetImagePath();
  void SetLocalData(const wstring& folder, const wstring& titles);
  
  int GetIntValue(int mode) const;
  wstring GetStrValue(int mode) const;

  int GetLastWatchedEpisode() const;
  int GetRewatching() const;
  int GetScore() const;
  int GetStatus() const;
  wstring GetTags() const;
  int GetTotalEpisodes() const;
  
  int GetAiringStatus();
  bool IsAiredYet(bool strict = false) const;
  bool IsFinishedAiring() const;
  void SetFinishDate(const wstring& date, bool ignore_previous);
  void SetStartDate(const wstring& date, bool ignore_previous);

  bool Edit(EventItem& item, const wstring& data, int status_code);
  
  bool GetFansubFilter(vector<wstring>& groups);
  bool SetFansubFilter(const wstring& group_name = L"TaigaSubs (change this)");
  
public:
  int index;
  bool new_episode_available, playing;
  vector<bool> episode_available;
  wstring next_episode_path;
  wstring folder, synonyms;
  int settings_keep_title;

public:
  wstring genres, popularity, producers, rank, score, synopsis;
};

#endif // ANIME_H