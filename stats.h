/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

#ifndef STATS_H
#define STATS_H

#include "std.h"

// =============================================================================

class TaigaStats {
public:
  TaigaStats();
  virtual ~TaigaStats() {}

  void Calculate();

public:
  int anime_count_;
  int episode_count_;
  wstring life_spent_;
  float score_mean_;
  float score_dev_;
};

extern TaigaStats Stats;

#endif // STATS_H