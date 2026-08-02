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

#include "media_player.hpp"

#include "base/file.hpp"

namespace track::media {

bool parsePlayersData(std::vector<Player>& players) {
  const auto file = base::readFile(":/players.anisthesia");

  if (file.isEmpty()) {
    return false;
  }

  // @TODO: Allow user to override via file in data directory
  return anisthesia::ParsePlayersData(file.toStdString(), players);
}

std::vector<Player> getEnabledPlayers(const std::vector<Player>& players) {
  std::vector<Player> enabledPlayers;

  // @TODO: Enable web browser detection
  // @TODO: Check taiga::settings
  for (const auto player : players) {
    if (player.type != anisthesia::PlayerType::WebBrowser) {
      enabledPlayers.emplace_back(player);
    }
  }

  return enabledPlayers;
}

}  // namespace track::media
