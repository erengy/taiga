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

#include "discord_rpc.hpp"

#include <QCoreApplication>
#include <QJsonArray>
#include <utility>

#include "base/string.hpp"
#include "share/discord_ipc.hpp"

namespace share::discord {

namespace {

constexpr qsizetype kMinTextLength = 2;
constexpr qsizetype kMaxTextLength = 128;
constexpr qsizetype kMaxButtonLabelLength = 32;
constexpr qsizetype kMaxButtonUrlLength = 512;
constexpr qsizetype kMaxButtons = 2;

QString truncate(const QString& text, const qsizetype maxLength) {
  if (text.size() <= maxLength) return text;
  return text.left(maxLength - 1) + u"…"_s;
}

// Discord rejects the whole activity if a text is too short, so such texts are omitted.
void insertText(QJsonObject& object, const QString& key, const QString& text) {
  const auto trimmed = text.trimmed();
  if (trimmed.size() >= kMinTextLength) object[key] = truncate(trimmed, kMaxTextLength);
}

void insertString(QJsonObject& object, const QString& key, const QString& value) {
  if (!value.isEmpty()) object[key] = value;
}

}  // namespace

ActivityManager::ActivityManager(const QString& applicationId, QObject* parent)
    : QObject{parent}, client_{new ipc::Client{applicationId, this}} {
  // A new connection starts without an activity.
  connect(client_, &ipc::Client::ready, this, [this]() {
    if (activity_) sendActivity();
  });
}

void ActivityManager::open() {
  client_->open();
}

void ActivityManager::close() {
  client_->close();
}

void ActivityManager::setActivity(const Activity& activity) {
  auto json = toJson(activity);

  // Avoid hitting the rate limit with updates that would change nothing.
  if (activity_ == json) return;

  activity_ = std::move(json);
  sendActivity();
}

void ActivityManager::clearActivity() {
  if (!activity_) return;

  activity_.reset();
  sendActivity();
}

void ActivityManager::sendActivity() {
  QJsonObject args{{"pid", QCoreApplication::applicationPid()}};

  // The activity is cleared if omitted.
  if (activity_) args["activity"] = *activity_;

  client_->sendCommand(u"SET_ACTIVITY"_s, args);
}

QJsonObject toJson(const Activity& activity) {
  QJsonObject json{{"type", static_cast<int>(activity.type)}};

  insertText(json, u"details"_s, activity.details);
  insertString(json, u"details_url"_s, activity.detailsUrl);
  insertText(json, u"state"_s, activity.state);
  insertString(json, u"state_url"_s, activity.stateUrl);

  // Discord would show nothing if the field to display were missing
  const auto displayed =
      activity.statusDisplayType == StatusDisplayType::State ? u"state"_s : u"details"_s;
  if (activity.statusDisplayType != StatusDisplayType::Name && json.contains(displayed)) {
    json["status_display_type"] = static_cast<int>(activity.statusDisplayType);
  }

  if (activity.startTime > 0) {
    json["timestamps"] = QJsonObject{{"start", activity.startTime}};
  }

  QJsonObject assets;
  insertString(assets, u"large_image"_s, activity.largeImage);
  insertText(assets, u"large_text"_s, activity.largeText);
  insertString(assets, u"large_url"_s, activity.largeUrl);
  insertString(assets, u"small_image"_s, activity.smallImage);
  insertText(assets, u"small_text"_s, activity.smallText);
  insertString(assets, u"small_url"_s, activity.smallUrl);
  if (!assets.isEmpty()) json["assets"] = assets;

  QJsonArray buttons;
  for (const auto& button : activity.buttons) {
    if (buttons.size() == kMaxButtons) break;
    const auto label = button.label.trimmed();
    if (label.isEmpty() || button.url.isEmpty() || button.url.size() > kMaxButtonUrlLength) {
      continue;
    }
    buttons.append(QJsonObject{
        {"label", truncate(label, kMaxButtonLabelLength)},
        {"url", button.url},
    });
  }
  if (!buttons.isEmpty()) json["buttons"] = buttons;

  return json;
}

}  // namespace share::discord
