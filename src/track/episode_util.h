/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
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

#include "track/episode.h"

namespace anime {

int GetEpisodeHigh(const Episode& episode);
int GetEpisodeLow(const Episode& episode);

std::wstring GetEpisodeRange(const Episode& episode);
std::wstring GetVolumeRange(const Episode& episode);
std::wstring GetEpisodeRange(const number_range_t& range);
bool IsEpisodeRange(const Episode& episode);

int GetVideoResolutionHeight(const std::wstring& str);
std::wstring NormalizeVideoResolution(const std::wstring& resolution);

}  // namespace anime
