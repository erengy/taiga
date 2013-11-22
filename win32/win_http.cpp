/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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
#include "../common.h"
#include "../string.h"

namespace win32 {

// =============================================================================

Url& Url::operator=(const Url& url) {
  Scheme = url.Scheme;
  Host = url.Host;
  Path = url.Path;

  return *this;
}

void Url::operator=(const wstring& url) {
  Crack(url);
}

void Url::Crack(wstring url) {
  // Get scheme
  size_t i = url.find(L"://", 0);
  if (i != wstring::npos) {
    Scheme = url.substr(0, i);
    url = url.substr(i + 3);
  } else {
    Scheme.clear();
  }
  
  // Get host and path
  i = url.find(L"/", 0);
  if (i == wstring::npos) i = url.length();
  Host = url.substr(0, i);
  Path = url.substr(i);
}

// =============================================================================

bool Http::Connect(wstring szServer, wstring szObject, wstring szData, wstring szVerb, wstring szHeader, 
                    wstring szReferer, wstring szFile, DWORD dwClientMode, LPARAM lParam) {
  // Close previous connection, if any
  Cleanup();

  // Set new information
  m_OptionalData = ToANSI(szData);
  m_dwClientMode = dwClientMode;
  m_lParam = lParam;
  m_File = szFile;
  m_Referer = szReferer;
  m_Verb = szVerb;

  // Prepare file location
  if (!szFile.empty()) {
    CreateFolder(GetPathOnly(szFile));
  }
    
  // Build request header
  m_RequestHeader = szHeader;
  wstring header = BuildRequestHeader(szHeader);

  // Last chance to modify things
  OnInitialize();

  // Create a session handle
  m_hSession = ::WinHttpOpen(m_UserAgent.c_str(), 
    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
    WINHTTP_NO_PROXY_NAME, 
    WINHTTP_NO_PROXY_BYPASS, 
    WINHTTP_FLAG_ASYNC);
  if (!m_hSession) {
    Cleanup();
    OnError(::GetLastError());
    return false;
  }
  
  // Open an HTTP session
  m_hConnect = ::WinHttpConnect(m_hSession, 
    szServer.c_str(), 
    INTERNET_DEFAULT_HTTP_PORT, 
    0);
  if (!m_hConnect) {
    Cleanup();
    OnError(::GetLastError());
    return false;
  }

  // Open a GET/POST request
  m_hRequest = ::WinHttpOpenRequest(m_hConnect, 
    szVerb.c_str(), 
    szObject.c_str(), 
    NULL, 
    szReferer.c_str(), 
    WINHTTP_DEFAULT_ACCEPT_TYPES, 
    0);
  if (!m_hRequest) {
    Cleanup();
    OnError(::GetLastError());
    return false;
  }

  // Setup proxy
  if (!m_Proxy.empty()) {
    WINHTTP_PROXY_INFO pi = {0};
    pi.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
    pi.lpszProxy = const_cast<LPWSTR>(m_Proxy.c_str());
    ::WinHttpSetOption(m_hRequest, WINHTTP_OPTION_PROXY, &pi, sizeof(pi));
    if (!m_ProxyUser.empty()) {
      ::WinHttpSetOption(m_hRequest, WINHTTP_OPTION_PROXY_USERNAME, 
        (LPVOID)m_ProxyUser.c_str(), m_ProxyUser.size() * sizeof(wchar_t));
      if (!m_ProxyPass.empty()) {
        ::WinHttpSetOption(m_hRequest, WINHTTP_OPTION_PROXY_PASSWORD, 
          (LPVOID)m_ProxyPass.c_str(), m_ProxyPass.size() * sizeof(wchar_t));
      }
    }
  }

  // Disable auto-redirect
  if (m_AutoRedirect == FALSE) {
    DWORD dwDisable = WINHTTP_DISABLE_REDIRECTS;
    ::WinHttpSetOption(m_hRequest, WINHTTP_OPTION_DISABLE_FEATURE, &dwDisable, sizeof(dwDisable));
  }
  
  // Install the status callback function
  WINHTTP_STATUS_CALLBACK pCallback = ::WinHttpSetStatusCallback(m_hRequest, 
    reinterpret_cast<WINHTTP_STATUS_CALLBACK>(&Callback), 
    WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 
    NULL);
  if (pCallback != NULL) {
    Cleanup();
    OnError(::GetLastError());
    return false;
  }
  
  // Send the request
  if (!::WinHttpSendRequest(m_hRequest, 
    header.c_str(), 
    header.length(), 
    (LPVOID)(m_OptionalData.c_str()), 
    m_OptionalData.length(), 
    m_OptionalData.length(), 
    reinterpret_cast<DWORD_PTR>(this))) {
      Cleanup();
      OnError(::GetLastError());
      return false;
  }

  // Success
  return true;
}

bool Http::Connect(const Url& url, wstring szData, wstring szVerb, wstring szHeader, wstring szReferer, 
                    wstring szFile, DWORD dwClientMode, LPARAM lParam) {
  return Connect(url.Host, url.Path, szData, szVerb, szHeader, szReferer, szFile, dwClientMode, lParam);
}

bool Http::Get(wstring szServer, wstring szObject, wstring szFile, DWORD dwClientMode, LPARAM lParam) {
  return Connect(szServer, szObject, L"", L"GET", L"", szServer, szFile, dwClientMode, lParam);
}

bool Http::Get(const Url& url, wstring szFile, DWORD dwClientMode, LPARAM lParam) {
  return Connect(url.Host, url.Path, L"", L"GET", L"", url.Host, szFile, dwClientMode, lParam);
}

bool Http::Post(wstring szServer, wstring szObject, wstring szData, wstring szFile, DWORD dwClientMode, LPARAM lParam) {
  return Connect(szServer, szObject, szData, L"POST", L"", szServer, szFile, dwClientMode, lParam);
}

bool Http::Post(const Url& url, wstring szData, wstring szFile, DWORD dwClientMode, LPARAM lParam) {
  return Connect(url.Host, url.Path, szData, L"POST", L"", url.Host, szFile, dwClientMode, lParam);
}

// =============================================================================

void CALLBACK Http::Callback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, 
                              LPVOID lpvStatusInformation, DWORD dwStatusInformationLength) {
  Http* object = reinterpret_cast<Http*>(dwContext);
  if (object != NULL) {
    object->StatusCallback(hInternet, dwInternetStatus,
      lpvStatusInformation, dwStatusInformationLength);
  }
}

