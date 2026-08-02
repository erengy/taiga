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

#include <QRegularExpression>

namespace compat::v1 {

// This regex is used to remove the extra root element from v1's XML documents so that they
// can be read by `QXmlStreamReader` without an "Extra content at end of document" error.
// See #842 for more information.
inline void removeMetaElement(QString& str) {
  static const QRegularExpression meta_element_regex{
      "<meta>.+?</meta>", QRegularExpression::DotMatchesEverythingOption};
  str.remove(meta_element_regex);
}

}  // namespace compat::v1
