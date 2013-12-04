/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#include "win_http.h"
#include "base/gzip.h"
#include "base/string.h"

namespace win {
namespace http {

void CALLBACK Client::Callback(HINTERNET hInternet,
                               DWORD_PTR dwContext,
                               DWORD dwInternetStatus,
                               LPVOID lpvStatusInformation,
                               DWORD dwStatusInformationLength) {
  auto object = reinterpret_cast<Client*>(dwContext);
  if (object) {
    object->StatusCallback(hInternet,
                           dwInternetStatus,
                           lpvStatusInformation,
                           dwStatusInformationLength);
  }
}

void Client::StatusCallback(HINTERNET hInternet,
                            DWORD dwInternetStatus,
                            LPVOID lpvStatusInformation,
                            DWORD dwStatusInformationLength) {
  switch (dwInternetStatus) {
    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE: {
      if (::WinHttpReceiveResponse(hInternet, nullptr))
        OnSendRequestComplete();
      break;
    }

    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE: {
      wstring buffer;
      DWORD buffer_length = 0;
      if (!::WinHttpQueryHeaders(hInternet,
                                 WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                 WINHTTP_HEADER_NAME_BY_INDEX,
                                 WINHTTP_NO_OUTPUT_BUFFER,
                                 &buffer_length,
                                 WINHTTP_NO_HEADER_INDEX)) {
        DWORD error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER) {
          OnError(error);
          break;
        }
      }
      buffer.resize(buffer_length, L'\0');
      if (::WinHttpQueryHeaders(hInternet,
                                WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                WINHTTP_HEADER_NAME_BY_INDEX,
                                (LPVOID)buffer.data(),
                                &buffer_length,
                                WINHTTP_NO_HEADER_INDEX)) {
        if (!GetResponseHeader(buffer) ||
            !ParseResponseHeader() ||
            OnHeadersAvailable()) {
          break;
        }
      }
      ::WinHttpQueryDataAvailable(hInternet, nullptr);
      break;
    }

    case WINHTTP_CALLBACK_STATUS_REDIRECT: {
      StatusCallback(hInternet, WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE,
                     lpvStatusInformation, dwStatusInformationLength);
      break;
    }

    case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE: {
      DWORD dwSize = *reinterpret_cast<LPDWORD>(lpvStatusInformation);
      if (dwSize > 0) {
        LPSTR lpOutBuffer = new char[dwSize + 1];
        ::ZeroMemory(lpOutBuffer, dwSize + 1);
        if (!::WinHttpReadData(hInternet, (LPVOID)lpOutBuffer, dwSize, nullptr)) {
          delete [] lpOutBuffer;
          break;
        }
        if (OnDataAvailable()) {
          Cleanup();
        }
      } else if (dwSize == 0) {
        if (content_encoding_ == kContentEncodingGzip) {
          string input, output;
          input.append(buffer_, current_length_);
          UncompressGzippedString(input, output);
          if (!output.empty()) {
            delete [] buffer_;
            current_length_ = output.length();
            buffer_ = new char[current_length_];
            memcpy(buffer_, &output[0], current_length_);
          }
        }
        if (!download_path_.empty()) {
          HANDLE hFile = ::CreateFile(download_path_.c_str(), GENERIC_WRITE,
                                      0, nullptr, CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL, nullptr);
          if (hFile != INVALID_HANDLE_VALUE) {
            DWORD bytes_to_write = current_length_;
            DWORD bytes_written = 0;
            ::WriteFile(hFile, (LPCVOID)buffer_, bytes_to_write,
                        &bytes_written, nullptr);
            ::CloseHandle(hFile);
          }
        }
        response_.body = StrToWstr(buffer_);
        if (!OnReadComplete()) {
          Cleanup();
        }
      }
      break;
    }

    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE: {
      if (dwStatusInformationLength > 0) {
        LPSTR lpReadBuffer = (LPSTR)lpvStatusInformation;
        DWORD dwBytesRead = dwStatusInformationLength;
        if (!buffer_) {
          buffer_ = lpReadBuffer;
        } else {
          LPSTR lpOldBuffer = buffer_;
          buffer_ = new char[current_length_ + dwBytesRead];
          memcpy(buffer_, lpOldBuffer, current_length_);
          memcpy(buffer_ + current_length_, lpReadBuffer, dwBytesRead);
          delete [] lpOldBuffer;
          delete [] lpReadBuffer;
        }
        current_length_ += dwBytesRead;
        if (OnReadData()) {
          Cleanup();
        } else {
          ::WinHttpQueryDataAvailable(hInternet, nullptr);
        }
      }
      break;
    }

    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR: {
      auto result = reinterpret_cast<WINHTTP_ASYNC_RESULT*>(lpvStatusInformation);
      switch (result->dwResult) {
        case API_RECEIVE_RESPONSE:
        case API_QUERY_DATA_AVAILABLE:
        case API_READ_DATA:
        case API_WRITE_DATA:
        case API_SEND_REQUEST:
          OnError(result->dwError);
          break;
      }
      break;
    }
  }
}

}  // namespace http
}  // namespace win