void Http::StatusCallback(HINTERNET hInternet, DWORD dwInternetStatus, 
                           LPVOID lpvStatusInformation, DWORD dwStatusInformationLength) {
  switch (dwInternetStatus) {
    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE: {
      if (::WinHttpReceiveResponse(hInternet, NULL)) {
        OnSendRequestComplete();
      }
      break;
    }

    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE: {
      DWORD dwSize = 0;
      LPVOID lpOutBuffer = NULL;
      if (!::WinHttpQueryHeaders(hInternet, 
        WINHTTP_QUERY_RAW_HEADERS_CRLF, 
        WINHTTP_HEADER_NAME_BY_INDEX, 
        NULL, &dwSize, WINHTTP_NO_HEADER_INDEX)) {
          DWORD dwError = GetLastError();
          if (dwError != ERROR_INSUFFICIENT_BUFFER) {
            OnError(dwError);
            break;
          }
      }
      lpOutBuffer = new WCHAR[dwSize];
      if (::WinHttpQueryHeaders(hInternet, 
        WINHTTP_QUERY_RAW_HEADERS_CRLF, 
        WINHTTP_HEADER_NAME_BY_INDEX, 
        lpOutBuffer, &dwSize, 
        WINHTTP_NO_HEADER_INDEX)) {
          if (!ParseResponseHeader((LPWSTR)lpOutBuffer) || 
            OnHeadersAvailable(m_ResponseHeader)) {
              delete [] lpOutBuffer;
              break;
          }
      }
      delete [] lpOutBuffer;
      ::WinHttpQueryDataAvailable(hInternet, NULL);
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
        if (!WinHttpReadData(hInternet, (LPVOID)lpOutBuffer, dwSize, NULL)) {
          delete [] lpOutBuffer;
          break;
        }
        if (OnDataAvailable()) {
          Cleanup();
        }
      } else if (dwSize == 0) {
        if (m_ContentEncoding == HTTP_Encoding_Gzip) {
          string input, output;
          input.append(m_Buffer, m_dwDownloaded);
          UncompressGzippedString(input, output);
          if (!output.empty()) {
            delete [] m_Buffer;
            m_dwDownloaded = output.length();
            m_Buffer = new char[m_dwDownloaded];
            memcpy(m_Buffer, &output[0], m_dwDownloaded);
          }
        }
        if (!m_File.empty()) {
          HANDLE hFile = ::CreateFile(m_File.c_str(), GENERIC_WRITE, 0, NULL, 
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
          if (hFile != INVALID_HANDLE_VALUE) {
            DWORD dwBytesToWrite = m_dwDownloaded, dwBytesWritten = 0;
            ::WriteFile(hFile, (LPCVOID)m_Buffer, dwBytesToWrite, &dwBytesWritten, NULL);
            ::CloseHandle(hFile);
          }
        }
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
        if(!m_Buffer) {
          m_Buffer = lpReadBuffer;
        } else {
          LPSTR lpOldBuffer = m_Buffer;
          m_Buffer = new char[m_dwDownloaded + dwBytesRead];
          memcpy(m_Buffer, lpOldBuffer, m_dwDownloaded);
          memcpy(m_Buffer + m_dwDownloaded, lpReadBuffer, dwBytesRead);
          delete [] lpOldBuffer;
          delete [] lpReadBuffer;
        }
        m_dwDownloaded += dwBytesRead;
        if (OnReadData()) {
          Cleanup();
        } else {
          ::WinHttpQueryDataAvailable(hInternet, NULL);
        }
      }
      break;
    }

    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR: {
      WINHTTP_ASYNC_RESULT* result = (WINHTTP_ASYNC_RESULT*)lpvStatusInformation;
      switch (result->dwResult) {
        case API_RECEIVE_RESPONSE:
        case API_QUERY_DATA_AVAILABLE:
        case API_READ_DATA:
        case API_WRITE_DATA:
        case API_SEND_REQUEST:
          OnError(result->dwError);
          //Cleanup();
      }
      break;
    }
  }
}

