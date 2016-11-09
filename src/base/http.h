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

#ifndef TAIGA_BASE_HTTP_H
#define TAIGA_BASE_HTTP_H

// Each client will have its own thread
#define TAIGA_HTTP_MULTITHREADED

#ifdef _DEBUG
#define TAIGA_HTTP_SSL_UNSECURE
#endif

// CURL definitions
#ifndef CURL_STATICLIB
#define CURL_STATICLIB
#endif
#ifndef HTTP_ONLY
#define HTTP_ONLY
#endif

#include <windows.h>
#include <string>
#include <vector>

#include <curl/curl.h>

#include "map.h"
#include "url.h"
#include "win/win_thread.h"

namespace base {
namespace http {

typedef base::multimap<std::wstring, std::wstring> header_t;

enum ContentEncoding {
  kContentEncodingNone,
  kContentEncodingGzip
};

////////////////////////////////////////////////////////////////////////////////

class Request {
public:
  Request();
  virtual ~Request() {}

  void Clear();

  std::wstring method;
  Url url;

  header_t header;
  std::wstring body;
  query_t data;

  std::wstring uid;
  LPARAM parameter;
};

class Response {
public:
  Response();
  virtual ~Response() {}

  void Clear();

  unsigned int code;

  header_t header;
  std::wstring body;

  std::wstring uid;
  LPARAM parameter;
};

std::wstring GenerateRequestId();

////////////////////////////////////////////////////////////////////////////////

class CurlGlobal {
public:
  CurlGlobal();
  ~CurlGlobal();

  bool initialized() const;

private:
  bool initialized_;
};

class Client : public win::Thread {
public:
  Client(const Request& request);
  virtual ~Client();

  void Cancel();
  void Cleanup(bool reuse);
  bool MakeRequest(const Request& request);

  bool allow_reuse() const;
  bool busy() const;
  bool no_revoke() const;
  const Request& request() const;
  const Response& response() const;
  curl_off_t content_length() const;
  curl_off_t current_length() const;

  void set_allow_reuse(bool allow);
  void set_auto_redirect(bool enabled);
  void set_debug_mode(bool enabled);
  void set_no_revoke(bool enabled);
  void set_proxy(
      const std::wstring& host,
      const std::wstring& username,
      const std::wstring& password);
  void set_referer(const std::wstring& referer);
  void set_user_agent(const std::wstring& user_agent);

  virtual void OnError(CURLcode error_code) {}
  virtual bool OnHeadersAvailable() { return false; }
  virtual bool OnProgress() { return false; }
  virtual void OnReadComplete() {}
  virtual bool OnRedirect(const std::wstring& address, bool refresh) { return false; }

  DWORD ThreadProc();

protected:
  Request request_;
  Response response_;

  ContentEncoding content_encoding_;
  curl_off_t content_length_;
  curl_off_t current_length_;
  std::string write_buffer_;

  bool allow_reuse_;
  bool auto_redirect_;
  bool no_revoke_;
  std::wstring proxy_host_;
  std::wstring proxy_password_;
  std::wstring proxy_username_;
  std::wstring referer_;
  std::wstring user_agent_;

private:
  static size_t HeaderFunction(void*, size_t, size_t, void*);
  static size_t WriteFunction(char*, size_t, size_t, void*);
  static int DebugFunction(CURL*, curl_infotype, char*, size_t, void*);
  static int XferInfoFunction(void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t);
  int DebugHandler(curl_infotype, std::string, bool);
  int ProgressFunction(curl_off_t, curl_off_t);

  bool Initialize();
  bool SetRequestOptions();
  bool SendRequest();
  bool Perform();

  void BuildRequestHeader();
  bool GetResponseHeader(const std::wstring& header);
  bool ParseResponseHeader();

  static CurlGlobal curl_global_;
  CURL* curl_handle_;

  bool busy_;
  bool cancel_;
  bool debug_mode_;
  curl_slist* header_list_;
  std::string optional_data_;
};

}  // namespace http
}  // namespace base

#endif  // TAIGA_BASE_HTTP_H