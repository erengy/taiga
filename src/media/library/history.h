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
#include <vector>

#include "base/xml.h"

namespace library {

struct HistoryItem {
  int anime_id = 0;
  int episode = 0;
  std::wstring time;
};

class History {
public:
  void Clear(bool save = true);
  bool Load();
  bool Save();

  void HandleCompatibility(const std::wstring& meta_version);

  std::vector<HistoryItem> items;
  int limit = 0;  // 0 for unlimited

private:
  void ReadQueue(const XmlDocument& document);
};

inline class History history;

}  // namespace library
