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

#include <algorithm>
#include <string>
#include <windows.h>

#include "base/crypto.h"

#include "base/base64.h"
#include "base/gzip.h"
#include "base/string.h"

StringCoder::StringCoder()
    : magic_string_("TAI"),
      min_length_(3 + 1 + 2 + 2 + 2),
      version_(0x01) {
}

bool StringCoder::Encode(const std::wstring& metadata, const std::wstring& data,
                         std::wstring& output) {
  if (data.empty())
    return false;

  std::string converted_metadata = WstrToStr(metadata);
  std::string converted_data = WstrToStr(data);

  std::string compressed_data;
  if (!DeflateString(converted_data, compressed_data))
    return false;

  std::string buffer;
  buffer.append(magic_string_);
  buffer.push_back(version_);
  buffer.append(ConvertSizeToString(
      static_cast<unsigned short>(converted_metadata.size())));
  buffer.append(converted_metadata);
  buffer.append(ConvertSizeToString(
      static_cast<unsigned short>(compressed_data.size())));
  buffer.append(ConvertSizeToString(
      static_cast<unsigned short>(converted_data.size())));
  buffer.append(compressed_data);

  output = StrToWstr(Base64Encode(buffer));

  return true;
}

bool StringCoder::Decode(const std::wstring& input, std::wstring& metadata,
                         std::wstring& data) {
  if (input.empty())
    return false;

  std::string decoded = Base64Decode(WstrToStr(input));

  if (decoded.size() < min_length_)
    return false;

  size_t pos = magic_string_.size();

  if (decoded.substr(0, pos) != magic_string_)
    return false;

  if (decoded.at(pos++) != version_)
    return false;

  unsigned short metadata_size = ReadSize(decoded, pos);
  pos += sizeof(metadata_size);
  if (metadata_size > 0) {
    metadata = StrToWstr(decoded.substr(pos, metadata_size));
    pos += metadata_size;
  }

  unsigned short data_size = ReadSize(decoded, pos);
  pos += sizeof(data_size);
  unsigned short inflated_data_size = ReadSize(decoded, pos);
  pos += sizeof(inflated_data_size);
  if (data_size > 0 && inflated_data_size > 0) {
    decoded = decoded.substr(pos, data_size);
  } else {
    return false;
  }

  std::string inflated;
  if (!InflateString(decoded, inflated, inflated_data_size))
    return false;

  data = StrToWstr(inflated);

  return true;
}

std::string StringCoder::ConvertSizeToString(unsigned short value) {
  std::string output;

  for (size_t i = 0; i < sizeof(value); i++) {
    output.push_back(static_cast<char>((value >> (i * 8)) & 0xFF));
  }

  return output;
}

unsigned short StringCoder::ReadSize(const std::string& str, size_t pos) {
  unsigned short result = 0;
  size_t len = sizeof(result);

  if (pos + len > str.size())
    return 0;

  for (size_t n = pos + len; n > pos; n--) {
    result = (result << 8) + static_cast<unsigned char>(str.at(n - 1));
  }

  return result;
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
      if (CryptHashData(hHash, (BYTE*)key_bytes.c_str(), (DWORD)key_bytes.size(), 0)) {
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
        key_blob.len = (DWORD)key_bytes.size();
        ZeroMemory(key_blob.key, sizeof(key_blob.key));
        CopyMemory(key_blob.key, key_bytes.c_str(), std::min(key_bytes.size(), key_size));
        if (CryptImportKey(hProv, (BYTE*)&key_blob, sizeof(key_blob), 0, CRYPT_IPSEC_HMAC_KEY, &hKey)) {
          if (CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHmac)) {
            if (CryptSetHashParam(hHmac, HP_HMAC_INFO, (BYTE*)&HmacInfo, 0)) {
              if (CryptHashData(hHmac, (BYTE*)data.c_str(), (DWORD)data.size(), 0)) {
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
