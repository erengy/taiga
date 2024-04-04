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

#include <atomic>
#include <list>
#include <mutex>
#include <thread>

#include "taiga/http.h"

#include <nstd/string.hpp>

#include "base/file.h"
#include "base/format.h"
#include "base/gzip.h"
#include "base/log.h"
#include "base/string.h"
#include "taiga/app.h"
#include "taiga/config.h"
#include "taiga/settings.h"
#include "taiga/stats.h"
#include "ui/ui.h"

namespace taiga::http {

namespace detail {

static hypr::Options GetOptions() {
  hypr::Options options;

  options.allow_redirects = true;

  // Disabling certificate revocation checks seems to work for those who get
  // "SSL connect error". See issue #312 for more information.
  options.certificate_revocation = !settings.GetAppConnectionNoRevoke();

  // Web browsers set the maximum number of redirects to ~20 (e.g. Firefox's
  // `network.http.redirection-limit` option), but we do not need that many.
  options.max_redirects = 5;

  // Complete connection within 30 seconds (default is 300 seconds)
  options.timeout = std::chrono::seconds{30};

  // Log verbose information about libcurl's operations
  options.verbose = app.options.debug_mode && app.options.verbose;

#ifdef _DEBUG
  // Skip SSL verifications in debug build
  options.verify_certificate = false;
#else
  options.verify_certificate = true;
#endif

  return options;
}

static hypr::Proxy GetProxy() {
  hypr::Proxy proxy;

  proxy.host = WstrToStr(settings.GetAppConnectionProxyHost());
  proxy.username = WstrToStr(settings.GetAppConnectionProxyUsername());
  proxy.password = WstrToStr(settings.GetAppConnectionProxyPassword());

  return proxy;
}

static void Debug(const curl_infotype type, std::string_view data) {
  auto str = StrToWstr(std::string{data});
  Trim(str, L" \r\n");

  switch (type) {
    case CURLINFO_TEXT:
      LOGD(str);
      break;
    case CURLINFO_HEADER_IN:
      LOGD(L"<= Recv header | {}", str);
      break;
    case CURLINFO_HEADER_OUT:
      LOGD(L"=> Send header | {}", str);
      break;
    case CURLINFO_DATA_IN:
      LOGD(L"<= Recv data ({} bytes)", data.size());
      break;
    case CURLINFO_DATA_OUT:
      LOGD(L"=> Send data ({} bytes)", data.size());
      break;
    case CURLINFO_SSL_DATA_IN:
      LOGD(L"<= Recv SSL data ({} bytes)", data.size());
      break;
    case CURLINFO_SSL_DATA_OUT:
      LOGD(L"=> Send SSL data ({} bytes)", data.size());
      break;
  }
}

static void SendRequest(Request request,
                        const TransferCallback& on_transfer,
                        const ResponseCallback& on_response,
                        hypr::Session& session) {
  session.options = GetOptions();
  session.proxy = GetProxy();

  session.callbacks.debug = Debug;
  session.callbacks.transfer = on_transfer;

  // The default header (e.g. "User-Agent: Taiga/1.0") will be used, unless
  // another value is specified in the request header
  if (request.header("user-agent").empty()) {
    request.set_header("User-Agent", WstrToStr(L"{}/{}.{}"_format(
        TAIGA_APP_NAME, TAIGA_VERSION_MAJOR, TAIGA_VERSION_MINOR)));
  }

  LOGD(L"URL: {}"_format(StrToWstr(hypp::to_string(request.target()))));

  auto response = session.send(request);  // blocks

  // @TODO: Remove once hypr is able to do this automatically
  const auto content_encoding = response.header("content-encoding");
  if (content_encoding.find("gzip") != content_encoding.npos) {
    if (!response.body().empty()) {
      std::string uncompressed;
      if (UncompressGzippedString(response.body(), uncompressed)) {
        std::swap(response.body(), uncompressed);
      }
    }
  }

  // @TODO: Do the rest in the main thread

  if (response.error()) {
    LOGE(util::to_string(response.error(), util::GetUrlHost(response.url())));
    taiga::stats.connections_failed++;
  } else {
    taiga::stats.connections_succeeded++;
  }

  if (on_response) {
    on_response(response);
  }

  ProcessQueue();
};

class Client final {
public:
  enum class State {
    Ready,
    Busy,
    Cancelled,
  };

  Client() = default;

  Client::~Client() {
    Cancel();
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  void Cancel() {
    if (state_ == State::Busy) {
      state_ = State::Cancelled;
    }
  }

  void Send(const Request& request,
            const TransferCallback& on_transfer,
            const ResponseCallback& on_response) {
    state_ = State::Busy;

    if (thread_.joinable()) {
      thread_.join();
    }

    const auto transfer_callback = [=](const Transfer& transfer) {
      if (state_ == State::Cancelled) {
        return false;
      }
      return on_transfer ? on_transfer(transfer) : true;
    };

    thread_ = std::thread([=]() {
      SendRequest(request, transfer_callback, on_response, session_);
      state_ = State::Ready;
    });
  }

  State state() const {
    return state_;
  }

private:
  hypr::Session session_;
  std::atomic<State> state_ = State::Ready;
  std::thread thread_;
};

struct QueuedRequest {
  Request request;
  TransferCallback on_transfer;
  ResponseCallback on_response;
};

class Pool final {
public:
  Pool() = default;

