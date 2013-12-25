/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#ifndef TAIGA_SYNC_MYANIMELIST_TYPES_H
#define TAIGA_SYNC_MYANIMELIST_TYPES_H

namespace sync {
namespace myanimelist {

enum SeriesStatus {
  kAiring = 1,
  kFinishedAiring,
  kNotYetAired
};

enum SeriesType {
  kTv = 1,
  kOva,
  kMovie,
  kSpecial,
  kOna,
  kMusic
};

enum MyStatus {
  kWatching = 1,
  kCompleted,
  kOnHold,
  kDropped,
  kPlanToWatch = 6
};

}  // namespace myanimelist
}  // namespace sync

#endif  // TAIGA_SYNC_MYANIMELIST_TYPES_H