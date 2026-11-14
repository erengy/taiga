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

#include <QDateTime>
#include <QJsonObject>

#include "media/anime.hpp"

namespace share {

// Version of the JSON document produced by `toJson`. Bump it on breaking changes only
// (adding fields is not one).
constexpr int kEventVersion = 1;

enum class EventType {
  Playing,  // an episode started playing
  Stopped,  // the episode that was playing is no longer playing
  Updated,  // the list entry was updated locally after watching an episode
};

struct Event {
  EventType type = EventType::Playing;
  int animeId = anime::kUnknownId;
  int episode = 0;  // last episode number of the file, 0 if unknown
  QDateTime time = QDateTime::currentDateTimeUtc();
};

// Anime that can be shared, or nothing if it is unknown or its list entry is private.
const Anime* shareableAnime(const int animeId);

// ```
// {
//   "version": 1,
//   "event": "playing" | "stopped" | "updated",
//   "timestamp": "2026-01-01T00:00:00Z",
//   "service": "anilist" | "kitsu" | "myanimelist",
//   "anime": {"id", "title", "episodes"?, "image"?, "url"},
//   "episode": {"number"}?,
//   "entry": {"status", "progress", "score"?, "rewatching"}?
// }
// ```
//
// - `anime` and `entry` reflect the database at call time, not when the event occurred.
// - `anime.id` belongs to `service`.
// - `entry.score` ranges from 1 to 100.
// - Optional fields are omitted when unknown.
QJsonObject toJson(const Event& event);

}  // namespace share
