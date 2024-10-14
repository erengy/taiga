/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
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

#include <QMap>
#include <QString>
#include <string>

#include "compat/settings.hpp"
#include "taiga/path.hpp"

namespace taiga {

inline QMap<QString, QString> read_settings() {
  const auto data_path = taiga::get_data_path();
  return compat::v1::read_settings(std::format("{}/v1/settings.xml", data_path));
}

}  // namespace taiga
