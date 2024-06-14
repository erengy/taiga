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

#include "track/episode_util.h"

#include "base/format.h"
#include "base/string.h"

namespace anime {

int GetEpisodeHigh(const Episode& episode) {
  return episode.episode_number_range().second;
}

int GetEpisodeLow(const Episode& episode) {
  return episode.episode_number_range().first;
}

static std::wstring GetElementRange(anitomy::ElementCategory category,
                                    const Episode& episode) {
  const auto element_count = episode.elements().count(category);

  if (element_count > 1) {
    const auto range = episode.GetElementAsRange(category);
    if (range.second > range.first)
      return L"{}-{}"_format(range.first, range.second);
  }

  if (element_count > 0)
    return ToWstr(episode.GetElementAsInt(category));

  return std::wstring();
}

std::wstring GetEpisodeRange(const Episode& episode) {
  if (IsEpisodeRange(episode))
    return L"{}-{}"_format(GetEpisodeLow(episode), GetEpisodeHigh(episode));

  if (!episode.elements().empty(anitomy::kElementEpisodeNumber))
    return ToWstr(episode.episode_number());

  return std::wstring();
}

std::wstring GetVolumeRange(const Episode& episode) {
  return GetElementRange(anitomy::kElementVolumeNumber, episode);
}

std::wstring GetEpisodeRange(const number_range_t& range) {
  if (range.second > range.first)
    return L"{}-{}"_format(range.first, range.second);

  return ToWstr(range.first);
}

bool IsEpisodeRange(const Episode& episode) {
  if (episode.elements().count(anitomy::kElementEpisodeNumber) < 2)
    return false;
  return GetEpisodeHigh(episode) > GetEpisodeLow(episode);
}

////////////////////////////////////////////////////////////////////////////////

int GetVideoResolutionHeight(const std::wstring& str) {
  // *###x###*
  if (str.length() > 6) {
    int pos = InStr(str, L"x", 0);
    if (pos > -1) {
      for (unsigned int i = 0; i < str.length(); i++)
        if (i != pos && !IsNumericChar(str.at(i)))
          return 0;
      return ToInt(str.substr(pos + 1));
    }

  // *###p
  } else if (str.length() > 3) {
    if (str.at(str.length() - 1) == 'p') {
      for (unsigned int i = 0; i < str.length() - 1; i++)
        if (!IsNumericChar(str.at(i)))
          return 0;
      return ToInt(str.substr(0, str.length() - 1));
    }
  }

  // 1080, 720, 480
  if (IsNumericString(str)) {
    const auto height = ToInt(str);
    switch (height) {
      case 1080:
      case 720:
      case 480:
        return height;
    }
  }

  return 0;
}

std::wstring NormalizeVideoResolution(const std::wstring& resolution) {
  const auto height = GetVideoResolutionHeight(resolution);
  return height ? L"{}p"_format(height) : resolution;
}

}  // namespace anime
