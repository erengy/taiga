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

#include "file.h"
#include "http.h"
#include "log.h"
#include "string.h"

namespace base {
namespace http {

size_t Client::HeaderFunction(void* ptr, size_t size, size_t nmemb,
                              void* userdata) {
  if (!userdata)
    return 0;

  size_t data_size = size * nmemb;
  std::string header(static_cast<char*>(ptr), data_size);

  auto client = reinterpret_cast<Client*>(userdata);

  if (client->cancel_)
    return 0;

  if (header != "\r\n") {
    client->GetResponseHeader(StrToWstr(header));
  } else {
    if (!client->ParseResponseHeader() || client->OnHeadersAvailable())
      return 0;
  }

  return data_size;
}

size_t Client::WriteFunction(char* ptr, size_t size, size_t nmemb,
                             void* userdata) {
  if (!userdata)
    return 0;

  size_t data_size = size * nmemb;

  auto client = reinterpret_cast<Client*>(userdata);

  if (client->cancel_)
    return 0;

  client->write_buffer_.append(ptr, data_size);

  return data_size;
}

int Client::ProgressFunction(curl_off_t dltotal, curl_off_t dlnow) {
  if (cancel_)
    return 1;  // Abort

  if (response_.header.empty())
    return 0;
  if (!dlnow && !dltotal)
    return 0;
  if (dlnow == current_length_)
    return 0;

  current_length_ = dlnow;

  if (OnProgress())
   return 1;  // Abort

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int Client::DebugHandler(curl_infotype infotype, std::string data,
                         bool simulated_callback) {
  std::wstring text;

  switch (infotype) {
    case CURLINFO_TEXT:
      break;
    case CURLINFO_HEADER_IN:
      text = L"<= Response header";
      break;
    case CURLINFO_HEADER_OUT:
      text = L"=> Request header";
      break;
    case CURLINFO_DATA_IN:
      if (content_encoding_ == ContentEncoding::Gzip && !simulated_callback)
        return 0;  // Don't output garbage
      text = L"<= Recv data";
      break;
    case CURLINFO_DATA_OUT:
      text = L"=> Send data";
      break;
    case CURLINFO_SSL_DATA_IN:
      text = L"<= Recv SSL data";
      break;
    case CURLINFO_SSL_DATA_OUT:
      text = L"=> Send SSL data";
      break;
    default:
      return 0;
  }
  if (!text.empty())
    text += L" | ";
  if (simulated_callback)
    text += L"<simulated> | ";

  auto wdata = StrToWstr(data);
  Trim(wdata, L" \r\n");

  LOGD(text + wdata);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

int Client::DebugFunction(CURL* curl, curl_infotype infotype, char* data,
                          size_t size, void* client) {
  if (!client)
    return 0;

  return reinterpret_cast<Client*>(client)->DebugHandler(
      infotype, std::string(data, size), false);
}

int Client::XferInfoFunction(void* clientp,
                             curl_off_t dltotal, curl_off_t dlnow,
                             curl_off_t ultotal, curl_off_t ulnow) {
  if (!clientp)
    return 0;

  return reinterpret_cast<Client*>(clientp)->ProgressFunction(dltotal, dlnow);
}

}  // namespace http
}  // namespace base