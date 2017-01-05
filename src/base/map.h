/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <map>

namespace base {

// A multimap implementation that allows accessing the first value that is
// mapped to a given key.

template<class key_type, class mapped_type>
class multimap : public std::multimap<key_type, mapped_type> {
public:
  mapped_type& operator[] (const key_type& key) {
    auto it = this->find(key);
    if (it == this->end())
      it = this->insert(std::pair<key_type, mapped_type>(key, mapped_type()));
    return it->second;
  }
};

}  // namespace base
