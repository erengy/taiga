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
#include <optional>
#include <string>

class Url;

namespace oauth {

struct Access {
  std::wstring token;
  std::wstring secret;
  std::wstring pin;
};

struct Consumer {
  std::wstring key;
  std::wstring secret;
};

using Parameters = std::map<std::wstring, std::wstring>;

std::wstring BuildAuthorizationHeader(
    const std::wstring& url,
    const std::wstring& http_method,
    const Consumer& consumer,
    const Access& access = {},
    const std::optional<Parameters>& post_parameters = std::nullopt);

Parameters ParseQueryString(const std::wstring& url);

namespace detail {

Parameters BuildSignedParameters(
    const std::wstring& url,
    const std::wstring& http_method,
    const Consumer& consumer,
    const Access& access,
    const Parameters& get_parameters,
    const std::optional<Parameters>& post_parameters);

std::wstring CreateNonce();
std::wstring CreateSignature(
    const std::wstring& consumer_secret,
    const std::wstring& signature_base,
    const std::wstring& access_token_secret);
std::wstring CreateTimestamp();

std::wstring NormalizeUrl(const std::wstring& url);
Parameters ParseQuery(const Url& url);
std::wstring SortParameters(const Parameters& parameters);

}  // namespace detail

}  // namespace oauth
