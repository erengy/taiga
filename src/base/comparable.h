/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#ifndef TAIGA_BASE_COMPARE_H
#define TAIGA_BASE_COMPARE_H

namespace base {

enum CompareResult {
  kLessThan = -1,
  kEqualTo = 0,
  kGreaterThan = 1,
};

template <typename T>
class Comparable {
public:
  bool operator==(const T& rhs) const {
    return Compare(rhs) == kEqualTo;
  }
  bool operator!=(const T& rhs) const {
    return !operator==(rhs);
  }
  bool operator<(const T& rhs) const {
    return Compare(rhs) == kLessThan;
  }
  bool operator<=(const T& rhs) const {
    return !operator>(rhs);
  }
  bool operator>(const T& rhs) const {
    return Compare(rhs) == kGreaterThan;
  }
  bool operator>=(const T& rhs) const {
    return !operator<(rhs);
  }

private:
  virtual CompareResult Compare(const T& rhs) const = 0;
};

}  // namespace base

#endif  // TAIGA_BASE_COMPARE_H