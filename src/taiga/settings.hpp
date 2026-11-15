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

#include <QNetworkProxy>
#include <QObject>
#include <QSet>
#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "base/settings.hpp"
#include "media/anime.hpp"
#include "track/update_trigger.hpp"

namespace taiga {

class Settings final : public QObject, public base::Settings {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Settings)

public:
  static constexpr QLatin1StringView kAppStyleSystem{"system"};
  static constexpr std::chrono::seconds kUpdateDelayMin{10};
  static constexpr std::chrono::seconds kUpdateDelayMax{3600};

  struct Key {
    static constexpr QLatin1StringView kAppCloseToTray{"app.closeToTray"};
    static constexpr QLatin1StringView kAppColorScheme{"app.colorScheme"};
    static constexpr QLatin1StringView kAppHideNsfw{"app.hideNsfw"};
    static constexpr QLatin1StringView kAppStartInTray{"app.startInTray"};
    static constexpr QLatin1StringView kAppStyle{"app.style"};
    static constexpr QLatin1StringView kAppTitleLanguage{"app.titleLanguage"};
    static constexpr QLatin1StringView kLibraryFolders{"library.folders"};
    static constexpr QLatin1StringView kMetaVersion{"meta.version"};
    static constexpr QLatin1StringView kProxyHost{"network.proxy.host"};
    static constexpr QLatin1StringView kProxyPassword{"network.proxy.password"};
    static constexpr QLatin1StringView kProxyPort{"network.proxy.port"};
    static constexpr QLatin1StringView kProxyType{"network.proxy.type"};
    static constexpr QLatin1StringView kProxyUsername{"network.proxy.username"};
    static constexpr QLatin1StringView kDiscordEnabled{"sharing.discord.enabled"};
    static constexpr QLatin1StringView kSharingEnabled{"sharing.enabled"};
    static constexpr QLatin1StringView kWebhookEnabled{"sharing.webhook.enabled"};
    static constexpr QLatin1StringView kWebhookHeaders{"sharing.webhook.headers"};
    static constexpr QLatin1StringView kWebhookUrl{"sharing.webhook.url"};
    static constexpr QLatin1StringView kSyncEnabled{"sync.enabled"};
    static constexpr QLatin1StringView kSyncService{"sync.service"};
    static constexpr QLatin1StringView kDisabledPlayers{"track.detection.disabledPlayers"};
    static constexpr QLatin1StringView kDetectionEnabled{"track.detection.enabled"};
    static constexpr QLatin1StringView kDetectionInterval{"track.detection.interval"};
    static constexpr QLatin1StringView kStreamingDetectionEnabled{
        "track.detection.streaming.enabled"};
    static constexpr QLatin1StringView kUpdateDelay{"track.update.delay"};
    static constexpr QLatin1StringView kUpdateLibraryOnly{"track.update.libraryOnly"};
    static constexpr QLatin1StringView kUpdatePauseWhenUnfocused{"track.update.pauseWhenUnfocused"};
    static constexpr QLatin1StringView kUpdateTrigger{"track.update.trigger"};
  };

  explicit Settings(QObject* parent);

  void init() const;

  bool appCloseToTray() const;
  Qt::ColorScheme appColorScheme() const;
  bool appHideNsfw() const;
  bool appStartInTray() const;
  std::string appStyle() const;
  anime::TitleLanguage appTitleLanguage() const;
  bool detectionEnabled() const;
  std::chrono::milliseconds detectionInterval() const;
  std::vector<std::string> disabledPlayers() const;
  bool discordEnabled() const;
  std::vector<std::string> libraryFolders() const;
  QNetworkProxy::ProxyType proxyType() const;
  std::string proxyHost() const;
  int proxyPort() const;
  std::string proxyUsername() const;
  std::string proxyPassword() const;
  bool sharingEnabled() const;
  bool streamingDetectionEnabled() const;
  bool syncEnabled() const;
  std::string syncService() const;
  std::chrono::seconds updateDelay() const;
  bool updateLibraryOnly() const;
  bool updatePauseWhenUnfocused() const;
  track::UpdateTrigger updateTrigger() const;
  bool webhookEnabled() const;
  std::vector<std::string> webhookHeaders() const;
  std::string webhookUrl() const;

  void setAppCloseToTray(const bool enabled) const;
  void setAppColorScheme(const Qt::ColorScheme scheme) const;
  void setAppHideNsfw(const bool enabled) const;
  void setAppStartInTray(const bool enabled) const;
  void setAppStyle(const std::string& style) const;
  void setAppTitleLanguage(const anime::TitleLanguage language) const;
  void setDetectionEnabled(const bool enabled) const;
  void setDetectionInterval(const std::chrono::milliseconds interval) const;
  void setDisabledPlayers(const std::vector<std::string>& players) const;
  void setDiscordEnabled(const bool enabled) const;
  void setLibraryFolders(const std::vector<std::string>& folders) const;
  void setProxyType(const QNetworkProxy::ProxyType type) const;
  void setProxyHost(const std::string& host) const;
  void setProxyPort(const int port) const;
  void setProxyUsername(const std::string& username) const;
  void setProxyPassword(const std::string& password) const;
  void setSharingEnabled(const bool enabled) const;
  void setStreamingDetectionEnabled(const bool enabled) const;
  void setSyncEnabled(const bool enabled) const;
  void setSyncService(const std::string& service) const;
  void setUpdateDelay(const std::chrono::seconds delay) const;
  void setUpdateLibraryOnly(const bool enabled) const;
  void setUpdatePauseWhenUnfocused(const bool enabled) const;
  void setUpdateTrigger(const track::UpdateTrigger trigger) const;
  void setWebhookEnabled(const bool enabled) const;
  void setWebhookHeaders(const std::vector<std::string>& headers) const;
  void setWebhookUrl(const std::string& url) const;

signals:
  // Emitted once per event loop iteration with the keys that were changed, as settings are often
  // changed together.
  void changed(const QSet<QString>& keys) const;

private:
  QString fileName() const override;
  void onValueChanged(QAnyStringView key) const override;

  mutable QSet<QString> changedKeys_;
  mutable std::optional<anime::TitleLanguage> appTitleLanguageCache_;
};

Settings* settings();

}  // namespace taiga
