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
#include "settings_page_media_players.hpp"

#include <QListWidget>
#include <algorithm>
#include <string>
#include <vector>

#include "base/string.hpp"
#include "taiga/settings.hpp"
#include "track/media_player.hpp"
#include "ui_settings_dialog.h"

namespace gui {

namespace {

// Web browsers are handled by the streaming page.
std::vector<track::media::Player> getMediaPlayers() {
  std::vector<track::media::Player> players;
  if (track::media::parsePlayersData(players)) {
    std::erase_if(players, [](const track::media::Player& player) {
      return player.type == anisthesia::PlayerType::WebBrowser;
    });
  }
  return players;
}

bool containsName(const std::vector<std::string>& names, const std::string& name) {
  return std::ranges::any_of(names, [&name](const std::string& n) {
    return compareStrings(n, name, Qt::CaseInsensitive) == 0;
  });
}

}  // namespace

SettingsPageMediaPlayers::SettingsPageMediaPlayers(Ui::SettingsDialog* ui, QDialog* dialog)
    : SettingsPage(ui, dialog) {}

void SettingsPageMediaPlayers::load() {
  const auto disabledPlayers = taiga::settings.disabledMediaPlayers();

  for (const auto& player : getMediaPlayers()) {
    auto item = new QListWidgetItem(QString::fromStdString(player.name), ui_->mediaPlayersList);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(containsName(disabledPlayers, player.name) ? Qt::Unchecked : Qt::Checked);
  }
}

void SettingsPageMediaPlayers::apply() const {
  const auto players = getMediaPlayers();

  // Keep disabled names that aren't listed on this page.
  auto disabledPlayers = taiga::settings.disabledMediaPlayers();
  std::erase_if(disabledPlayers, [&players](const std::string& name) {
    return std::ranges::any_of(players, [&name](const track::media::Player& player) {
      return compareStrings(player.name, name, Qt::CaseInsensitive) == 0;
    });
  });

  for (int i = 0; i < ui_->mediaPlayersList->count(); ++i) {
    const auto item = ui_->mediaPlayersList->item(i);
    if (item->checkState() == Qt::Unchecked) {
      disabledPlayers.push_back(item->text().toStdString());
    }
  }

  taiga::settings.setDisabledMediaPlayers(std::move(disabledPlayers));
}

}  // namespace gui
