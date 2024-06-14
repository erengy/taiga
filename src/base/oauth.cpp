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

#include <array>
#include <chrono>
#include <list>
#include <string>
#include <string_view>
#include <vector>

#include "base/oauth.h"

#include "base/base64.h"
#include "base/crypto.h"
#include "base/string.h"
#include "base/url.h"

////////////////////////////////////////////////////////////////////////////////
// OAuth implementation is based on codebrook-twitter-oauth example code
// Copyright (c) 2010 Brook Miles
// https://code.google.com/p/codebrook-twitter-oauth/

namespace oauth {

std::wstring BuildAuthorizationHeader(
    const std::wstring& url,
    const std::wstring& http_method,
    const Consumer& consumer,
    const Access& access,
    const std::optional<Parameters>& post_parameters) {
  // Build request parameters
  const Parameters get_parameters = detail::ParseQuery(Url(url));

  // Build signed OAuth parameters
  const Parameters signed_parameters = detail::BuildSignedParameters(
      url, http_method, consumer, access, get_parameters, post_parameters);

  // Build and return OAuth header
  std::wstring oauth_header = L"OAuth ";
  for (auto it = signed_parameters.begin();
       it != signed_parameters.end(); ++it) {
    if (it != signed_parameters.begin())
      oauth_header += L", ";
    oauth_header += it->first + L"=\"" + it->second + L"\"";
  }
  return oauth_header;
}

Parameters ParseQueryString(const std::wstring& url) {
  Parameters parsed_parameters;

  std::vector<std::wstring> parameters;
  Split(url, L"&", parameters);

  for (const auto& parameter : parameters) {
    std::vector<std::wstring> elements;
    Split(parameter, L"=", elements);
    if (elements.size() == 2) {
      parsed_parameters[elements[0]] = elements[1];
    }
  }

  return parsed_parameters;
}

////////////////////////////////////////////////////////////////////////////////

namespace detail {

Parameters BuildSignedParameters(
    const std::wstring& url,
    const std::wstring& http_method,
    const Consumer& consumer,
    const Access& access,
    const Parameters& get_parameters,
    const std::optional<Parameters>& post_parameters) {
  // Create OAuth parameters
  Parameters oauth_parameters;
  oauth_parameters[L"oauth_callback"] = L"oob";
  oauth_parameters[L"oauth_consumer_key"] = consumer.key;
  oauth_parameters[L"oauth_nonce"] = CreateNonce();
  oauth_parameters[L"oauth_signature_method"] = L"HMAC-SHA1";
  oauth_parameters[L"oauth_timestamp"] = CreateTimestamp();
  oauth_parameters[L"oauth_version"] = L"1.0";

  // Add the request token
  if (!access.token.empty())
    oauth_parameters[L"oauth_token"] = access.token;
  // Add the authorization PIN
  if (!access.pin.empty())
    oauth_parameters[L"oauth_verifier"] = access.pin;

  // Create a parameter list containing both OAuth and original parameters
  Parameters all_parameters = get_parameters;
  if (IsEqual(http_method, L"POST") && post_parameters)
    all_parameters.insert(post_parameters->begin(), post_parameters->end());
  all_parameters.insert(oauth_parameters.begin(), oauth_parameters.end());

  // Prepare the signature base
  const std::wstring signature_base = http_method +
      L"&" + Url::Encode(NormalizeUrl(url)) +
      L"&" + Url::Encode(SortParameters(all_parameters));

  // Obtain a signature and add it to header parameters
  oauth_parameters[L"oauth_signature"] =
      CreateSignature(consumer.secret, signature_base, access.secret);

  return oauth_parameters;
}

std::wstring CreateNonce() {
  constexpr std::wstring_view alphanumeric =
      L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::wstring nonce;

  for (int i = 0; i <= 16; ++i) {
    nonce += alphanumeric[rand() % (alphanumeric.size() - 1)];
  }

  return nonce;
}

std::wstring CreateSignature(
    const std::wstring& consumer_secret,
    const std::wstring& signature_base,
    const std::wstring& access_token_secret) {
  // Create a SHA-1 hash of signature
  const std::wstring key =
      Url::Encode(consumer_secret) + L"&" + Url::Encode(access_token_secret);
  const std::string hash = HmacSha1(WstrToStr(key), WstrToStr(signature_base));

  // Encode signature in Base64
  const std::wstring signature = StrToWstr(Base64Encode(hash));

  // Return URL-encoded signature
  return Url::Encode(signature);
}

std::wstring CreateTimestamp() {
  const auto utc_now = std::chrono::system_clock::now();
  const auto time_since_epoch =
      std::chrono::duration_cast<std::chrono::seconds>(
          utc_now.time_since_epoch());
  return std::to_wstring(time_since_epoch.count());
}

std::wstring NormalizeUrl(const std::wstring& url) {
  auto uri = Url(url).uri();

  // Strip off ? and # elements in the path
  uri.query.reset();
  uri.fragment.reset();

  // Build and return normal URL
  return Url(uri).to_wstring();
}

Parameters ParseQuery(const Url& url) {
  Parameters parameters;

  const auto query = url.query();
  for (const auto& pair : query) {
    parameters[pair.first] = pair.second;
  }

  return parameters;
}

std::wstring SortParameters(const Parameters& parameters) {
  std::list<std::wstring> sorted;
  for (const auto& [name, value] : parameters) {
    sorted.push_back(name + L"=" + value);
  }
  sorted.sort();

  std::wstring params;
  for (const auto& param : sorted) {
    if (params.size() > 0)
      params += L"&";
    params += param;
  }
  return params;
}

}  // namespace detail

}  // namespace oauth
