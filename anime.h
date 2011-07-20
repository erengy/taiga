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

// =============================================================================

class CEpisode {
public:
  CEpisode();
  virtual ~CEpisode() {}

  void Clear();

  int Index;
  wstring File, Folder, Format, Title, Name, Group, Number, Version, 
    Resolution, AudioType, VideoType, Checksum, Extra;
};

extern CEpisode CurrentEpisode;

// =============================================================================

class CEventItem;

class CMALAnime {
public:
  CMALAnime();
  virtual ~CMALAnime() {}

  int Series_ID, Series_Type, Series_Episodes, Series_Status,
    My_ID, My_WatchedEpisodes, My_Score, My_Status, My_Rewatching, My_RewatchingEP;
  wstring Series_Title, Series_Synonyms, Series_Start, Series_End, Series_Image,
    My_LastUpdated, My_StartDate, My_FinishDate, My_Tags;
};

class CAnime : public CMALAnime {
public:
  CAnime();
  virtual ~CAnime() {}

  int Ask(CEpisode episode);
  void Start(CEpisode episode);
  void End(CEpisode episode, bool end_watching, bool update_list);
  void Update(CEpisode episode, bool change_status);
  void CheckFolder();
  void SetFolder(const wstring& folder, bool save_settings, bool check_episodes);
  bool CheckEpisodes(int episode = -1, bool check_folder = false);
  bool PlayEpisode(int number);
  bool IsEpisodeAvailable(int number);
  bool SetEpisodeAvailability(int number, bool available, const wstring& path = L"");
  void SetLocalData(const wstring& folder, const wstring& titles);
  int GetIntValue(int mode);
  int GetLastWatchedEpisode();
  int GetRewatching();
  int GetScore();
  int GetStatus();
  wstring GetStrValue(int mode);
  wstring GetTags();
  int GetTotalEpisodes();
  bool ParseSearchResult(const wstring& data);
  void Edit(const wstring& data, CEventItem item);
  bool IsAiredYet(bool strict = false) const;
  bool IsFinishedAiring() const;
  int GetAiringStatus();
  void SetStartDate(wstring date, bool ignore_previous);
  void SetFinishDate(wstring date, bool ignore_previous);
  wstring GetFansubFilter();
  bool SetFansubFilter(const wstring& group_name = L"TaigaSubs (change this)");
  
  int Index;
  bool NewEps, Playing;
  vector<bool> EpisodeAvailable;
  wstring NextEpisodePath;
  wstring Folder, Synonyms;
  wstring Genres, Popularity, Rank, Score, Synopsis;
};

#endif // ANIME_H