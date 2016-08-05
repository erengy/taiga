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

#include "foreach.h"
#include "http.h"
#include "string.h"
#include "url.h"

namespace base {
namespace http {

bool Client::GetResponseHeader(const std::wstring& header) {
  if (header.empty())
    return false;

  std::vector<std::wstring> lines;
  Split(header, L"\r\n", lines);

  foreach_(line, lines) {
    int pos = InStr(*line, L":", 0);
    if (pos == -1) {
      if (StartsWith(*line, L"HTTP/"))
        response_.code = ToInt(InStr(*line, L" ", L" "));
    } else {
      std::wstring name = line->substr(0, pos);
      std::wstring value = line->substr(pos + 1);
      TrimLeft(value);
      // Using insert function instead of operator[] to avoid overwriting
      // previous header fields.
      response_.header.insert(std::make_pair(name, value));
    }
  }

  if (!response_.code)
    return false;

  return true;
}

bool Client::ParseResponseHeader() {
  Url location;
  bool refresh = false;

  foreach_(it, response_.header) {
    std::wstring name = it->first;
    std::wstring value = it->second;

    if (IsEqual(name, L"Content-Encoding")) {
      if (InStr(value, L"gzip") > -1) {
        content_encoding_ = kContentEncodingGzip;
      } else {
        content_encoding_ = kContentEncodingNone;
      }

    } else if (IsEqual(name, L"Content-Length")) {
      content_length_ = ToInt(value);

    } else if (IsEqual(name, L"Location") || IsEqual(name, L"Refresh")) {
      refresh = IsEqual(name, L"Refresh");
      if (refresh) {
        const std::wstring url_field = L"url=";
        size_t pos = value.find(url_field);
        if (pos != value.npos)
          value = value.substr(pos + url_field.size());
      }
      location.Crack(value);
      if (location.host.empty())  // relative URL
        location.host = request_.url.host;
      if (OnRedirect(location.Build(), refresh))
        return false;
    }
  }

  // Redirection
  if (!location.host.empty() && auto_redirect_ && !refresh) {
    content_encoding_ = kContentEncodingNone;
    content_length_ = 0;
    current_length_ = 0;
    request_.url.host = location.host;
    request_.url.path = location.path;
    response_.Clear();
    // cURL will automatically follow due to CURLOPT_FOLLOWLOCATION
  }

  return true;
}

}  // namespace http
}  // namespace base