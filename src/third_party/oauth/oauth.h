/*
** Copyright (c) 2010 Brook Miles
** 
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
** 
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
** 
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
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
  std::string Crypt_HMACSHA1(const std::string& key_bytes, const std::string& data);
  std::wstring NormalizeURL(const std::wstring& url);
  std::wstring SortParameters(const OAuthParameters& parameters);
  std::wstring UrlGetQuery(const std::wstring& url);
};

#endif  // TAIGA_THIRD_PARTY_OAUTH_H