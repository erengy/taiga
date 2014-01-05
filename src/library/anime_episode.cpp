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

#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "ui/menu.h"

anime::Episode CurrentEpisode;

namespace anime {

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
  ui::Menus.UpdateAll(AnimeDatabase.FindItem(anime_id));
}

}  // namespace anime