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

#include <windows.h>

#include <zlib/zlib.h>

#include "base/gzip.h"

bool UncompressGzippedString(const std::string& input, std::string& output) {
  z_stream stream;
  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = NULL;
  stream.total_out = 0;
  stream.next_in = (BYTE*)&input[0];
  stream.avail_in = (uInt)input.length();

  if (inflateInit2(&stream, MAX_WBITS + 32) != Z_OK)
    return false;

  size_t buffer_length = input.length() * 2;
  char* buffer = (char*)GlobalAlloc(GMEM_ZEROINIT, buffer_length);

  size_t total_length = 0;
  int status = Z_OK;

  do {
    stream.next_out = (BYTE*)buffer;
    stream.avail_out = (uInt)buffer_length;
    status = inflate(&stream, Z_SYNC_FLUSH);
    if (status == Z_OK || status == Z_STREAM_END) {
      output.append(buffer, stream.total_out - total_length);
      total_length = stream.total_out;
    }
  } while (status == Z_OK);

  GlobalFree(buffer);
  inflateEnd(&stream);

  return status == Z_STREAM_END;
}

////////////////////////////////////////////////////////////////////////////////

bool DeflateString(const std::string& input, std::string& output) {
  uLong source_length = (uLong)input.length();
  uLong destination_length = compressBound(source_length);

  output.resize(destination_length);

  int result = compress((Bytef*)output.data(), &destination_length,
                        (Bytef*)input.data(), source_length);

  if (output.length() != destination_length)
    output.resize(destination_length);

  return result == Z_OK;
}

bool InflateString(const std::string& input, std::string& output,
                   size_t output_length) {
  uLong source_length = (uLong)input.length();
  uLong destination_length = (uLong)output_length;

  output.resize(destination_length);

  int result = uncompress((Bytef*)output.data(), &destination_length,
                          (Bytef*)input.data(), source_length);

  if (output.length() != destination_length)
    output.resize(destination_length);

  return result == Z_OK;
}
