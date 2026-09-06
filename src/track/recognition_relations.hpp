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

#include <QString>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sync {
enum class ServiceId;
}

namespace track::recognition {

struct Redirection {
  int id;
  std::pair<int, int> episode_range;
};

class Relations {
public:
  void load(const QString& contents, const int column);
  std::optional<Redirection> find(const int id, const int episode_number) const;

private:
  struct Rule {
    int destination_id;
    std::pair<int, int> source_range;
    std::pair<int, int> destination_range;
  };

  void parseRule(const QString& rule, const int column);
  void addRule(const int id, const std::pair<int, int>& source_range, const int destination_id,
               const std::pair<int, int>& destination_range);

  std::unordered_map<int, std::vector<Rule>> rules_;
};

std::optional<Redirection> findRedirection(const int id, const std::pair<int, int>& episode_range);

}  // namespace track::recognition
