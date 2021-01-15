/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include <map>
#include <vector>

#include "base/gfx.h"

namespace ui {

class ImageDatabase {
public:
  ImageDatabase() {}
  virtual ~ImageDatabase() {}

  // Loads a picture into memory, downloads a new file if requested.
  void Load(const int anime_id, const bool download);
  void Load(const std::vector<int>& anime_ids);

  bool LoadFile(const int anime_id);

  // Releases image data from memory if an image is not in sight.
  void FreeMemory();
  void Clear();

  // Returns a pointer to requested image if available.
  base::Image* GetImage(const int anime_id);

private:
  std::map<int, base::Image> items_;
};

inline ImageDatabase image_db;

}  // namespace ui
