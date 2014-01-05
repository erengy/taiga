/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include "base/file.h"
#include "base/foreach.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/discover.h"
#include "library/resource.h"
#include "sync/sync.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_season.h"

anime::ImageDatabase ImageDatabase;

namespace anime {

bool ImageDatabase::Load(int anime_id, bool load, bool download) {
  if (anime_id <= anime::ID_UNKNOWN)
    return false;

  if (items_.find(anime_id) != items_.end()) {
    if (items_[anime_id].data > anime::ID_UNKNOWN) {
      return true;
    } else if (!load) {
      return false;
    }
  }

  if (items_[anime_id].Load(anime::GetImagePath(anime_id))) {
    items_[anime_id].data = anime_id;
    if (download) {
      // Refresh if current file is too old
      auto anime_item = AnimeDatabase.FindItem(anime_id);
      if (anime_item->GetAiringStatus() != kFinishedAiring) {
        // Check last modified date (>= 7 days)
        if (GetFileAge(anime::GetImagePath(anime_id)) / (60 * 60 * 24) >= 7) {
          sync::DownloadImage(anime_id, anime_item->GetImageUrl());
        }
      }
    }
    return true;
  } else {
    items_[anime_id].data = -1;
  }

  if (download) {
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    sync::DownloadImage(anime_id, anime_item->GetImageUrl());
  }

  return false;
}

void ImageDatabase::FreeMemory() {
  foreach_(it, ::AnimeDatabase.items) {
    bool erase = true;
    int anime_id = it->first;

    if (items_.find(anime_id) == items_.end())
      continue;

    if (::AnimeDialog.GetCurrentId() == anime_id ||
        ::NowPlayingDialog.GetCurrentId() == anime_id)
      erase = false;

    if (!SeasonDatabase.items.empty())
      if (std::find(SeasonDatabase.items.begin(), SeasonDatabase.items.end(),
                    anime_id) != SeasonDatabase.items.end())
        if (SeasonDialog.IsVisible())
          erase = false;

    if (erase)
      items_.erase(anime_id);
  }
}

base::Image* ImageDatabase::GetImage(int anime_id) {
  if (items_.find(anime_id) != items_.end())
    if (items_[anime_id].data > 0)
      return &items_[anime_id];

  return nullptr;
}

}  // namespace anime