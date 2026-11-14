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

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <algorithm>
#include <ranges>
#include <utility>

#include "base/string.hpp"
#include "compat/settings.hpp"
#include "sync/service.hpp"
#include "taiga/accounts.hpp"
#include "taiga/path.hpp"
#include "taiga/version.hpp"

namespace taiga {

Settings::Settings(QObject* parent) : QObject{parent} {}

void Settings::init() const {
  const auto appVersion = taiga::version().to_string();

  // v1 to v2
  if (!QFile::exists(fileName())) {
    compat::v1::readSettings(std::format("{}/v1/settings.xml", get_data_path()), *this,
                             *accounts());
    setValue(Key::kMetaVersion, appVersion);
    return;
  }

  // v2.x
  const auto fileVersion = value(Key::kMetaVersion).toString().toStdString();
  if (fileVersion != appVersion) {
    setValue(Key::kMetaVersion, appVersion);
  }
}

QString Settings::fileName() const {
  return u"%1/settings.json"_s.arg(get_data_path());
}

void Settings::onValueChanged(QAnyStringView key) const {
  const bool scheduled = !changedKeys_.isEmpty();
  changedKeys_.insert(key.toString());
  if (scheduled) return;

  QMetaObject::invokeMethod(
      const_cast<Settings*>(this), [this] { emit changed(std::exchange(changedKeys_, {})); },
      Qt::QueuedConnection);
}

////////////////////////////////////////////////////////////////////////////////

Qt::ColorScheme Settings::appColorScheme() const {
  return value(Key::kAppColorScheme, static_cast<int>(Qt::ColorScheme::Unknown))
      .value<Qt::ColorScheme>();
}

std::string Settings::appStyle() const {
#ifdef Q_OS_WINDOWS
  // Fusion is more consistent than the Windows 11 style.
  const auto defaultStyle = u"fusion"_s;
#else
  const auto defaultStyle = QString{kAppStyleSystem};
#endif
  return value(Key::kAppStyle, defaultStyle).toString().toStdString();
}

bool Settings::detectionEnabled() const {
  return value(Key::kDetectionEnabled, true).toBool();
}

std::vector<std::string> Settings::disabledMediaPlayers() const {
  return value(Key::kDisabledMediaPlayers).toJsonArray().toVariantList() |
         std::views::transform([](const QVariant& v) { return v.toString().toStdString(); }) |
         std::ranges::to<std::vector>();
}

bool Settings::discordEnabled() const {
  return value(Key::kDiscordEnabled, false).toBool();
}

bool Settings::httpEnabled() const {
  return value(Key::kHttpEnabled, false).toBool();
}

std::vector<std::string> Settings::httpHeaders() const {
  return value(Key::kHttpHeaders).toJsonArray().toVariantList() |
         std::views::transform([](const QVariant& v) { return v.toString().toStdString(); }) |
         std::ranges::to<std::vector>();
}

std::string Settings::httpUrl() const {
  return value(Key::kHttpUrl).toString().toStdString();
}

std::vector<std::string> Settings::libraryFolders() const {
  return value(Key::kLibraryFolders).toJsonArray().toVariantList() |
         std::views::transform([](const QVariant& v) { return v.toString().toStdString(); }) |
         std::ranges::to<std::vector>();
}

std::chrono::milliseconds Settings::mediaDetectionInterval() const {
  const auto interval = value(Key::kMediaDetectionInterval, 3000).toInt();
  return std::chrono::milliseconds{interval};
}

QNetworkProxy::ProxyType Settings::proxyType() const {
  const auto type = value(Key::kProxyType, u"http"_s).toString();
  if (type == u"socks5") return QNetworkProxy::Socks5Proxy;
  return QNetworkProxy::HttpProxy;
}

std::string Settings::proxyHost() const {
  return value(Key::kProxyHost).toString().toStdString();
}

int Settings::proxyPort() const {
  return value(Key::kProxyPort, -1).toInt();
}

std::string Settings::proxyUsername() const {
  return value(Key::kProxyUsername).toString().toStdString();
}

std::string Settings::proxyPassword() const {
  return value(Key::kProxyPassword).toString().toStdString();
}

std::string Settings::service() const {
  return value(Key::kService, sync::serviceSlug(sync::ServiceId::AniList)).toString().toStdString();
}

bool Settings::sharingEnabled() const {
  return value(Key::kSharingEnabled, true).toBool();
}

bool Settings::streamingMediaEnabled() const {
  return value(Key::kStreamingMediaEnabled, false).toBool();
}

bool Settings::syncEnabled() const {
  return value(Key::kSyncEnabled, true).toBool();
}

anime::TitleLanguage Settings::titleLanguage() const {
  if (!titleLanguageCache_) {
    const auto language = value(Key::kTitleLanguage, u"romaji"_s).toString();
    if (language == u"english") {
      titleLanguageCache_ = anime::TitleLanguage::English;
    } else if (language == u"native") {
      titleLanguageCache_ = anime::TitleLanguage::Native;
    } else {
      titleLanguageCache_ = anime::TitleLanguage::Romaji;
    }
  }
  return *titleLanguageCache_;
}

std::chrono::seconds Settings::updateDelay() const {
  const std::chrono::seconds delay{value(Key::kUpdateDelay, 120).toInt()};
  return std::clamp(delay, kUpdateDelayMin, kUpdateDelayMax);
}

bool Settings::updateLibraryOnly() const {
  return value(Key::kUpdateLibraryOnly, false).toBool();
}

bool Settings::updatePauseWhenUnfocused() const {
  return value(Key::kUpdatePauseWhenUnfocused, false).toBool();
}

track::UpdateTrigger Settings::updateTrigger() const {
  const auto trigger = value(Key::kUpdateTrigger, u"afterDelay"_s).toString();
  if (trigger == u"onPlayerClose") return track::UpdateTrigger::OnPlayerClose;
  return track::UpdateTrigger::AfterDelay;
}

////////////////////////////////////////////////////////////////////////////////

void Settings::setAppColorScheme(const Qt::ColorScheme scheme) const {
  setValue(Key::kAppColorScheme, static_cast<int>(scheme));
}

void Settings::setAppStyle(const std::string& style) const {
  setValue(Key::kAppStyle, style);
}

void Settings::setDetectionEnabled(const bool enabled) const {
  setValue(Key::kDetectionEnabled, enabled);
}

void Settings::setDisabledMediaPlayers(std::vector<std::string> players) const {
  setValue(Key::kDisabledMediaPlayers, QJsonArray::fromStringList(toStringList(players)));
}

void Settings::setDiscordEnabled(const bool enabled) const {
  setValue(Key::kDiscordEnabled, enabled);
}

void Settings::setHttpEnabled(const bool enabled) const {
  setValue(Key::kHttpEnabled, enabled);
}

void Settings::setHttpHeaders(std::vector<std::string> headers) const {
  setValue(Key::kHttpHeaders, QJsonArray::fromStringList(toStringList(headers)));
}

void Settings::setHttpUrl(const std::string& url) const {
  setValue(Key::kHttpUrl, url);
}

void Settings::setLibraryFolders(std::vector<std::string> folders) const {
  setValue(Key::kLibraryFolders, QJsonArray::fromStringList(toStringList(folders)));
}

void Settings::setMediaDetectionInterval(const std::chrono::milliseconds interval) const {
  setValue(Key::kMediaDetectionInterval, static_cast<qint64>(interval.count()));
}

void Settings::setProxyType(const QNetworkProxy::ProxyType type) const {
  const std::string slug = type == QNetworkProxy::Socks5Proxy ? "socks5" : "http";
  setValue(Key::kProxyType, slug);
}

void Settings::setProxyHost(const std::string& host) const {
  setValue(Key::kProxyHost, host);
}

void Settings::setProxyPort(const int port) const {
  setValue(Key::kProxyPort, port);
}

void Settings::setProxyUsername(const std::string& username) const {
  setValue(Key::kProxyUsername, username);
}

void Settings::setProxyPassword(const std::string& password) const {
  setValue(Key::kProxyPassword, password);
}

void Settings::setService(const std::string& service) const {
  setValue(Key::kService, service);
}

void Settings::setSharingEnabled(const bool enabled) const {
  setValue(Key::kSharingEnabled, enabled);
}

void Settings::setStreamingMediaEnabled(const bool enabled) const {
  setValue(Key::kStreamingMediaEnabled, enabled);
}

void Settings::setSyncEnabled(const bool enabled) const {
  setValue(Key::kSyncEnabled, enabled);
}

void Settings::setTitleLanguage(const anime::TitleLanguage language) const {
  const auto slug = [language]() -> std::string {
    switch (language) {
      default:
      case anime::TitleLanguage::Romaji:
        return "romaji";
      case anime::TitleLanguage::English:
        return "english";
      case anime::TitleLanguage::Native:
        return "native";
    }
  }();
  setValue(Key::kTitleLanguage, slug);
  titleLanguageCache_ = language;
}

void Settings::setUpdateDelay(const std::chrono::seconds delay) const {
  setValue(Key::kUpdateDelay, static_cast<int>(delay.count()));
}

void Settings::setUpdateLibraryOnly(const bool enabled) const {
  setValue(Key::kUpdateLibraryOnly, enabled);
}

void Settings::setUpdatePauseWhenUnfocused(const bool enabled) const {
  setValue(Key::kUpdatePauseWhenUnfocused, enabled);
}

void Settings::setUpdateTrigger(const track::UpdateTrigger trigger) const {
  const std::string slug =
      trigger == track::UpdateTrigger::OnPlayerClose ? "onPlayerClose" : "afterDelay";
  setValue(Key::kUpdateTrigger, slug);
}

Settings* settings() {
  static auto instance = new Settings{qApp};
  return instance;
}

}  // namespace taiga
