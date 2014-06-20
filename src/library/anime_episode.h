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

#ifndef TAIGA_LIBRARY_ANIME_EPISODE_H
#define TAIGA_LIBRARY_ANIME_EPISODE_H

#include <string>

namespace anime {

class Episode {
 public:
  Episode();
  virtual ~Episode() {}

  void Clear();
  void Set(int anime_id);

  int anime_id;
  std::wstring file;
  std::wstring folder;
  std::wstring format;
  std::wstring title;
  std::wstring clean_title;
  std::wstring name;
  std::wstring group;
  std::wstring number;
  std::wstring version;
  std::wstring resolution;
  std::wstring audio_type;
  std::wstring video_type;
  std::wstring checksum;
  std::wstring extras;
  std::wstring year;
  bool processed;
};

}  // namespace anime

extern anime::Episode CurrentEpisode;

#endif  // TAIGA_LIBRARY_ANIME_EPISODE_H