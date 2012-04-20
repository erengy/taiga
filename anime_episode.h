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

#ifndef ANIME_EPISODE_H
#define ANIME_EPISODE_H

#include "std.h"

namespace anime {

// =============================================================================

class Episode {
 public:
  Episode();
  virtual ~Episode() {}

  void Clear();
  void Set(int anime_id);

  int anime_id;
  wstring file;
  wstring folder;
  wstring format;
  wstring title;
  wstring name;
  wstring group;
  wstring number;
  wstring version;
  wstring resolution;
  wstring audio_type;
  wstring video_type;
  wstring checksum;
  wstring extras;
};

} // namespace anime

extern anime::Episode CurrentEpisode;

#endif // ANIME_EPISODE_H