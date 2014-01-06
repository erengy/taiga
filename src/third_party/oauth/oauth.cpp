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

#include <windows.h>
#include <winhttp.h>
#include <time.h>
#include <list>
#include <vector>

#include "oauth.h"

#include "base/encoding.h"
#include "base/string.h"
#include "third_party/base64/base64.h"

#ifndef SIZEOF
#define SIZEOF(x) (sizeof(x)/sizeof(*x))
#endif

std::wstring OAuth::BuildAuthorizationHeader(
  const std::wstring& url,
  const std::wstring& http_method,
  const OAuthParameters* post_parameters,
  const std::wstring& oauth_token,
  const std::wstring& oauth_token_secret,
  const std::wstring& pin)
{
  // Build request parameters
  OAuthParameters get_parameters = ParseQueryString(UrlGetQuery(url));

  // Build signed OAuth parameters
  OAuthParameters signed_parameters =
      BuildSignedParameters(get_parameters, url, http_method, post_parameters,
                            oauth_token, oauth_token_secret, pin);

  // Build and return OAuth header
  std::wstring oauth_header = L"OAuth ";
  for (OAuthParameters::const_iterator it = signed_parameters.begin();
       it != signed_parameters.end(); ++it) {
    if (it != signed_parameters.begin())
      oauth_header += L", ";
    oauth_header += it->first + L"=\"" + it->second + L"\"";
  }
  oauth_header += L"\r\n";
  return oauth_header;
}

OAuthParameters OAuth::ParseQueryString(const std::wstring& url) {
  OAuthParameters parsed_parameters;

  std::vector<std::wstring> parameters;
  Split(url, L"&", parameters);

  for (size_t i = 0; i < parameters.size(); ++i) {
    std::vector<std::wstring> elements;
    Split(parameters[i], L"=", elements);
    if (elements.size() == 2) {
      parsed_parameters[elements[0]] = elements[1];
    }
  }

  return parsed_parameters;
}

////////////////////////////////////////////////////////////////////////////////

