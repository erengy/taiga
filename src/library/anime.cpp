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

#include "anime.h"

namespace anime {

MyInformation::MyInformation()
    : watched_episodes(0),
      score(0),
      status(kNotInList),
      rewatched_times(0),
      rewatching(FALSE),
      rewatching_ep(0) {
}

LocalInformation::LocalInformation()
    : last_aired_episode(0),
      playing(false),
      use_alternative(false) {
}

} // namespace anime