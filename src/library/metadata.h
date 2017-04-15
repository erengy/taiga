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

#include "base/time.h"
#include "base/types.h"

namespace library {

enum class TitleType {
  Unknown,
  Synonym,
  LangEnglish,
  LangJapanese,
};

struct Title {
  Title();
  Title(TitleType type, const string_t& value);
  ~Title() {}

  TitleType type;
  string_t value;
};

// A generic metadata structure for all kinds of media
struct Metadata {
  Metadata();
  ~Metadata() {}

  std::vector<string_t> uid;
  enum_t source;
  time_t modified;

  string_t title;
  std::vector<Title> alternative;

  enum_t type;
  enum_t status;
  enum_t audience;

  std::vector<unsigned short> extent;
  std::vector<Date> date;

  std::vector<string_t> subject;
  std::vector<string_t> creator;
  std::vector<string_t> resource;
  std::vector<string_t> community;

  string_t description;
};

}  // namespace library