OAuthParameters OAuth::BuildSignedParameters(
  const OAuthParameters& get_parameters,
  const std::wstring& url,
  const std::wstring& http_method,
  const OAuthParameters* post_parameters,
  const std::wstring& oauth_token,
  const std::wstring& oauth_token_secret,
  const std::wstring& pin)
{
  // Create OAuth parameters
  OAuthParameters oauth_parameters;
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
  OAuthParameters all_parameters = get_parameters;
  if (CompareStrings(http_method, L"POST") == 0 && post_parameters)
    all_parameters.insert(post_parameters->begin(), post_parameters->end());
  all_parameters.insert(oauth_parameters.begin(), oauth_parameters.end());

  // Prepare the signature base
  std::wstring normal_url = NormalizeURL(url);
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
  std::string hash = Crypt_HMACSHA1(WstrToStr(key), WstrToStr(signature_base));

  // Encode signature in Base64
  Base64Coder coder;
  coder.Encode((BYTE*)hash.c_str(), hash.size());
  std::wstring signature = StrToWstr(coder.EncodedMessage());

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

std::wstring OAuth::NormalizeURL(const std::wstring& url) {
  wchar_t scheme[1024 * 4] = {'\0'};
  wchar_t host[1024 * 4] = {'\0'};
  wchar_t path[1024 * 4] = {'\0'};

  URL_COMPONENTS components = {sizeof(URL_COMPONENTS)};
  components.lpszScheme = scheme;
  components.dwSchemeLength = SIZEOF(scheme);
  components.lpszHostName = host;
  components.dwHostNameLength = SIZEOF(host);
  components.lpszUrlPath = path;
  components.dwUrlPathLength = SIZEOF(path);

  std::wstring normal_url = url;
  if (::WinHttpCrackUrl(url.c_str(), url.size(), 0, &components)) {
    // Include port number if it is non-standard
    wchar_t port[10] = {};
    if ((CompareStrings(scheme, L"http") == 0 && components.nPort != 80) ||
        (CompareStrings(scheme, L"https") == 0 && components.nPort != 443)) {
      swprintf_s(port, SIZEOF(port), L":%u", components.nPort);
    }
    // Strip off ? and # elements in the path
    std::wstring path_only = path;
    std::wstring::size_type pos = path_only.find_first_of(L"#?");
    if (pos != std::wstring::npos)
      path_only = path_only.substr(0, pos);
    // Build normal URL
    normal_url = std::wstring(scheme) + L"://" + host + port + path_only;
  }
  return normal_url;
}

std::wstring OAuth::SortParameters(const OAuthParameters& parameters) {
  std::list<std::wstring> sorted;
  for (OAuthParameters::const_iterator it = parameters.begin();
       it != parameters.end(); ++it) {
    std::wstring param = it->first + L"=" + it->second;
    sorted.push_back(param);
  }
  sorted.sort();

  std::wstring params;
  for (std::list<std::wstring>::iterator it = sorted.begin();
       it != sorted.end(); ++it) {
    if (params.size() > 0)
      params += L"&";
    params += *it;
  }
  return params;
}

std::wstring OAuth::UrlGetQuery(const std::wstring& url) {
  std::wstring query;
  wchar_t buf[1024 * 4] = {'\0'};
  
  URL_COMPONENTS components = {sizeof(URL_COMPONENTS)};
  components.dwExtraInfoLength = SIZEOF(buf);
  components.lpszExtraInfo = buf;

  if (::WinHttpCrackUrl(url.c_str(), url.size(), 0, &components)) {
    query = components.lpszExtraInfo;
    std::wstring::size_type q = query.find_first_of(L'?');
    if (q != std::wstring::npos)
      query = query.substr(q + 1);
    std::wstring::size_type h = query.find_first_of(L'#');
    if (h != std::wstring::npos)
      query = query.substr(0, h);
  }

  return query;
}

////////////////////////////////////////////////////////////////////////////////
// HMAC (Hashed Message Authentication Checksum) code is based on:
// http://msdn.microsoft.com/en-us/library/aa382379%28v=VS.85%29.aspx

// Key creation is based on:
// http://mirror.leaseweb.com/NetBSD/NetBSD-release-5-0/src/dist/wpa/src/crypto/crypto_cryptoapi.c

std::string OAuth::Crypt_HMACSHA1(const std::string& key_bytes,
                                  const std::string& data) {
  std::string hash;
  HCRYPTPROV hProv = NULL;
  HCRYPTHASH hHash = NULL;
  HCRYPTKEY hKey = NULL;
  HCRYPTHASH hHmacHash = NULL;
  PBYTE pbHash = NULL;
  DWORD dwDataLen = 0;
  HMAC_INFO HmacInfo;
  ZeroMemory(&HmacInfo, sizeof(HmacInfo));
  HmacInfo.HashAlgid = CALG_SHA1;

  if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
    if (CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
      if (CryptHashData(hHash, (BYTE*)key_bytes.c_str(), key_bytes.size(), 0)) {
        struct {
          BLOBHEADER hdr;
          DWORD len;
          BYTE key[1024];
        } key_blob;
        key_blob.hdr.bType = PLAINTEXTKEYBLOB;
        key_blob.hdr.bVersion = CUR_BLOB_VERSION;
        key_blob.hdr.reserved = 0;
        key_blob.hdr.aiKeyAlg = CALG_RC2;
        key_blob.len = key_bytes.size();
        ZeroMemory(key_blob.key, sizeof(key_blob.key));
        CopyMemory(key_blob.key, key_bytes.c_str(), min(key_bytes.size(), SIZEOF(key_blob.key)));
        if (CryptImportKey(hProv, (BYTE*)&key_blob, sizeof(key_blob), 0, CRYPT_IPSEC_HMAC_KEY, &hKey)) {
          if (CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHmacHash)) {
            if (CryptSetHashParam(hHmacHash, HP_HMAC_INFO, (BYTE*)&HmacInfo, 0)) {
              if (CryptHashData(hHmacHash, (BYTE*)data.c_str(), data.size(), 0)) {
                if (CryptGetHashParam(hHmacHash, HP_HASHVAL, NULL, &dwDataLen, 0)) {
                  pbHash = (BYTE*)malloc(dwDataLen);
                  if (pbHash != NULL) {
                    if (CryptGetHashParam(hHmacHash, HP_HASHVAL, pbHash, &dwDataLen, 0)) {
                      for (DWORD i = 0 ; i < dwDataLen ; i++) {
                        hash.push_back((char)pbHash[i]);
  } } } } } } } } } } }

  if (hHmacHash) CryptDestroyHash(hHmacHash);
  if (hKey) CryptDestroyKey(hKey);
  if (hHash) CryptDestroyHash(hHash);    
  if (hProv) CryptReleaseContext(hProv, 0);
  if (pbHash) free(pbHash);

  return hash;
}