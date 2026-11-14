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

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <optional>

namespace share::discord {

namespace ipc {
class Client;
}

// Types allowed by `SET_ACTIVITY`
enum class ActivityType {
  Playing = 0,
  Listening = 2,
  Watching = 3,
  Competing = 5,
};

// Which field is shown in the user's status text in the member list
enum class StatusDisplayType {
  Name = 0,  // application name
  State = 1,
  Details = 2,
};

struct ActivityButton {
  QString label;
  QString url;
};

// Rich presence shown on the user's profile. Empty fields are omitted, and texts are trimmed to
// the lengths that Discord accepts.
// See: https://docs.discord.com/developers/events/gateway-events#activity-object
struct Activity {
  ActivityType type = ActivityType::Playing;
  StatusDisplayType statusDisplayType = StatusDisplayType::Name;

  QString details;
  QString detailsUrl;
  QString state;
  QString stateUrl;

  qint64 startTime = 0;  // seconds since epoch, shown as elapsed time

  QString largeImage;  // asset key or image URL
  QString largeText;
  QString largeUrl;
  QString smallImage;  // asset key or image URL
  QString smallText;
  QString smallUrl;

  QList<ActivityButton> buttons;  // at most 2, not visible to the user themselves
};

// Sets the rich presence of the user through the Discord desktop app.
class ActivityManager final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(ActivityManager)

public:
  ActivityManager(const QString& applicationId, QObject* parent = nullptr);
  ~ActivityManager() override = default;

  // Connects to Discord, and keeps reconnecting until closed.
  void open();
  // Disconnects from Discord, which removes the activity.
  void close();

  // The activity is kept until changed, and sent again whenever the connection is
  // (re)established.
  void setActivity(const Activity& activity);
  void clearActivity();

private:
  void sendActivity();

  ipc::Client* client_ = nullptr;
  std::optional<QJsonObject> activity_;
};

// The `activity` object of a `SET_ACTIVITY` command
QJsonObject toJson(const Activity& activity);

}  // namespace share::discord
