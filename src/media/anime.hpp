/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <array>
#include <map>
#include <string>
#include <vector>

#include "base/chrono.hpp"
// #include "sync/service.h"

namespace anime {

enum AnimeId {
  ID_NOTINLIST = -1,
  ID_UNKNOWN = 0
};

enum class AgeRating {
  Unknown,
  G,
  PG,
  PG13,
  R17,
  R18,
};

enum class SeriesStatus {
  Unknown,
  FinishedAiring,
  Airing,
  NotYetAired
};

enum class SeriesType {
  Unknown,
  Tv,
  Ova,
  Movie,
  Special,
  Ona,
  Music,
};

enum class MyStatus {
  NotInList,
  Watching,
  Completed,
  OnHold,
  Dropped,
  PlanToWatch,
};

enum class TitleLanguage {
  Romaji,
  English,
  Native,
};

constexpr std::array<SeriesStatus, 3> kSeriesStatuses{
  SeriesStatus::FinishedAiring,
  SeriesStatus::Airing,
  SeriesStatus::NotYetAired,
};

constexpr std::array<SeriesType, 6> kSeriesTypes{
  SeriesType::Tv,
  SeriesType::Ova,
  SeriesType::Movie,
  SeriesType::Special,
  SeriesType::Ona,
  SeriesType::Music,
};

constexpr std::array<MyStatus, 5> kMyStatuses{
  MyStatus::Watching,
  MyStatus::Completed,
  MyStatus::OnHold,
  MyStatus::Dropped,
  MyStatus::PlanToWatch,
};

constexpr int kMaxEpisodeCount = 1900;
constexpr int kUnknownEpisodeCount = -1;
constexpr int kUnknownEpisodeLength = -1;
constexpr double kUnknownScore = 0.0;
constexpr int kUserScoreMax = 100;

struct Titles {
  std::string romaji;
  std::string english;
  std::string japanese;
  std::vector<std::string> synonyms;
};

struct SeriesInformation {
  int id = AnimeId::ID_UNKNOWN;
  // std::map<sync::ServiceId, std::string> uids;
  // sync::ServiceId source = sync::ServiceId::Unknown;
  std::time_t last_modified = 0;
  int episode_count = kUnknownEpisodeCount;
  int episode_length = kUnknownEpisodeLength;
  AgeRating age_rating = AgeRating::Unknown;
  SeriesStatus status = SeriesStatus::Unknown;
  SeriesType type = SeriesType::Unknown;
  FuzzyDate start_date;
  FuzzyDate end_date;
  float score = 0.0f;
  int popularity_rank = 0;
  std::string image_url;
  std::string slug;
  std::string synopsis;
  std::string trailer_id;
  Titles titles;
  std::vector<std::string> genres;
  std::vector<std::string> producers;
  std::vector<std::string> studios;
  std::vector<std::string> tags;
  int last_aired_episode = 0;
  std::time_t next_episode_time = 0;
};

struct MyInformation {
  std::string id;
  int watched_episodes = 0;
  int score = 0;
  MyStatus status = MyStatus::NotInList;
  bool is_private = false;
  int rewatched_times = 0;
  bool rewatching = false;
  int rewatching_ep = 0;
  FuzzyDate date_start;
  FuzzyDate date_finish;
  std::string last_updated;
  std::string notes;
};

struct LocalInformation {
  std::vector<bool> available_episodes;
  std::string next_episode_path;
};

}  // namespace anime

using Anime = anime::SeriesInformation;
