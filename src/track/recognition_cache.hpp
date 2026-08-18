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

#include <QObject>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>

namespace anime {
struct Details;
};

namespace track::recognition {

class Cache final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Cache)

public:
  Cache();

  struct Data {
    struct Match {
      int id;
      float weight;
    };

    std::unordered_map<int, Match> matches;
  };

  bool empty() const;
  const std::optional<Data> find(const std::string& title) const;

  void clear();
  void init();

  void add(const anime::Details& item);
  void remove(const anime::Details& item);
  void update(const anime::Details& item);

private:
  std::unordered_map<std::string, Data> titles_;
};

inline Cache* cache() {
  static auto cache = new Cache();
  return cache;
}

}  // namespace track::recognition
