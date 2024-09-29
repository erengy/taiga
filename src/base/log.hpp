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

#include <format>
#include <monolog.hpp>
#include <string_view>

namespace base {

template <class... Args>
inline void log(const monolog::Level level, const monolog::Source& source,
                const std::format_string<Args...> fmt, Args&&... args) {
  const monolog::Record record{std::vformat(fmt.get(), std::make_format_args(args...))};
  monolog::log.write(level, record, source);
}

}  // namespace base

#define TAIGA_LOG(level, text, ...) \
  base::log(level, monolog::Source{__FILE__, __FUNCTION__, __LINE__}, text, __VA_ARGS__)

#define LOGD(text, ...) TAIGA_LOG(monolog::Level::Debug, text, __VA_ARGS__)
#define LOGI(text, ...) TAIGA_LOG(monolog::Level::Informational, text, __VA_ARGS__)
#define LOGW(text, ...) TAIGA_LOG(monolog::Level::Warning, text, __VA_ARGS__)
#define LOGE(text, ...) TAIGA_LOG(monolog::Level::Error, text, __VA_ARGS__)
