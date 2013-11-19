/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "base/std.h"

#include "anime.h"
#include "anime_db.h"
#include "anime_episode.h"

#include "base/common.h"

anime::Episode CurrentEpisode;

namespace anime {

// =============================================================================

Episode::Episode()
    : anime_id(ID_UNKNOWN), processed(false) {
}

void Episode::Clear() {
  anime_id = ID_UNKNOWN;
  audio_type.clear();
  checksum.clear();
  extras.clear();
  file.clear();
  folder.clear();
  format.clear();
  group.clear();
  name.clear();
  number.clear();
  resolution.clear();
  title.clear();
  clean_title.clear();
  version.clear();
  video_type.clear();
  year.clear();
  processed = false;
}

void Episode::Set(int anime_id) {
  this->anime_id = anime_id;
  this->processed = false;
  UpdateAllMenus(AnimeDatabase.FindItem(anime_id));
}

} // namespace anime