// =============================================================================

Http::Http() : 
  m_AutoRedirect(TRUE), m_Buffer(NULL), m_ContentEncoding(HTTP_Encoding_None), 
  m_ResponseStatusCode(0), m_dwDownloaded(0), m_dwTotal(0), m_dwClientMode(0), m_lParam(0), 
  m_hConnect(NULL), m_hRequest(NULL), m_hSession(NULL)
{
  m_UserAgent = L"Mozilla/5.0";
}

Http::~Http() {
  Cleanup();
}

void Http::Cleanup() {
  // Close handles
  if (m_hRequest) {
    ::WinHttpSetStatusCallback(m_hRequest, NULL, NULL, NULL);
    ::WinHttpCloseHandle(m_hRequest);
    m_hRequest = NULL;
  }
  if (m_hConnect) {
    ::WinHttpCloseHandle(m_hConnect);
    m_hConnect = NULL;
  }
  if (m_hSession) {
    ::WinHttpCloseHandle(m_hSession);
    m_hSession = NULL;
  }

  // Clear buffer
  if (m_Buffer) {
    delete [] m_Buffer;
    m_Buffer = NULL;
  }

  // Reset variables
  m_ContentEncoding = HTTP_Encoding_None;
  m_File.clear();
  m_OptionalData.clear();
  m_Referer.clear();
  m_RequestHeader.clear();
  m_ResponseStatusCode = 0;
  m_ResponseHeader.clear();
  m_Verb.clear();
  m_dwDownloaded = 0;
  m_dwTotal = 0;
  m_dwClientMode = 0;
  m_lParam = 0;
}

