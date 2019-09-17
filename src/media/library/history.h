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

#include <string>
#include <vector>

#include "base/time.h"
#include "base/xml.h"
#include "media/library/queue.h"
#include "track/episode.h"

namespace library {

struct HistoryItem {
  int anime_id = 0;
  int episode = 0;
  std::wstring time;
};

class History {
public:
  History();

  void Clear(bool save = true);
  bool Load();
  bool Save();

  void HandleCompatibility(const std::wstring& meta_version);

  std::vector<HistoryItem> items;
  int limit = 0;  // 0 for unlimited

private:
  void ReadQueue(const pugi::xml_document& document);
  void ReadQueueInCompatibilityMode(const pugi::xml_document& document);

  int TranslateModeFromString(const std::wstring& mode);
  std::wstring TranslateModeToString(int mode);
};

inline class History history;

}  // namespace library
