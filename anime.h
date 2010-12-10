/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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

  int  Ask(CEpisode episode);
  void Start(CEpisode episode);
  void End(CEpisode episode, bool do_end, bool do_update);
  void Update(CEpisode episode, bool do_move);
  void CheckFolder();
  void CheckNewEpisode(bool check_folder = false);
  int GetLastWatchedEpisode();
  int GetScore();
  int GetStatus();
  wstring GetTags();
  int GetTotalEpisodes();
  bool ParseSearchResult(const wstring& data);
  void Edit(const wstring& data, 
    int index = -1, int episode = -1, int score = -1, 
    int status = -1, int mode = -1, const wstring& tags = L"");
  void SetStartDate(wstring date, bool ignore_previous);
  void SetFinishDate(wstring date, bool ignore_previous);
  void Delete();
  
  int Index;
  bool NewEps, Playing;
  wstring Folder, Score, Synonyms, Synopsis;
};

#endif // ANIME_H