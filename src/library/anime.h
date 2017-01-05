/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#pragma once

#include <string>
#include <vector>

#include "base/time.h"

namespace anime {

// ID_NOTINLIST
//   Used in Episode data to denote the item is not in user's list.
//   Item may or may not be in the database.
//
// ID_UNKNOWN
//   There's no item in the database with this ID.
//   This is the default ID for all anime items.
//
enum AnimeId {
  ID_NOTINLIST = -1,
  ID_UNKNOWN = 0
};

enum SeriesStatus {
  kUnknownStatus,
  kFinishedAiring,
  kAiring,
  kNotYetAired
};

enum SeriesType {
  kUnknownType,
  kTv,
  kOva,
  kMovie,
  kSpecial,
  kOna,
  kMusic
};

const int kUnknownEpisodeCount = -1;
const int kUnknownEpisodeLength = -1;
const double kUnknownScore = 0.0;

enum MyStatus {
  kMyStatusFirst = 1,
  kNotInList = 0,
  kWatching,
  kCompleted,
  kOnHold,
  kDropped,
  kPlanToWatch,
  kMyStatusLast
};

enum AgeRating {
  kUnknownAgeRating,
  kAgeRatingG,
  kAgeRatingPG,
  kAgeRatingPG13,
  kAgeRatingR17,
  kAgeRatingR18
};

// Invalid for anime items that are not in user's list
class MyInformation {
 public:
  MyInformation();
  virtual ~MyInformation() {}

  std::wstring id;
  int watched_episodes;
  int score;
  int status;
  int rewatched_times;
  int rewatching;
  int rewatching_ep;
  Date date_start;
  Date date_finish;
  std::wstring last_updated;
  std::wstring tags;
  std::wstring notes;
};

// For all kinds of other temporary information
class LocalInformation {
 public:
  LocalInformation();
  virtual ~LocalInformation() {}

  int last_aired_episode;
  std::vector<bool> available_episodes;
  std::wstring next_episode_path;
  std::wstring folder;
  std::vector<std::wstring> synonyms;
  bool playing;
  bool use_alternative;
};

}  // namespace anime
