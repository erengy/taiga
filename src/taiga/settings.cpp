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
    compat::v1::readSettings(std::format("{}/v1/settings.xml", getDataPath()), *this, *accounts());
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
  return u"%1/settings.json"_s.arg(getDataPath());
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

bool Settings::appCloseToTray() const {
  return value(Key::kAppCloseToTray, true).toBool();
}

Qt::ColorScheme Settings::appColorScheme() const {
  const auto scheme = value(Key::kAppColorScheme, u"system"_s).toString();
  if (scheme == u"light") return Qt::ColorScheme::Light;
  if (scheme == u"dark") return Qt::ColorScheme::Dark;
  return Qt::ColorScheme::Unknown;
}

bool Settings::appHideNsfw() const {
  return value(Key::kAppHideNsfw, true).toBool();
}

bool Settings::appStartInTray() const {
  return value(Key::kAppStartInTray, false).toBool();
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

anime::TitleLanguage Settings::appTitleLanguage() const {
  if (!appTitleLanguageCache_) {
    const auto language = value(Key::kAppTitleLanguage, u"romaji"_s).toString();
    if (language == u"english") {
      appTitleLanguageCache_ = anime::TitleLanguage::English;
    } else if (language == u"native") {
      appTitleLanguageCache_ = anime::TitleLanguage::Native;
    } else {
      appTitleLanguageCache_ = anime::TitleLanguage::Romaji;
    }
  }
  return *appTitleLanguageCache_;
}

bool Settings::detectionEnabled() const {
  return value(Key::kDetectionEnabled, true).toBool();
}

std::chrono::milliseconds Settings::detectionInterval() const {
  const auto interval = value(Key::kDetectionInterval, 3000).toInt();
  return std::chrono::milliseconds{interval};
}

std::vector<std::string> Settings::disabledPlayers() const {
  return value(Key::kDisabledPlayers).toJsonArray().toVariantList() |
         std::views::transform([](const QVariant& v) { return v.toString().toStdString(); }) |
         std::ranges::to<std::vector>();
}

bool Settings::discordEnabled() const {
  return value(Key::kDiscordEnabled, false).toBool();
}

std::vector<std::string> Settings::libraryFolders() const {
  return value(Key::kLibraryFolders).toJsonArray().toVariantList() |
         std::views::transform([](const QVariant& v) { return v.toString().toStdString(); }) |
         std::ranges::to<std::vector>();
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

bool Settings::sharingEnabled() const {
  return value(Key::kSharingEnabled, true).toBool();
}

bool Settings::streamingDetectionEnabled() const {
  return value(Key::kStreamingDetectionEnabled, false).toBool();
}

bool Settings::syncEnabled() const {
  return value(Key::kSyncEnabled, true).toBool();
}

std::string Settings::syncService() const {
  return value(Key::kSyncService, sync::serviceSlug(sync::ServiceId::AniList))
      .toString()
      .toStdString();
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

bool Settings::webhookEnabled() const {
  return value(Key::kWebhookEnabled, false).toBool();
}

std::vector<std::string> Settings::webhookHeaders() const {
  return value(Key::kWebhookHeaders).toJsonArray().toVariantList() |
         std::views::transform([](const QVariant& v) { return v.toString().toStdString(); }) |
         std::ranges::to<std::vector>();
}

std::string Settings::webhookUrl() const {
  return value(Key::kWebhookUrl).toString().toStdString();
}

////////////////////////////////////////////////////////////////////////////////

void Settings::setAppCloseToTray(const bool enabled) const {
  setValue(Key::kAppCloseToTray, enabled);
}

void Settings::setAppColorScheme(const Qt::ColorScheme scheme) const {
  const auto slug = [scheme]() -> std::string {
    switch (scheme) {
      default:
      case Qt::ColorScheme::Unknown:
        return "system";
      case Qt::ColorScheme::Light:
        return "light";
      case Qt::ColorScheme::Dark:
        return "dark";
    }
  }();
  setValue(Key::kAppColorScheme, slug);
}

void Settings::setAppHideNsfw(const bool enabled) const {
  setValue(Key::kAppHideNsfw, enabled);
}

void Settings::setAppStartInTray(const bool enabled) const {
  setValue(Key::kAppStartInTray, enabled);
}

void Settings::setAppStyle(const std::string& style) const {
  setValue(Key::kAppStyle, style);
}

void Settings::setAppTitleLanguage(const anime::TitleLanguage language) const {
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
  setValue(Key::kAppTitleLanguage, slug);
  appTitleLanguageCache_ = language;
}

void Settings::setDetectionEnabled(const bool enabled) const {
  setValue(Key::kDetectionEnabled, enabled);
}

void Settings::setDetectionInterval(const std::chrono::milliseconds interval) const {
  setValue(Key::kDetectionInterval, static_cast<qint64>(interval.count()));
}

void Settings::setDisabledPlayers(const std::vector<std::string>& players) const {
  setValue(Key::kDisabledPlayers, QJsonArray::fromStringList(toStringList(players)));
}

void Settings::setDiscordEnabled(const bool enabled) const {
  setValue(Key::kDiscordEnabled, enabled);
}

void Settings::setLibraryFolders(const std::vector<std::string>& folders) const {
  setValue(Key::kLibraryFolders, QJsonArray::fromStringList(toStringList(folders)));
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

void Settings::setSharingEnabled(const bool enabled) const {
  setValue(Key::kSharingEnabled, enabled);
}

void Settings::setStreamingDetectionEnabled(const bool enabled) const {
  setValue(Key::kStreamingDetectionEnabled, enabled);
}

void Settings::setSyncEnabled(const bool enabled) const {
  setValue(Key::kSyncEnabled, enabled);
}

void Settings::setSyncService(const std::string& service) const {
  setValue(Key::kSyncService, service);
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

void Settings::setWebhookEnabled(const bool enabled) const {
  setValue(Key::kWebhookEnabled, enabled);
}

void Settings::setWebhookHeaders(const std::vector<std::string>& headers) const {
  setValue(Key::kWebhookHeaders, QJsonArray::fromStringList(toStringList(headers)));
}

void Settings::setWebhookUrl(const std::string& url) const {
  setValue(Key::kWebhookUrl, url);
}

Settings* settings() {
  static auto instance = new Settings{qApp};
  return instance;
}

}  // namespace taiga
