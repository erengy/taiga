/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#include <regex>
#include <string>
#include <vector>

#include "base/types.h"

namespace track {
namespace recognition {

enum class Stream {
  Unknown,
  Animelab,
  Adn,
  Ann,
  Crunchyroll,
  Funimation,
  Hidive,
  Plex,
  Veoh,
  Viz,
  Vrv,
  Wakanim,
  Yahoo,
  Youtube,
};

struct StreamData {
  Stream id;
  enum_t option_id;
  std::wstring name;
  std::wstring url;
  std::regex url_pattern;
  std::regex title_pattern;
};

const std::vector<StreamData>& GetStreamData();

bool GetTitleFromStreamingMediaProvider(const std::wstring& url, std::wstring& title);
void NormalizeWebBrowserTitle(const std::wstring& url, std::wstring& title);

}  // namespace recognition
}  // namespace track
