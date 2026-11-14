/**
 * Taiga
 * Copyright (C) 2010-2026, Eren Okka
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

#include "discord_ipc.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QTimer>
#include <QUuid>
#include <QtEndian>
#include <algorithm>

#include "base/log.hpp"
#include "base/string.hpp"

namespace share::discord::ipc {

namespace {

constexpr int kSocketCount = 10;  // Discord listens on the first free of discord-ipc-{0..9}
constexpr int kHeaderSize = 8;    // opcode + length, both 32-bit little-endian
constexpr int kInvalidClientId = 4000;
constexpr qint32 kMaxFrameSize = 1024 * 1024;
constexpr std::chrono::milliseconds kMaxRetryInterval = std::chrono::minutes{1};

}  // namespace

Client::Client(const QString& clientId, QObject* parent, const QString& socketPrefix,
               std::chrono::milliseconds retryInterval, std::chrono::milliseconds handshakeTimeout)
    : QObject{parent},
      clientId_{clientId},
      socketPrefix_{socketPrefix},
      minRetryInterval_{retryInterval},
      retryInterval_{retryInterval},
      socket_{new QLocalSocket{this}},
      retryTimer_{new QTimer{this}},
      handshakeTimer_{new QTimer{this}} {
  retryTimer_->setSingleShot(true);
  connect(retryTimer_, &QTimer::timeout, this, &Client::connectToDiscord);

  handshakeTimer_->setSingleShot(true);
  handshakeTimer_->setInterval(handshakeTimeout);
  connect(handshakeTimer_, &QTimer::timeout, this, [this]() {
    if (state_ == State::Handshaking) abortConnection(u"no response to handshake"_s);
  });

  connect(socket_, &QLocalSocket::connected, this, &Client::onConnected);
  connect(socket_, &QLocalSocket::disconnected, this, &Client::onDisconnected);
  connect(socket_, &QLocalSocket::readyRead, this, &Client::readFrames);

  // Queued, so that the socket is not reconnected from within its own error handler.
  connect(socket_, &QLocalSocket::errorOccurred, this, &Client::tryNextSocket,
          Qt::QueuedConnection);
}

void Client::open() {
  if (open_) return;
  open_ = true;
  retryAllowed_ = true;
  retryInterval_ = minRetryInterval_;
  connectToDiscord();
}

void Client::close() {
  if (!open_) return;
  open_ = false;
  retryTimer_->stop();
  handshakeTimer_->stop();
  // Closing immediately (rather than waiting for pending writes) lets the socket be reused right
  // away if opened again.
  socket_->abort();
  state_ = State::Disconnected;
}

bool Client::isReady() const {
  return state_ == State::Ready;
}

void Client::sendCommand(const QString& command, const QJsonObject& args) {
  if (state_ != State::Ready) return;

  const QJsonObject frame{
      {"cmd", command},
      {"args", args},
      {"nonce", QUuid::createUuid().toString(QUuid::WithoutBraces)},
  };

  writeFrame(Opcode::Frame, frame);
}

QString Client::socketName(int index) const {
  const auto name = u"%1%2"_s.arg(socketPrefix_).arg(index);

#ifdef Q_OS_WINDOWS
  return name;  // QLocalSocket prepends `\\.\pipe\`
#else
  for (const auto variable : {"XDG_RUNTIME_DIR", "TMPDIR", "TMP", "TEMP"}) {
    if (const auto dir = qEnvironmentVariable(variable); !dir.isEmpty()) {
      return u"%1/%2"_s.arg(dir, name);
    }
  }
  return u"/tmp/%1"_s.arg(name);
#endif
}

void Client::connectToDiscord() {
  if (!open_ || state_ != State::Disconnected) return;

  state_ = State::Connecting;
  socketIndex_ = 0;
  socket_->connectToServer(socketName(socketIndex_));
}

void Client::tryNextSocket() {
  if (state_ != State::Connecting) return;

  if (++socketIndex_ < kSocketCount) {
    socket_->connectToServer(socketName(socketIndex_));
  } else {
    state_ = State::Disconnected;
    scheduleRetry();
  }
}

void Client::onConnected() {
  state_ = State::Handshaking;
  buffer_.clear();
  handshakeTimer_->start();

  const QJsonObject handshake{
      {"v", 1},
      {"client_id", clientId_},
  };

  writeFrame(Opcode::Handshake, handshake);
}

void Client::onDisconnected() {
  const bool wasConnected = state_ == State::Handshaking || state_ == State::Ready;
  state_ = State::Disconnected;
  handshakeTimer_->stop();
  buffer_.clear();

  if (wasConnected) emit disconnected();
  scheduleRetry();
}

void Client::abortConnection(const QString& reason) {
  qWarning() << "Discord:" << reason;
  socket_->abort();
  if (state_ != State::Disconnected) onDisconnected();
}

void Client::scheduleRetry() {
  if (!open_ || !retryAllowed_) return;

  // Retries quickly at first (e.g. when Discord is restarting), then less often.
  retryTimer_->start(retryInterval_);
  retryInterval_ = std::min(retryInterval_ * 2, kMaxRetryInterval);
}

void Client::readFrames() {
  buffer_ += socket_->readAll();

  while (buffer_.size() >= kHeaderSize) {
    const auto opcode = qFromLittleEndian<qint32>(buffer_.constData());
    const auto length = qFromLittleEndian<qint32>(buffer_.constData() + 4);
    if (length < 0 || length > kMaxFrameSize) {
      abortConnection(u"invalid frame length %1"_s.arg(length));
      return;
    }
    if (buffer_.size() < kHeaderSize + length) break;

    const auto payload = buffer_.mid(kHeaderSize, length);
    buffer_.remove(0, kHeaderSize + length);
    handleFrame(static_cast<Opcode>(opcode), payload);
  }
}

void Client::handleFrame(Opcode opcode, const QByteArray& payload) {
  const auto json = QJsonDocument::fromJson(payload).object();

  switch (opcode) {
    case Opcode::Frame: {
      const auto event = json["evt"].toString();
      if (json["cmd"].toString() == u"DISPATCH" && event == u"READY") {
        state_ = State::Ready;
        handshakeTimer_->stop();
        retryInterval_ = minRetryInterval_;
        emit ready();
      } else if (event == u"ERROR") {
        const auto data = json["data"].toObject();
        const auto code = data["code"].toInt();
        const auto message = data["message"].toString();
        qWarning() << "Discord: error" << code << message;
        emit errorReceived(code, message);
      }
      break;
    }

    case Opcode::Close:
      qWarning() << "Discord: connection closed" << json["code"].toInt()
                 << json["message"].toString();
      if (json["code"].toInt() == kInvalidClientId) retryAllowed_ = false;
      socket_->disconnectFromServer();
      break;

    case Opcode::Ping:
      writeFrame(Opcode::Pong, json);
      break;

    default:
      break;
  }
}

void Client::writeFrame(Opcode opcode, const QJsonObject& json) {
  const auto payload = QJsonDocument{json}.toJson(QJsonDocument::Compact);

  QByteArray frame(kHeaderSize, Qt::Uninitialized);
  qToLittleEndian<qint32>(static_cast<qint32>(opcode), frame.data());
  qToLittleEndian<qint32>(static_cast<qint32>(payload.size()), frame.data() + 4);
  frame += payload;
  socket_->write(frame);
}

}  // namespace share::discord::ipc