void Http::ClearCookies() {
  m_Cookie.clear();
}

wstring Http::GetCookie() {
  return m_Cookie;
}

DWORD Http::GetClientMode() {
  return m_dwClientMode;
}

wstring Http::GetData() {
  if (!m_Buffer) return L"";
  return ToUTF8(m_Buffer);
}

wstring Http::GetDefaultHeader() {
  return L"Content-Type: application/x-www-form-urlencoded\r\nAccept: */*\r\n";
}

LPARAM Http::GetParam() {
  return m_lParam;
}

int Http::GetResponseStatusCode() {
  return m_ResponseStatusCode;
}

void Http::SetAutoRedirect(BOOL enabled) {
  m_AutoRedirect = enabled;
}

void Http::SetProxy(const wstring& proxy, const wstring& user, const wstring& pass) {
  m_Proxy = proxy;
  m_ProxyUser = user;
  m_ProxyPass = pass;
}

void Http::SetUserAgent(const wstring& user_agent) {
  m_UserAgent = user_agent;
}

// =============================================================================

bool Http::ParseHeader(const wstring& text, http_header_t& header) {
  header.clear();
  if (text.empty()) return false;
  
  vector<wstring> header_list;
  Split(text, L"\r\n", header_list);

  for (auto it = header_list.begin(); it != header_list.end(); ++it) {
    int pos = InStr(*it, L":", 0);
    if (pos == -1) {
      if (StartsWith(*it, L"HTTP/")) {
        m_ResponseStatusCode = ToInt(InStr(*it, L" ", L" "));
      }
    } else {
      wstring name = CharLeft(*it, pos);
      wstring value = it->substr(pos + 2);
      header.insert(std::pair<wstring, wstring>(name, value));
    }
  }

  return true;
}

wstring Http::BuildRequestHeader(wstring header) {
  if (header.empty()) {
    header = GetDefaultHeader();
  } else {
    header = header + L"\r\n";
  }
  
  if (!m_Cookie.empty()) {
    header += L"Cookie: " + m_Cookie + L"\r\n";
  }
  
  return header;
}

bool Http::ParseResponseHeader(const wstring& header) {
  if (!ParseHeader(header, m_ResponseHeader)) return false;
  Url location;

  for (auto it = m_ResponseHeader.cbegin(); it != m_ResponseHeader.cend(); ++it) {
    wstring name = it->first;
    wstring value = it->second;

    // Content-Encoding:
    if (IsEqual(name, L"Content-Encoding")) {
      if (InStr(value, L"gzip") > -1) {
        m_ContentEncoding = HTTP_Encoding_Gzip;
      } else {
        m_ContentEncoding = HTTP_Encoding_None;
      }

    // Content-Length:
    } else if (IsEqual(name, L"Content-Length")) {
      m_dwTotal = ToInt(value);

    // Location:
    } else if (IsEqual(name, L"Location")) {
      if (!OnRedirect(value)) {
        location.Crack(value);
      }
      
    // Set-Cookie:
    } else if (IsEqual(name, L"Set-Cookie")) {
      int pos = InStr(value, L";", 0);
      if (pos > -1) value = value.substr(0, pos);
      m_Cookie += (m_Cookie.empty() ? L"" : L"; ") + value;
    }
  }

  if (!location.Host.empty()) {
    if (m_AutoRedirect == FALSE) {
      if (!Connect(location, ToUTF8(m_OptionalData), m_Verb, m_RequestHeader, 
        m_Referer, m_File, m_dwClientMode, m_lParam)) {
          Cleanup();
      }
      return false;
    } else {
      m_ContentEncoding = HTTP_Encoding_None;
      m_dwDownloaded = 0;
      m_dwTotal = 0;
    }
  }

  return true;
}

} // namespace win32