  ~Pool() {
    Shutdown();
  }

  void AddToQueue(QueuedRequest&& item) {
    std::lock_guard lock{mutex_};

    if (shutdown_) {
      LOGD(L"Shutting down...");
      return;
    }

    const auto& authority = item.request.target().uri.authority;

    if (!authority) {
      LOGE(L"Invalid request target: {}",
           StrToWstr(hypp::to_string(item.request.target())));
      return;
    }

    queue_[authority->host].push_back(std::move(item));
  }

  void ProcessQueue() {
    std::lock_guard lock{mutex_};

    if (queue_.empty()) {
      return;
    }
    if (ReachedMaxConnections()) {
      if (app.options.verbose) {
        LOGD(L"Reached max connections");
      }
      return;
    }

    for (auto& [host, items] : queue_) {
      for (size_t i = 0; i < items.size(); ++i) {
        const auto& item = items[i];

        if (ReachedMaxConnections(host)) {
          if (app.options.verbose) {
            LOGD(L"Reached max connections for host: {}", StrToWstr(host));
          }
          break;
        }

        auto& client = GetClient(host);
        client.Send(item.request, item.on_transfer, item.on_response);

        items.erase(items.begin() + i--);
      }
    }
  }

  void Shutdown() {
    std::lock_guard lock{mutex_};

    shutdown_ = true;
    queue_.clear();

    for (auto& [host, clients] : connections_) {
      for (auto& client : clients) {
        client.Cancel();
      }
    }
  }

private:
  Client& GetClient(const std::string& host) {
    auto& clients = connections_[host];

    if (settings.GetAppConnectionReuseActive()) {
      for (auto& client : clients) {
        if (client.state() == Client::State::Ready) {
          if (app.options.verbose) {
            LOGD(L"Reusing client for {}", StrToWstr(host));
          }
          return client;
        }
      }
    }

    auto& client = clients.emplace_back();
    if (app.options.verbose) {
      LOGD(L"Created new client for {} ({})", StrToWstr(host), clients.size());
    }
    return client;
  }

  bool ReachedMaxConnections() const {
    constexpr size_t kMaxSimultaneousConnections = 10;
    size_t connections = 0;
    for (const auto& [host, clients] : connections_) {
      for (const auto& client : clients) {
        if (client.state() != Client::State::Ready) {
          if (++connections == kMaxSimultaneousConnections) {
            return true;
          }
        }
      }
    }
    return false;
  }

  bool ReachedMaxConnections(const std::string& host) const {
    constexpr size_t kMaxSimultaneousConnectionsPerHost = 6;
    const auto it = connections_.find(host);
    if (it != connections_.end()) {
      size_t connections = 0;
      for (const auto& client : it->second) {
        if (client.state() != Client::State::Ready) {
          if (++connections == kMaxSimultaneousConnectionsPerHost) {
            return true;
          }
        }
      }
    }
    return false;
  }

  std::map<std::string, std::list<Client>> connections_;
  std::map<std::string, std::vector<QueuedRequest>> queue_;

  std::mutex mutex_;
  bool shutdown_ = false;
};

}  // namespace detail

namespace util {

std::wstring GetUrlHost(const Uri& uri) {
  if (uri.authority.has_value()) {
    return StrToWstr(uri.authority->host);
  }
  return {};
}

std::wstring GetUrlHost(const std::string_view url) {
  hypp::Parser parser{url};
  if (const auto expected = hypp::ParseUri(parser)) {
    if (expected.value().authority.has_value()) {
      return StrToWstr(expected.value().authority->host);
    }
  }
  return {};
}

// Check for DDoS protection that requires a JavaScript challenge to be solved
// (e.g. Cloudflare's "I'm Under Attack" mode)
bool IsDdosProtectionEnabled(const Response& response) {
  const std::string server = nstd::tolower_string(response.header("server"));

  switch (response.status_code()) {
    case hypp::status::k403_Forbidden:
      return nstd::starts_with(server, "ddos-guard");
    case hypp::status::k429_Too_Many_Requests:
    case hypp::status::k503_Service_Unavailable:
      return nstd::starts_with(server, "cloudflare");
  }

  return false;
};

std::wstring to_string(const Error& error, const std::wstring& host) {
  std::wstring message = StrToWstr(error.str());
  TrimRight(message, L" \r\n");
  message = L"{} ({})"_format(message, static_cast<int>(error.code));
  if (!host.empty()) {
    switch (error.code) {
      case CURLE_COULDNT_RESOLVE_HOST:
      case CURLE_COULDNT_CONNECT:
        message += L" ({})"_format(host);
        break;
    }
  }
  return message;
}

std::wstring to_string(const Transfer& transfer) {
  if (transfer.total > 0) {
    const auto percentage = static_cast<float>(transfer.current) /
                            static_cast<float>(transfer.total) * 100;
    return L"{}%"_format(static_cast<int>(percentage));
  } else {
    return ToSizeString(transfer.current);
  }
}

}  // namespace util

static detail::Pool pool;

void Init() {
  hypr::init();
}

void ProcessQueue() {
  pool.ProcessQueue();
}

void Shutdown() {
  pool.Shutdown();
}

void Send(const Request& request,
          const TransferCallback& on_transfer,
          const ResponseCallback& on_response) {
  pool.AddToQueue({request, on_transfer, on_response});
  pool.ProcessQueue();
}

}  // namespace taiga::http
