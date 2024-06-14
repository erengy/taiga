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

#include <functional>
#include <string_view>

#include <hypp.hpp>
#include <hypr.hpp>

namespace taiga::http {

using Error = hypr::Error;
using Request = hypr::Request;
using Response = hypr::Response;
using Transfer = hypr::detail::Transfer;
using Uri = hypp::Uri;

using ResponseCallback = std::function<void(const Response&)>;
using TransferCallback = std::function<bool(const Transfer&)>;

void Init();
void ProcessQueue();
void Shutdown();

void Send(const Request& request,
          const TransferCallback& on_transfer,
          const ResponseCallback& on_response);

namespace util {

std::wstring GetUrlHost(const Uri& uri);
std::wstring GetUrlHost(const std::string_view url);
bool IsDdosProtectionEnabled(const Response& response);
std::wstring to_string(const Error& error, const std::wstring& host);
std::wstring to_string(const Transfer& transfer);

}  // namespace util

}  // namespace taiga::http
