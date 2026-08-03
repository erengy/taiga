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

#include "common.hpp"

#include <QRegularExpression>
#include <QString>

namespace compat::v1 {

void removeMetaElement(QString& str) {
  static const QRegularExpression meta_element_regex{
      "<meta>.+?</meta>", QRegularExpression::DotMatchesEverythingOption};
  str.remove(meta_element_regex);
}

void removeInvalidCharacterReferences(QString& str) {
  static const QRegularExpression ref_regex{"&#(x?)([0-9A-Fa-f]+);"};

  const auto is_illegal = [](const uint codepoint) {
    return codepoint <= 0x1F && codepoint != 0x9 && codepoint != 0xA && codepoint != 0xD;
  };

  qsizetype offset = 0;
  for (auto match = ref_regex.match(str, offset); match.hasMatch();
       match = ref_regex.match(str, offset)) {
    bool ok = false;
    const auto codepoint = match.captured(2).toUInt(&ok, match.captured(1).isEmpty() ? 10 : 16);
    if (ok && is_illegal(codepoint)) {
      str.remove(match.capturedStart(), match.capturedLength());
      offset = match.capturedStart();
    } else {
      offset = match.capturedEnd();
    }
  }
}

}  // namespace compat::v1
