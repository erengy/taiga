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

#pragma once

#include <map>

#include "base/gfx.h"

namespace anime {

class ImageDatabase {
public:
  ImageDatabase() {}
  virtual ~ImageDatabase() {}

  // Loads a picture into memory, downloads a new file if requested.
  bool Load(int anime_id, bool load, bool download);

  bool Reload(int anime_id);

  // Releases image data from memory if an image is not in sight.
  void FreeMemory();
  void Clear();

  // Returns a pointer to requested image if available.
  base::Image* GetImage(int anime_id);

private:
  std::map<int, base::Image> items_;
};

}  // namespace anime

extern anime::ImageDatabase ImageDatabase;
