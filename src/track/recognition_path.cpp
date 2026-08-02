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

#include "recognition_path.hpp"

#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <anitomy.hpp>
#include <ranges>
#include <vector>

#include "base/string.hpp"
#include "taiga/settings.hpp"

namespace track::recognition {

namespace {

bool isTitle(const anitomy::Element& element) {
  return element.kind == anitomy::ElementKind::Title;
}

bool isLibraryFolder(const QDir& dir) {
  return std::ranges::any_of(taiga::settings.libraryFolders(), [&dir](const std::string& folder) {
    const auto normalizedPath = [](const QDir& dir) { return dir.absolutePath().toStdString(); };
    const auto path = normalizedPath(dir);
    const auto folderPath = normalizedPath(QDir(QString::fromStdString(folder)));
    return compareStrings(path, folderPath, Qt::CaseInsensitive) == 0;
  });
}

bool isKeywordOnly(const std::string& name) {
  const auto elements = anitomy::parse(name);
  return std::ranges::none_of(elements, isTitle);
}

bool isKnownInvalidName(const std::string& name) {
  static const std::vector<std::string> invalidNames{
      "anime", "download", "downloads", "extra", "extras",
  };

  return std::ranges::any_of(invalidNames, [&name](const std::string& invalidName) {
    return compareStrings(name, invalidName, Qt::CaseInsensitive) == 0;
  });
}

bool isInvalidDirectoryName(const std::string& name) {
  return name.empty() || isKnownInvalidName(name) || isKeywordOnly(name);
}

std::string findDirectoryName(const QFileInfo& info) {
  QDir dir = info.dir();

  constexpr int maxDepth = 2;

  for (int depth = 0; depth < maxDepth; ++depth) {
    const auto name = dir.dirName().toStdString();

    // A library folder, drive root (e.g. "C:"), or anything above cannot be
    // an anime folder.
    if (isLibraryFolder(dir) || name.contains(':')) break;

    if (!isInvalidDirectoryName(name)) return name;
    if (!dir.cdUp()) break;
  }

  return {};
}

std::string extractTitle(const std::string& directoryName) {
  const anitomy::Options options{
      .parse_episode = false,
      .parse_episode_title = false,
      .parse_file_checksum = false,
      .parse_file_extension = false,
  };

  // Parse directory name in case it looks like "[Group] Title [Info]"
  // rather than just "Title".
  const auto elements = anitomy::parse(directoryName, options);

  const auto it = std::ranges::find_if(elements, isTitle);

  return it != elements.end() ? it->value : directoryName;
}

}  // namespace

std::string findTitleFromPath(const QFileInfo& info) {
  const auto directoryName = findDirectoryName(info);
  if (directoryName.empty()) return {};

  return extractTitle(directoryName);
}

}  // namespace track::recognition
