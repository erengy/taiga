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
#include "settings_player_list.hpp"

#include <QListWidget>
#include <algorithm>
#include <string>
#include <vector>

#include "base/string.hpp"
#include "taiga/settings.hpp"
#include "track/media_player.hpp"

namespace gui {

namespace {

std::vector<track::media::Player> getPlayers(const anisthesia::PlayerType type) {
  std::vector<track::media::Player> players;
  if (track::media::parsePlayersData(players)) {
    std::erase_if(players,
                  [type](const track::media::Player& player) { return player.type != type; });
  }
  return players;
}

bool isSameName(const std::string& a, const std::string& b) {
  return compareStrings(a, b, Qt::CaseInsensitive) == 0;
}

bool containsName(const std::vector<std::string>& names, const std::string& name) {
  return std::ranges::any_of(names, [&name](const std::string& n) { return isSameName(n, name); });
}

}  // namespace

void loadPlayerList(QListWidget* list, const anisthesia::PlayerType type) {
  const auto disabledPlayers = taiga::settings.disabledMediaPlayers();

  for (const auto& player : getPlayers(type)) {
    auto item = new QListWidgetItem(QString::fromStdString(player.name), list);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(containsName(disabledPlayers, player.name) ? Qt::Unchecked : Qt::Checked);
  }
}

void savePlayerList(const QListWidget* list, const anisthesia::PlayerType type) {
  const auto players = getPlayers(type);

  // Keep disabled names that aren't listed here.
  auto disabledPlayers = taiga::settings.disabledMediaPlayers();
  std::erase_if(disabledPlayers, [&players](const std::string& name) {
    return std::ranges::any_of(players, [&name](const track::media::Player& player) {
      return isSameName(player.name, name);
    });
  });

  for (int i = 0; i < list->count(); ++i) {
    const auto item = list->item(i);
    if (item->checkState() == Qt::Unchecked) {
      disabledPlayers.push_back(item->text().toStdString());
    }
  }

  taiga::settings.setDisabledMediaPlayers(std::move(disabledPlayers));
}

}  // namespace gui
