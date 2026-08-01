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

#include <semaver.hpp>

#include "media/library/history.h"

#include "base/string.h"

namespace library {

void History::HandleCompatibility(const std::wstring& meta_version) {
  const semaver::Version version(WstrToStr(meta_version));
  bool need_to_save = false;

  if (need_to_save)
    Save();
}

}  // namespace library
