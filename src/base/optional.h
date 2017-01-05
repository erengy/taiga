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

template <typename T>
class Optional {
public:
  Optional() : initialized_(false) {}
  Optional(const T& value) : initialized_(true), value_(value) {}
  virtual ~Optional() {}

  void Reset() {
    initialized_ = false;
  }

  operator bool() const {
    return initialized_;
  }

  const T& operator *() const {
    return value_;
  }

  const T* operator ->() const {
    return &value_;
  }

  T& operator =(const T& value) {
    value_ = value;
    initialized_ = true;
    return value_;
  }

private:
  bool initialized_;
  T value_;
};
