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

#include "string.hpp"

#include <QRegularExpression>
#include <nstd/string.hpp>
#include <ranges>

int compareStrings(const std::string_view a, const std::string_view b,
                   Qt::CaseSensitivity caseSensitivity) {
  return QString::fromUtf8(a).compare(QString::fromUtf8(b), caseSensitivity);
}

QString joinStrings(const std::vector<std::string>& list, QString placeholder) {
  if (list.empty()) return placeholder;
  return QString::fromStdString(nstd::join(list, ", "));
}

void removeHtmlTags(QString& str) {
  static const QRegularExpression re{"<[a-z/]+>"};
  str.remove(re);
}

QString& replaceWholeWord(QString& str, const QString& before, const QString& after) {
  static constexpr auto is_boundary = [](const QChar& c) { return c.isSpace() || c.isPunct(); };

  const auto is_whole_word = [&str, &before](qsizetype pos) {
    if (pos == 0 || is_boundary(str.at(pos - 1))) {
      const qsizetype pos_end = pos + before.size();
      if (pos_end >= str.size() || is_boundary(str.at(pos_end))) return true;
    }
    return false;
  };

  for (auto pos = str.indexOf(before); pos != -1; pos = str.indexOf(before, pos)) {
    if (!is_whole_word(pos)) {
      pos += before.size();
    } else {
      str.replace(pos, before.size(), after);
      pos += after.size();
    }
  }

  return str;
}

[[nodiscard]] int toInt(const std::string& value) {
  return QString::fromStdString(value).toInt();
};

[[nodiscard]] std::string toStdString(const QString& s) {
  return s.toStdString();
}

std::vector<std::string> toVector(const QStringList& list) {
  return list | std::views::transform(toStdString) | std::ranges::to<std::vector>();
}
