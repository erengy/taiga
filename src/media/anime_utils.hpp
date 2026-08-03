/**
 * Taiga
 * Copyright (C) 2010-2026, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <string>

namespace anime {

enum class Status;
struct Details;

Status airingStatus(const Details& item);
bool isAiredYet(const Details& item);
bool isFinishedAiring(const Details& item);

int estimateEpisodeCount(const Details& item, const int lastKnownEpisode);
int estimateEpisodeLength(const Details& item);
int estimateLastAiredEpisodeNumber(const Details& item);

bool isNsfw(const Details& item);
bool isStale(const Details& item);

const std::string& preferredTitle(const Details& item);

}  // namespace anime
