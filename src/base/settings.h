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

#include <map>
#include <string>
#include <variant>

namespace base {

enum class SettingValueType : size_t {
  Bool,
  Int,
  Wstring,
  None = std::variant_npos
};

using SettingVariant = std::variant<bool, int, std::wstring>;

SettingValueType GetSettingValueType(const SettingVariant& variant);

class Settings {
public:
  using key_t = std::string;
  using value_t = SettingVariant;

  value_t value(const key_t& key) const;
  bool set_value(const key_t& key, value_t&& value);

private:
  std::map<key_t, value_t> map_;
};

}  // namespace base
