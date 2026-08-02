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

#include <anitomy.hpp>
#include <string>
#include <vector>

namespace track {

class Episode final {
public:
  Episode();

  int animeId() const;
  void setAnimeId(int id);

  const std::vector<anitomy::Element>& elements() const noexcept;
  void setElements(std::vector<anitomy::Element>& elements);
  
  bool contains(const anitomy::ElementKind kind) const;
  std::string element(const anitomy::ElementKind kind, const std::string placeholder = {}) const;
  std::vector<std::string> elements(const anitomy::ElementKind kind) const;
  void addElement(const anitomy::ElementKind kind, const std::string& value);

private:
  auto find(const anitomy::ElementKind kind) const;

  int anime_id_;
  std::vector<anitomy::Element> elements_;
};

}  // namespace track
