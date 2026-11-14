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

#include "settings.hpp"

#include <QIODevice>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>

namespace {

QSettings::Format jsonSettingsFormat() {
  static const auto read = [](QIODevice& device, QSettings::SettingsMap& map) {
    QJsonParseError error;
    map = QJsonDocument::fromJson(device.readAll(), &error).toVariant().toMap();
    return error.error == QJsonParseError::NoError;
  };

  static const auto write = [](QIODevice& device, const QSettings::SettingsMap& map) {
    const auto json = QJsonDocument::fromVariant(map).toJson();
    return device.write(json) == json.size();
  };

  static const auto format = QSettings::registerFormat("json", read, write);
  return format;
}

}  // namespace

namespace base {

QVariant Settings::value(QAnyStringView key) const {
  return settings().value(key);
}

QVariant Settings::value(QAnyStringView key, const QVariant& defaultValue) const {
  return settings().value(key, defaultValue);
}

void Settings::setValue(QAnyStringView key, const QVariant& value) const {
  auto qsettings = settings();

  // Values are compared as JSON, which is how they are stored (e.g. `QJsonArray` is read back as
  // `QVariantList`).
  if (QJsonValue::fromVariant(qsettings.value(key)) == QJsonValue::fromVariant(value)) return;

  qsettings.setValue(key, value);
  qsettings.sync();

  onValueChanged(key);
}

void Settings::setValue(QAnyStringView key, const std::string_view value) const {
  setValue(key, QString::fromUtf8(value));
}

QSettings Settings::settings() const {
  return QSettings(fileName(), jsonSettingsFormat());
}

}  // namespace base
