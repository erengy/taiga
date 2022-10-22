/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include <nstd/algorithm.hpp>

#include "ui/resource.h"

#include "base/file.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "media/anime_season_db.h"
#include "media/anime_util.h"
#include "sync/sync.h"
#include "taiga/path.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_season.h"

namespace ui {

void ImageDatabase::Load(const int anime_id, const bool download) {
  if (GetImage(anime_id))
    return;

  LoadFile(anime_id);

  const auto is_old_image = [](const anime::Item& anime_item) {
    return (anime_item.GetAiringStatus() != anime::SeriesStatus::FinishedAiring) &&
           (GetFileAge(anime::GetImagePath(anime_item.GetId())) /
                (60 * 60 * 24) >= 7);  // >= 7 days
  };

  if (download) {
    if (const auto anime_item = anime::db.Find(anime_id)) {
      if (!anime::IsValidId(static_cast<int>(items_[anime_id].data)) ||
          is_old_image(*anime_item)) {
        sync::DownloadImage(anime_id, anime_item->GetImageUrl());
      }
    }
  }
}

void ImageDatabase::Load(const std::vector<int>& anime_ids) {
  for (const auto anime_id : anime_ids) {
    Load(anime_id, true);
  }
}

bool ImageDatabase::LoadFile(const int anime_id) {
  if (!anime::IsValidId(anime_id))
    return false;

  if (items_[anime_id].Load(anime::GetImagePath(anime_id))) {
    items_[anime_id].data = anime_id;
  } else {
    items_[anime_id].data = -1;
  }

  return anime::IsValidId(static_cast<int>(items_[anime_id].data));
}

void ImageDatabase::FreeMemory() {
  for (const auto& [anime_id, anime_item] : anime::db.items) {
    if (items_.find(anime_id) == items_.end())
      continue;

    if (ui::DlgAnime.GetCurrentId() == anime_id ||
        ui::DlgNowPlaying.GetCurrentId() == anime_id) {
      continue;
    }

    if (ui::DlgSeason.IsVisible() &&
        nstd::contains(anime::season_db.items, anime_id)) {
      continue;
    }

    items_.erase(anime_id);
  }
}

void ImageDatabase::Clear() {
  items_.clear();
  DeleteFolder(taiga::GetPath(taiga::Path::DatabaseImage));
}

base::Image* ImageDatabase::GetImage(const int anime_id) {
  if (const auto it = items_.find(anime_id); it != items_.end()) {
    return anime::IsValidId(static_cast<int>(it->second.data)) ? &it->second
                                                               : nullptr;
  }
  return LoadFile(anime_id) ? &items_[anime_id] : nullptr;
}

}  // namespace ui
