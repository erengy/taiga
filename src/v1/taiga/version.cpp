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

#include "taiga/version.h"

#include "base/preprocessor.h"
#include "taiga/config.h"

namespace taiga {

const semaver::Version& version() {
  static const semaver::Version version(
      TAIGA_VERSION_MAJOR,
      TAIGA_VERSION_MINOR,
      TAIGA_VERSION_PATCH,
      TAIGA_VERSION_PRE,
      TAIGA_VERSION_BUILD > 0 ? STRINGIZE(TAIGA_VERSION_BUILD) : ""
    );
  return version;
}

}  // namespace taiga
