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

#include "base/settings.h"

namespace base {

SettingValueType GetSettingValueType(const SettingVariant& variant) {
  return static_cast<SettingValueType>(variant.index());
}

Settings::value_t Settings::value(const key_t& key) const {
  const auto it = map_.find(key);
  return it != map_.end() ? it->second : value_t{};
}

bool Settings::set_value(const key_t& key, value_t&& value) {
  auto& value_ = map_[key];
  if (value_ != value) {
    value_ = std::move(value);
    return true;
  }
  return false;
}

}  // namespace base
