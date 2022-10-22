/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <fmt/format.h>
#include <fmt/xchar.h>
#include <monolog.hpp>

namespace base {

template <class... Args>
void Log(const monolog::Level level, const monolog::Source& source,
         const std::wstring& str, const Args&... args) {
  const monolog::Record record{sizeof...(Args) ? fmt::format(str, args...)
                                               : str};
  monolog::log.Write(level, record, source);
}

}  // namespace base

#define TAIGA_LOG(level, text, ...) \
    base::Log(level, monolog::Source{__FILE__, __FUNCTION__, __LINE__}, text, __VA_ARGS__)

#define LOGD(text, ...) TAIGA_LOG(monolog::Level::Debug, text, __VA_ARGS__)
#define LOGI(text, ...) TAIGA_LOG(monolog::Level::Informational, text, __VA_ARGS__)
#define LOGW(text, ...) TAIGA_LOG(monolog::Level::Warning, text, __VA_ARGS__)
#define LOGE(text, ...) TAIGA_LOG(monolog::Level::Error, text, __VA_ARGS__)
