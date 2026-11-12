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

#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>
#include <chrono>

class QJsonObject;
class QLocalSocket;
class QTimer;

namespace share::discord::ipc {

enum class Opcode : qint32 {
  Handshake = 0,
  Frame = 1,
  Close = 2,
  Ping = 3,
  Pong = 4,
};

enum class State {
  Disconnected,
  Connecting,
  Handshaking,
  Ready,
};

// Connection to the local IPC socket of the Discord desktop app. Handles the handshake, framing
// and reconnection, but knows nothing about the commands sent over it.
// See: https://docs.discord.com/developers/topics/rpc
class Client final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Client)

public:
  Client(const QString& clientId, QObject* parent = nullptr,
         const QString& socketPrefix = QStringLiteral("discord-ipc-"),
         std::chrono::milliseconds retryInterval = std::chrono::seconds{2},
         std::chrono::milliseconds handshakeTimeout = std::chrono::seconds{20});
  ~Client() override = default;

  // Connects, and keeps reconnecting until closed (e.g. if Discord is not running yet).
  void open();
  // Disconnects immediately, which makes Discord remove anything set by this connection.
  void close();

  // Whether the handshake is complete and commands can be sent.
  bool isReady() const;

  // Sends a command if ready, otherwise drops it.
  void sendCommand(const QString& command, const QJsonObject& args);

signals:
  void ready();
  void disconnected();
  void errorReceived(int code, const QString& message);

private:
  QString socketName(int index) const;

  void connectToDiscord();
  void tryNextSocket();
  void onConnected();
  void onDisconnected();
  void abortConnection(const QString& reason);
  void scheduleRetry();

  void readFrames();
  void handleFrame(Opcode opcode, const QByteArray& payload);
  void writeFrame(Opcode opcode, const QJsonObject& json);

  const QString clientId_;
  const QString socketPrefix_;
  const std::chrono::milliseconds minRetryInterval_;
  std::chrono::milliseconds retryInterval_;

  QLocalSocket* socket_ = nullptr;
  QTimer* retryTimer_ = nullptr;
  QTimer* handshakeTimer_ = nullptr;
  QByteArray buffer_;

  State state_ = State::Disconnected;
  int socketIndex_ = 0;
  bool open_ = false;
  bool retryAllowed_ = true;
};

}  // namespace share::discord::ipc
