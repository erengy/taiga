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
#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "base/settings.hpp"
#include "media/anime.hpp"
#include "track/update_trigger.hpp"

namespace taiga {

class Settings final : public base::Settings {
public:
  static constexpr std::chrono::seconds kUpdateDelayMin{10};
  static constexpr std::chrono::seconds kUpdateDelayMax{3600};

  void init() const;

  Qt::ColorScheme appColorScheme() const;
  bool detectionEnabled() const;
  std::vector<std::string> disabledMediaPlayers() const;
  std::string service() const;
  std::vector<std::string> libraryFolders() const;
  std::chrono::milliseconds mediaDetectionInterval() const;
  QNetworkProxy::ProxyType proxyType() const;
  std::string proxyHost() const;
  int proxyPort() const;
  std::string proxyUsername() const;
  std::string proxyPassword() const;
  bool streamingMediaEnabled() const;
  bool syncEnabled() const;
  anime::TitleLanguage titleLanguage() const;
  std::chrono::seconds updateDelay() const;
  bool updateLibraryOnly() const;
  bool updatePauseWhenUnfocused() const;
  track::UpdateTrigger updateTrigger() const;

  void setAppColorScheme(const Qt::ColorScheme scheme) const;
  void setDetectionEnabled(const bool enabled) const;
  void setDisabledMediaPlayers(std::vector<std::string> players) const;
  void setService(const std::string& service) const;
  void setLibraryFolders(std::vector<std::string> folders) const;
  void setMediaDetectionInterval(const std::chrono::milliseconds interval) const;
  void setProxyType(const QNetworkProxy::ProxyType type) const;
  void setProxyHost(const std::string& host) const;
  void setProxyPort(const int port) const;
  void setProxyUsername(const std::string& username) const;
  void setProxyPassword(const std::string& password) const;
  void setStreamingMediaEnabled(const bool enabled) const;
  void setSyncEnabled(const bool enabled) const;
  void setTitleLanguage(const anime::TitleLanguage language) const;
  void setUpdateDelay(const std::chrono::seconds delay) const;
  void setUpdateLibraryOnly(const bool enabled) const;
  void setUpdatePauseWhenUnfocused(const bool enabled) const;
  void setUpdateTrigger(const track::UpdateTrigger trigger) const;

private:
  QString fileName() const override;

  mutable std::optional<anime::TitleLanguage> titleLanguageCache_;
};

inline Settings settings;

}  // namespace taiga
