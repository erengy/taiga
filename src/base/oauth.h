/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#ifndef TAIGA_THIRD_PARTY_OAUTH_H
#define TAIGA_THIRD_PARTY_OAUTH_H

#include <map>
#include <string>

typedef std::map<std::wstring, std::wstring> OAuthParameters;

class OAuth {
public:
  OAuth() {}
  ~OAuth() {}

  std::wstring BuildAuthorizationHeader(
      const std::wstring& url,
      const std::wstring& http_method,
      const OAuthParameters* post_parameters = nullptr,
      const std::wstring& oauth_token = L"",
      const std::wstring& oauth_token_secret = L"",
      const std::wstring& pin = L"");
  OAuthParameters ParseQueryString(const std::wstring& url);

  std::wstring consumer_key;
  std::wstring consumer_secret;

private:
  OAuthParameters BuildSignedParameters(
      const OAuthParameters& get_parameters,
      const std::wstring& url,
      const std::wstring& http_method,
      const OAuthParameters* post_parameters,
      const std::wstring& oauth_token,
      const std::wstring& oauth_token_secret,
      const std::wstring& pin);

  std::wstring CreateNonce();
  std::wstring CreateSignature(const std::wstring& signature_base, const std::wstring& oauth_token_secret);
  std::wstring CreateTimestamp();

  std::wstring NormalizeUrl(const std::wstring& url);
  std::wstring SortParameters(const OAuthParameters& parameters);
  std::wstring UrlGetQuery(const std::wstring& url);
};

#endif  // TAIGA_THIRD_PARTY_OAUTH_H