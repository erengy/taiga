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

#include <hypp/detail/uri.hpp>
#include <hypp/generator/uri.hpp>
#include <hypp/parser/uri.hpp>
#include <hypp/uri.hpp>
#include <nstd/string.hpp>

#include "base/url.h"

#include "base/log.h"
#include "base/string.h"

Url& Url::operator=(const Url& url) {
  this->uri_ = url.uri_;
  return *this;
}

void Url::operator=(const std::wstring& url) {
  *this = Parse(url);
}

std::wstring Url::host() const {
  return uri_.authority.has_value() ? StrToWstr(uri_.authority->host)
                                    : std::wstring{};
}

Url::query_t Url::query() const {
  if (!uri_.query.has_value()) {
    return {};
  }

  query_t query;

  const auto parameters = nstd::split(uri_.query.value(), "&");
  for (const auto& parameter : parameters) {
    const auto [name, _, value] = nstd::partition(parameter, "=");
    query[Decode(StrToWstr(name))] = Decode(StrToWstr(value));
  }

  return query;
}

hypp::Uri Url::uri() const {
  return uri_;
}

std::wstring Url::to_wstring() const {
  return StrToWstr(hypp::to_string(uri_));
}

Url Url::Parse(std::wstring url, const bool log_errors) {
  Trim(url, L"\t\n\r ");

  const std::string uri = WstrToStr(url);
  hypp::Parser parser{uri};

  if (auto expected = hypp::ParseUri(parser)) {
    return Url{expected.value()};
  } else if (log_errors) {
    LOGE(L"Could not parse URL: {} | Reason: {}", url,
         StrToWstr(hypp::to_string(expected.error())));
  }

  return {};
}

std::wstring Url::Decode(const std::wstring& input) {
  return StrToWstr(hypp::detail::uri::decode(WstrToStr(input)));
}

std::wstring Url::Encode(const std::wstring& input) {
  return StrToWstr(hypp::detail::uri::encode(WstrToStr(input)));
}
