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

#include <chrono>
#include <string>
#include <vector>

#include "base/settings.hpp"

namespace taiga {

class Settings final : public base::Settings {
public:
  void init() const;

  Qt::ColorScheme appColorScheme() const;
  std::string service() const;
  std::vector<std::string> libraryFolders() const;
  std::chrono::milliseconds mediaDetectionInterval() const;
  bool syncEnabled() const;

  void setAppColorScheme(const Qt::ColorScheme scheme) const;
  void setService(const std::string& service) const;
  void setLibraryFolders(std::vector<std::string> folders) const;
  void setMediaDetectionInterval(const std::chrono::milliseconds interval) const;
  void setSyncEnabled(const bool enabled) const;

private:
  QString fileName() const override;
};

inline Settings settings;

}  // namespace taiga
