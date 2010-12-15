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

#ifndef OAUTH_H
#define OAUTH_H

#include <map>
#include <string>

using std::string;
using std::wstring;

typedef std::map<wstring, wstring> HTTPParameters;

// =============================================================================

class COAuth {
public:
  COAuth() {}
  virtual ~COAuth() {}

  wstring ConsumerKey, ConsumerSecret;

  wstring BuildHeader(
    const wstring& url, 
    const wstring& http_method, 
    const HTTPParameters* post_parameters = NULL, 
    const wstring& oauth_token = L"", 
    const wstring& oauth_token_secret = L"", 
    const wstring& pin = L"");
  HTTPParameters ParseQueryString(const wstring& url);

private:
  HTTPParameters BuildSignedParameters(
    const HTTPParameters& get_parameters, 
    const wstring& url, 
    const wstring& http_method, 
    const HTTPParameters* post_parameters, 
    const wstring& oauth_token, 
    const wstring& oauth_token_secret, 
    const wstring& pin);
  wstring CreateNonce();
  wstring CreateSignature(const wstring& signature_base, const wstring& oauth_token_secret);
  wstring CreateTimestamp();
  string Crypt_HMACSHA1(const string& keyBytes, const string& data);
  wstring NormalizeURL(const wstring& url);
  wstring SortParameters(const HTTPParameters& parameters);
  wstring UrlGetQuery(const wstring& url);
};

#endif // OAUTH_H