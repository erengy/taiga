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

#include <hypp/uri.hpp>

class Url final {
public:
  using query_t = std::map<std::wstring, std::wstring>;

  Url() = default;
  Url(const hypp::Uri& uri) : uri_{uri} {}
  Url(const std::wstring& url) : Url(Parse(url)) {}

  Url& operator=(const Url& url);
  void operator=(const std::wstring& url);

  std::wstring to_wstring() const;

  std::wstring host() const;
  query_t query() const;
  hypp::Uri uri() const;

  static Url Parse(std::wstring url, const bool log_errors = true);

  static std::wstring Decode(const std::wstring& input);
  static std::wstring Encode(const std::wstring& input);

private:
  hypp::Uri uri_;
};
