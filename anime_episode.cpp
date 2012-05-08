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

#include "std.h"

#include "anime.h"
#include "anime_episode.h"

#include "dlg/dlg_main.h"

anime::Episode CurrentEpisode;

namespace anime {

// =============================================================================

Episode::Episode()
    : anime_id(ID_UNKNOWN) {
}

void Episode::Clear() {
  anime_id = ID_UNKNOWN;
  audio_type.clear();
  checksum.clear();
  extras.clear();
  file.clear();
  format.clear();
  group.clear();
  name.clear();
  number.clear();
  resolution.clear();
  title.clear();
  clean_title.clear();
  version.clear();
  video_type.clear();
}

void Episode::Set(int anime_id) {
  this->anime_id = anime_id;
  MainDialog.RefreshMenubar(anime_id);
}

} // namespace anime