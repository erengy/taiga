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

#ifndef TAIGA_BASE_VERSION_H
#define TAIGA_BASE_VERSION_H

#include <string>

#include "comparable.h"

namespace base {

// An implementation of Semantic Versioning 2.0.0 that provides an easy way
// to compare version numbers.
//
// See http://semver.org for Semantic Versioning Specification.

class SemanticVersion : public Comparable<SemanticVersion> {
public:
  typedef unsigned int numeric_identifier_t;
  typedef std::wstring string_t;

  enum Version {
    kMajor,
    kMinor,
    kPatch,
    kPreRelease,
    kBuildMetadata
  };

  SemanticVersion();
  SemanticVersion(const string_t& version);
  SemanticVersion(numeric_identifier_t major,
                  numeric_identifier_t minor,
                  numeric_identifier_t patch);
  ~SemanticVersion() {}

  SemanticVersion& operator = (const SemanticVersion& version);

  operator string_t() const;

  numeric_identifier_t major;
  numeric_identifier_t minor;
  numeric_identifier_t patch;
  string_t prerelease_identifiers;
  string_t build_metadata;

private:
  CompareResult Compare(const SemanticVersion& version) const;
  void Parse(const string_t& version);
};

}  // namespace base

#endif  // TAIGA_BASE_VERSION_H