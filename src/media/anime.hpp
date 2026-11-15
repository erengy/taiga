/**
 * Taiga
 * Copyright (C) 2010-2026, Eren Okka
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
#include <string>
#include <vector>

#include "base/chrono.hpp"

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

// clang-format off
constexpr std::array<Type, 6> kTypes{
    Type::Tv,
    Type::Ova,
    Type::Movie,
    Type::Special,
    Type::Ona,
    Type::Music,
};
// clang-format on

constexpr int kMaxEpisodeCount = 1900;
constexpr int kUnknownEpisodeCount = -1;
constexpr int kUnknownEpisodeLength = -1;
constexpr int kUnknownId = 0;
constexpr double kUnknownScore = 0.0;

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
  std::time_t lastModified = 0;
  int episodeCount = kUnknownEpisodeCount;
  int episodeLength = kUnknownEpisodeLength;
  AgeRating ageRating = AgeRating::Unknown;
  Status status = Status::Unknown;
  Type type = Type::Unknown;
  FuzzyDate dateStarted;
  FuzzyDate dateFinished;
  float score = 0.0f;
  int popularityRank = 0;
  std::string imageUrl;
  std::string slug;
  std::string synopsis;
  std::string trailerId;
  Titles titles;
  std::vector<std::string> genres;
  std::vector<std::string> producers;
  std::vector<std::string> studios;
  std::vector<std::string> tags;
  int lastAiredEpisode = 0;
  std::time_t nextEpisodeTime = 0;
};

}  // namespace anime

using Anime = anime::Details;
