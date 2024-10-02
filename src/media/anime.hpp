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
// #include <map>
#include <string>
#include <vector>

#include "base/chrono.hpp"
// #include "sync/service.h"

namespace anime {

enum class AgeRating {
  Unknown,
  G,
  PG,
  PG13,
  R17,
  R18,
};

enum class Status {
  Unknown,
  FinishedAiring,
  Airing,
  NotYetAired,
};

enum class Type {
  Unknown,
  Tv,
  Ova,
  Movie,
  Special,
  Ona,
  Music,
};

enum class TitleLanguage {
  Romaji,
  English,
  Native,
};

constexpr std::array<Status, 3> kStatuses{
  Status::FinishedAiring,
  Status::Airing,
  Status::NotYetAired,
};

constexpr std::array<Type, 6> kTypes{
  Type::Tv,
  Type::Ova,
  Type::Movie,
  Type::Special,
  Type::Ona,
  Type::Music,
};

constexpr int kMaxEpisodeCount = 1900;
constexpr int kUnknownEpisodeCount = -1;
constexpr int kUnknownEpisodeLength = -1;
constexpr int kUnknownId = 0;
constexpr double kUnknownScore = 0.0;
constexpr int kUserScoreMax = 100;

struct Titles {
  std::string romaji;
  std::string english;
  std::string japanese;
  std::vector<std::string> synonyms;
};

struct Details {
  int id = kUnknownId;
  // std::map<sync::ServiceId, std::string> uids;
  // sync::ServiceId source = sync::ServiceId::Unknown;
  std::time_t last_modified = 0;
  int episode_count = kUnknownEpisodeCount;
  int episode_length = kUnknownEpisodeLength;
  AgeRating age_rating = AgeRating::Unknown;
  Status status = Status::Unknown;
  Type type = Type::Unknown;
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

namespace list {

enum class Status {
  NotInList,
  Watching,
  Completed,
  OnHold,
  Dropped,
  PlanToWatch,
};

constexpr std::array<Status, 5> kStatuses{
  Status::Watching,
  Status::Completed,
  Status::OnHold,
  Status::Dropped,
  Status::PlanToWatch,
};

struct Entry {
  std::string id;
  int anime_id = kUnknownId;
  int watched_episodes = 0;
  int score = 0;
  Status status = Status::NotInList;
  bool is_private = false;
  int rewatched_times = 0;
  bool rewatching = false;
  int rewatching_ep = 0;
  FuzzyDate date_start;
  FuzzyDate date_finish;
  std::string last_updated;
  std::string notes;
};

}  // namespace list

// struct LocalInformation {
//   std::vector<bool> available_episodes;
//   std::string next_episode_path;
// };

}  // namespace anime

using Anime = anime::Details;
using ListEntry = anime::list::Entry;
