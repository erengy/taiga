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

#include "file.hpp"

#include <QFile>
#include <QFileInfo>
#include <algorithm>
#include <array>

namespace base {

bool isExecutableFile(const QString& path) {
  const QFileInfo info(path);
  if (!info.isFile()) return false;
  if (info.isExecutable()) return true;

  // See: https://github.com/erengy/taiga/issues/1232
  constexpr std::array kDangerousExtensions{
      "bat", "cmd", "com", "exe", "lnk", "msi", "ps1", "scr", "url", "vbs",
  };

  return std::ranges::contains(kDangerousExtensions, info.suffix().toLower().toStdString());
}

QString readFile(const QString& name) {
  QFile file{name};
  return file.open(QFile::ReadOnly) ? file.readAll() : QString{};
}

}  // namespace base
