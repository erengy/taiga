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

#include <string>
#include <windows.h>

#include "crypto.h"
#include "string.h"

namespace base {
const wchar_t encryption_char = L'?';
const wchar_t* encryption_key = L"Tenori Taiga";
const size_t encryption_length_min = 16;

std::wstring Xor(std::wstring str, const std::wstring& key) {
  for (size_t i = 0; i < str.size(); i++)
    str[i] = str[i] ^ key[i % key.length()];

  return str;
}

}  // namespace base

std::wstring SimpleEncrypt(std::wstring str) {
  // Set minimum length
  if (str.length() > 0 && str.length() < base::encryption_length_min)
    str.append(base::encryption_length_min - str.length(),
               base::encryption_char);

  // Encrypt
  str = base::Xor(str, base::encryption_key);

  // Convert to hexadecimal string
  std::wstring buffer;
  for (size_t i = 0; i < str.size(); i++) {
    wchar_t c[32] = {'\0'};
    _itow_s(str[i], c, 32, 16);
    if (wcslen(c) == 1)
      buffer.push_back('0');
    buffer += c;
  }
  ToUpper(buffer);

  return buffer;
}

std::wstring SimpleDecrypt(std::wstring str) {
  // Convert from hexadecimal string
  std::wstring buffer;
  for (size_t i = 0; i < str.size(); i = i + 2) {
    wchar_t c = static_cast<wchar_t>(wcstoul(str.substr(i, 2).c_str(),
                                             nullptr, 16));
    buffer.push_back(c);
  }

  // Decrypt
  buffer = base::Xor(buffer, base::encryption_key);

  // Trim characters appended to match the minimum length
  if (buffer.size() >= base::encryption_length_min)
    TrimRight(buffer, std::wstring(1, base::encryption_char).c_str());

  return buffer;
}

////////////////////////////////////////////////////////////////////////////////
// HMAC code is based on "Creating an HMAC" example
// http://msdn.microsoft.com/en-us/library/aa382379%28v=VS.85%29.aspx
//
// Key creation is based on "Crypto wrapper for Microsoft CryptoAPI"
// Copyright (c) 2005-2009, Jouni Malinen
// http://ftp.netbsd.org/pub/NetBSD/NetBSD-current/src/external/bsd/wpa/dist/src/crypto/crypto_cryptoapi.c

std::string HmacSha1(const std::string& key_bytes, const std::string& data) {
  std::string hash;

  HCRYPTPROV hProv = NULL;
  HCRYPTHASH hHash = NULL;
  HCRYPTHASH hHmac = NULL;
  HCRYPTKEY hKey = NULL;
  PBYTE pbHash = nullptr;
  DWORD dwDataLen = 0;

  HMAC_INFO HmacInfo;
  ZeroMemory(&HmacInfo, sizeof(HmacInfo));
  HmacInfo.HashAlgid = CALG_SHA1;

  if (CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
    if (CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
      if (CryptHashData(hHash, (BYTE*)key_bytes.c_str(), key_bytes.size(), 0)) {
        const size_t key_size = 1024;
        struct {
          BLOBHEADER hdr;
          DWORD len;
          BYTE key[key_size];
        } key_blob;
        key_blob.hdr.bType = PLAINTEXTKEYBLOB;
        key_blob.hdr.bVersion = CUR_BLOB_VERSION;
        key_blob.hdr.reserved = 0;
        key_blob.hdr.aiKeyAlg = CALG_RC2;
        key_blob.len = key_bytes.size();
        ZeroMemory(key_blob.key, sizeof(key_blob.key));
        CopyMemory(key_blob.key, key_bytes.c_str(), min(key_bytes.size(), key_size));
        if (CryptImportKey(hProv, (BYTE*)&key_blob, sizeof(key_blob), 0, CRYPT_IPSEC_HMAC_KEY, &hKey)) {
          if (CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHmac)) {
            if (CryptSetHashParam(hHmac, HP_HMAC_INFO, (BYTE*)&HmacInfo, 0)) {
              if (CryptHashData(hHmac, (BYTE*)data.c_str(), data.size(), 0)) {
                if (CryptGetHashParam(hHmac, HP_HASHVAL, nullptr, &dwDataLen, 0)) {
                  pbHash = (BYTE*)malloc(dwDataLen);
                  if (pbHash != nullptr) {
                    if (CryptGetHashParam(hHmac, HP_HASHVAL, pbHash, &dwDataLen, 0)) {
                      for (DWORD i = 0 ; i < dwDataLen ; i++) {
                        hash.push_back((char)pbHash[i]);
  } } } } } } } } } } }

  if (hHmac)
    CryptDestroyHash(hHmac);
  if (hKey)
    CryptDestroyKey(hKey);
  if (hHash)
    CryptDestroyHash(hHash);    
  if (hProv)
    CryptReleaseContext(hProv, 0);
  if (pbHash)
    free(pbHash);

  return hash;
}