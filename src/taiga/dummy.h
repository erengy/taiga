/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#pragma once

#include "library/anime_episode.h"
#include "library/anime_item.h"

namespace taiga {

class DummyAnime : public anime::Item {
public:
  void Initialize();
};

class DummyEpisode : public anime::Episode {
public:
  void Initialize();
};

extern class DummyAnime DummyAnime;
extern class DummyEpisode DummyEpisode;

}  // namespace taiga
