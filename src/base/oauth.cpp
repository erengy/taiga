/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include <list>
#include <time.h>
#include <vector>

#include "base64.h"
#include "foreach.h"
#include "crypto.h"
#include "string.h"
#include "oauth.h"
#include "url.h"

#ifndef SIZEOF
#define SIZEOF(x) (sizeof(x) / sizeof(*x))
#endif

////////////////////////////////////////////////////////////////////////////////
// OAuth implementation is based on codebrook-twitter-oauth example code
// Copyright (c) 2010 Brook Miles
// https://code.google.com/p/codebrook-twitter-oauth/

std::wstring OAuth::BuildAuthorizationHeader(
    const std::wstring& url,
    const std::wstring& http_method,
    const oauth_parameter_t* post_parameters,
    const std::wstring& oauth_token,
    const std::wstring& oauth_token_secret,
    const std::wstring& pin) {
  // Build request parameters
  oauth_parameter_t get_parameters = ParseQuery(Url(url).query);

  // Build signed OAuth parameters
  oauth_parameter_t signed_parameters =
      BuildSignedParameters(get_parameters, url, http_method, post_parameters,
                            oauth_token, oauth_token_secret, pin);

  // Build and return OAuth header
  std::wstring oauth_header = L"OAuth ";
  foreach_c_(it, signed_parameters) {
    if (it != signed_parameters.begin())
      oauth_header += L", ";
    oauth_header += it->first + L"=\"" + it->second + L"\"";
  }
  return oauth_header;
}

oauth_parameter_t OAuth::ParseQueryString(const std::wstring& url) {
  oauth_parameter_t parsed_parameters;

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

oauth_parameter_t OAuth::BuildSignedParameters(
    const oauth_parameter_t& get_parameters,
    const std::wstring& url,
    const std::wstring& http_method,
    const oauth_parameter_t* post_parameters,
    const std::wstring& oauth_token,
    const std::wstring& oauth_token_secret,
    const std::wstring& pin) {
  // Create OAuth parameters
  oauth_parameter_t oauth_parameters;
  oauth_parameters[L"oauth_callback"] = L"oob";
  oauth_parameters[L"oauth_consumer_key"] = consumer_key;
  oauth_parameters[L"oauth_nonce"] = CreateNonce();
  oauth_parameters[L"oauth_signature_method"] = L"HMAC-SHA1";
  oauth_parameters[L"oauth_timestamp"] = CreateTimestamp();
  oauth_parameters[L"oauth_version"] = L"1.0";

  // Add the request token
  if (!oauth_token.empty())
    oauth_parameters[L"oauth_token"] = oauth_token;
  // Add the authorization PIN
  if (!pin.empty())
    oauth_parameters[L"oauth_verifier"] = pin;

  // Create a parameter list containing both OAuth and original parameters
  oauth_parameter_t all_parameters = get_parameters;
  if (IsEqual(http_method, L"POST") && post_parameters)
    all_parameters.insert(post_parameters->begin(), post_parameters->end());
  all_parameters.insert(oauth_parameters.begin(), oauth_parameters.end());

  // Prepare the signature base
  std::wstring normal_url = NormalizeUrl(url);
  std::wstring sorted_parameters = SortParameters(all_parameters);
  std::wstring signature_base = http_method +
                                L"&" + EncodeUrl(normal_url) +
                                L"&" + EncodeUrl(sorted_parameters);

  // Obtain a signature and add it to header parameters
  std::wstring signature = CreateSignature(signature_base, oauth_token_secret);
  oauth_parameters[L"oauth_signature"] = signature;

  return oauth_parameters;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring OAuth::CreateNonce() {
  static const wchar_t alphanumeric[] =
      L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::wstring nonce;

  for (int i = 0; i <= 16; ++i)
    nonce += alphanumeric[rand() % (SIZEOF(alphanumeric) - 1)];

  return nonce;
}

std::wstring OAuth::CreateSignature(const std::wstring& signature_base,
                                    const std::wstring& oauth_token_secret) {
  // Create a SHA-1 hash of signature
  std::wstring key = EncodeUrl(consumer_secret) +
                     L"&" + EncodeUrl(oauth_token_secret);
  std::string hash = HmacSha1(WstrToStr(key), WstrToStr(signature_base));

  // Encode signature in Base64
  std::wstring signature = StrToWstr(Base64Encode(hash));

  // Return URL-encoded signature
  return EncodeUrl(signature);
}

std::wstring OAuth::CreateTimestamp() {
  __time64_t utcNow;
  __time64_t ret = _time64(&utcNow);
  wchar_t buf[100] = {'\0'};
  swprintf_s(buf, SIZEOF(buf), L"%I64u", utcNow);
  return buf;
}

std::wstring OAuth::NormalizeUrl(const std::wstring& url) {
  Url url_components = url;

  // Strip off ? and # elements in the path
  url_components.query.clear();
  url_components.fragment.clear();

  // Build normal URL
  std::wstring normal_url = url_components.Build();

  return normal_url;
}

oauth_parameter_t OAuth::ParseQuery(const query_t& query) {
  oauth_parameter_t parameters;

  for (const auto& pair : query)
    parameters[pair.first] = pair.second;

  return parameters;
}

std::wstring OAuth::SortParameters(const oauth_parameter_t& parameters) {
  std::list<std::wstring> sorted;
  for (const auto& pair : parameters) {
    std::wstring param = pair.first + L"=" + pair.second;
    sorted.push_back(param);
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