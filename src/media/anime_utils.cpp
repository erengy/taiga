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

#include "anime_utils.hpp"

#include <QDateTime>
#include <QTimeZone>
#include <ranges>

#include "base/chrono.hpp"
#include "media/anime.hpp"
#include "media/anime_db.hpp"
#include "taiga/settings.hpp"

namespace {

FuzzyDate dateInJapan() {
  static const auto tz = QTimeZone{"Asia/Tokyo"};
  return FuzzyDate{QDateTime::currentDateTime(tz).date().toStdSysDays()};
}

}  // namespace

namespace anime {

Status airingStatus(const Details& item) {
  // We can trust the active service when it says something happened in the past
  // (but not the opposite, because the information might be stale).
  // This is the most common case, and it also handles the rare case where an
  // anime has finished airing but the dates are completely unknown.
  if (item.status == Status::FinishedAiring) return Status::FinishedAiring;

  // It is now safe to assume that an anime with unknown dates has not yet
  // aired, considering we have covered the rare case mentioned above.
  if (item.date_started.empty() && item.date_finished.empty()) return Status::NotYetAired;

  const auto today = dateInJapan();
  if (!item.date_finished.empty() && item.date_finished < today) return Status::FinishedAiring;
  if (!item.date_started.empty() && item.date_started <= today) return Status::Airing;

  return Status::NotYetAired;
}

bool isAiredYet(const Details& item) {
  if (item.status == Status::FinishedAiring) return true;
  if (item.status == Status::Airing) return true;

  const auto status = airingStatus(item);
  if (status == Status::FinishedAiring) return true;
  if (status == Status::Airing) return true;

  return false;
}

bool isFinishedAiring(const Details& item) {
  return airingStatus(item) == Status::FinishedAiring;
}

////////////////////////////////////////////////////////////////////////////////

int estimateEpisodeCount(const Details& item, const int lastKnownEpisode) {
  // If we already know the number, we don't need to estimate
  if (item.episode_count > 0) return item.episode_count;

  // Given all TV series aired in 2006-2016, most of them have their episodes
  // spanning one or two seasons. Following is a table of top ten values:
  //
  //   Episodes    Seasons    Percent
  //   ------------------------------
  //         12          1      34.2%
  //         13          1      18.5%
  //         26          2       9.5%
  //         25          2       5.5%
  //         24          2       5.4%
  //         52          4       2.9%
  //         11          1       2.7%
  //         10          1       2.6%
  //         51          4       2.4%
  //         39          3       1.4%
  //   ------------------------------
  //   Total:                   85.1%
  //
  // With that in mind, we can normalize our output at several points.
  if (lastKnownEpisode < 12) return 12;
  if (lastKnownEpisode < 24) return 26;
  if (lastKnownEpisode < 50) return 52;

  // This is a series that has aired for more than a year, which means we cannot
  // estimate for how long it is going to continue.
  return 0;
}

int estimateEpisodeLength(const Details& item) {
  if (item.episode_length > 0) return item.episode_length;

  // clang-format off
  switch (item.type) {
    default:
    case Type::Tv:      return 24;
    case Type::Ova:     return 24;
    case Type::Movie:   return 90;
    case Type::Special: return 12;
    case Type::Ona:     return 24;
    case Type::Music:   return  5;
  }
  // clang-format on
}

int estimateLastAiredEpisodeNumber(const Details& item) {
  // Can't estimate for other types of anime
  if (item.type != Type::Tv) return 0;

  // TV series air weekly, so the number of weeks that has passed since the day
  // the series started airing gives us the last aired episode. Note that
  // irregularities such as broadcasts being postponed due to sports events make
  // this method unreliable.
  if (!item.date_started) return 0;

  // To compensate for the fact that we don't know the airing hour,
  // we subtract one more day.
  const auto days = dateInJapan() - item.date_started - std::chrono::days{1};
  const auto weeks = std::chrono::duration_cast<std::chrono::weeks>(days);
  if (weeks.count() < 0) return 0;

  if (!item.episode_count || weeks.count() < item.episode_count) {
    if (weeks.count() < 13) {  // Not reliable for longer series
      return weeks.count() + 1;
    }
  }

  return item.episode_count;
}

////////////////////////////////////////////////////////////////////////////////

bool isNsfw(const Details& item) {
  if (item.age_rating == anime::AgeRating::R18) return true;

  if (item.age_rating == anime::AgeRating::Unknown) {
    if (std::ranges::contains(item.genres, "Hentai")) return true;
  }

  return false;
}

bool isStale(const Details& item) {
  if (!item.last_modified) return true;
  if (item.synopsis.empty()) return true;
  if (item.genres.empty()) return true;
  if (item.score == kUnknownScore && isAiredYet(item)) return true;

  const auto ms = QDateTime::currentDateTime() - QDateTime::fromSecsSinceEpoch(item.last_modified);
  const auto duration = Duration{std::chrono::duration_cast<std::chrono::seconds>(ms)};

  if (airingStatus(item) == Status::FinishedAiring) return duration.weeks() >= 1;
  return duration.hours() >= 1;
}

////////////////////////////////////////////////////////////////////////////////

std::string preferredTitle(const Details& item) {
  if (const auto* settings = db.settings(item.id); settings && !settings->display_title.empty()) {
    return settings->display_title;
  }

  switch (taiga::settings.titleLanguage()) {
    default:
    case TitleLanguage::Romaji:
      return item.titles.romaji;
    case TitleLanguage::English:
      return !item.titles.english.empty() ? item.titles.english : item.titles.romaji;
    case TitleLanguage::Native:
      return !item.titles.japanese.empty() ? item.titles.japanese : item.titles.romaji;
  }
}

}  // namespace anime
