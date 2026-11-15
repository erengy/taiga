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

#include "event.hpp"

#include "base/string.hpp"
#include "media/anime_db.hpp"
#include "media/anime_list.hpp"
#include "media/anime_list_utils.hpp"
#include "media/anime_utils.hpp"
#include "sync/service.hpp"

namespace share {

namespace {

QString eventName(const EventType type) {
  // clang-format off
  switch (type) {
    case EventType::Playing: return u"playing"_s;
    case EventType::Stopped: return u"stopped"_s;
    case EventType::Updated: return u"updated"_s;
  }
  // clang-format on
  return {};
}

QString statusName(const anime::list::Status status) {
  using enum anime::list::Status;
  // clang-format off
  switch (status) {
    case Watching: return u"watching"_s;
    case Completed: return u"completed"_s;
    case OnHold: return u"onHold"_s;
    case Dropped: return u"dropped"_s;
    case PlanToWatch: return u"planToWatch"_s;
    case NotInList: break;
  }
  // clang-format on
  return {};
}

QJsonObject animeToJson(const Anime& item) {
  QJsonObject object{
      {u"id"_s, item.id},
      {u"title"_s, QString::fromStdString(anime::preferredTitle(item))},
      {u"url"_s, sync::animePageUrl(item.id)},
  };
  if (item.episodeCount > 0) object[u"episodes"_s] = item.episodeCount;
  if (!item.imageUrl.empty()) object[u"image"_s] = QString::fromStdString(item.imageUrl);
  return object;
}

QJsonObject entryToJson(const ListEntry& entry) {
  QJsonObject object{
      {u"status"_s, statusName(entry.status)},
      {u"progress"_s, entry.watchedEpisodes},
      {u"rewatching"_s, entry.rewatching},
  };
  if (entry.score > 0) object[u"score"_s] = entry.score;
  return object;
}

}  // namespace

const Anime* shareableAnime(const int animeId) {
  const auto* item = anime::db()->item(animeId);
  if (!item) return nullptr;

  const auto* entry = anime::db()->entry(item->id);
  if (entry && entry->isPrivate) return nullptr;

  return item;
}

QJsonObject toJson(const Event& event) {
  QJsonObject object{
      {u"version"_s, kEventVersion},
      {u"event"_s, eventName(event.type)},
      {u"timestamp"_s, event.time.toUTC().toString(Qt::ISODate)},
      {u"service"_s, sync::serviceSlug(sync::currentServiceId())},
  };

  if (const auto* item = shareableAnime(event.animeId)) {
    object[u"anime"_s] = animeToJson(*item);
    if (const auto* entry = anime::db()->entry(item->id); anime::list::isInList(entry)) {
      object[u"entry"_s] = entryToJson(*entry);
    }
  }

  if (event.episode > 0) {
    object[u"episode"_s] = QJsonObject{{u"number"_s, event.episode}};
  }

  return object;
}

}  // namespace share
