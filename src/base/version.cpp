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

#include <vector>

#include "string.h"
#include "version.h"

namespace base {

SemanticVersion::SemanticVersion()
    : major(1), minor(0), patch(0) {
}

SemanticVersion::SemanticVersion(const string_t& version)
    : major(1), minor(0), patch(0) {
  Parse(version);
}

SemanticVersion::SemanticVersion(numeric_identifier_t major,
                                 numeric_identifier_t minor,
                                 numeric_identifier_t patch)
    : major(major), minor(minor), patch(patch) {
}

SemanticVersion& SemanticVersion::operator=(const SemanticVersion& version) {
  major = version.major;
  minor = version.minor;
  patch = version.patch;

  prerelease_identifiers = version.prerelease_identifiers;
  build_metadata = version.build_metadata;

  return *this;
}

SemanticVersion::operator string_t() const {
  string_t version = ToWstr(static_cast<int>(major)) + L"." +
                     ToWstr(static_cast<int>(minor)) + L"." +
                     ToWstr(static_cast<int>(patch));
  if (!prerelease_identifiers.empty())
    version += L"-" + prerelease_identifiers;
  if (!build_metadata.empty())
    version += L"+" + build_metadata;

  return version;
}

////////////////////////////////////////////////////////////////////////////////

CompareResult SemanticVersion::Compare(const SemanticVersion& version) const {
  if (major != version.major)
    return major < version.major ? kLessThan : kGreaterThan;
  if (minor != version.minor)
    return minor < version.minor ? kLessThan : kGreaterThan;
  if (patch != version.patch)
    return patch < version.patch ? kLessThan : kGreaterThan;

  if (prerelease_identifiers != version.prerelease_identifiers) {
    if (prerelease_identifiers.empty() &&
        !version.prerelease_identifiers.empty())
      return kGreaterThan;
    if (!prerelease_identifiers.empty() &&
        version.prerelease_identifiers.empty())
      return kLessThan;

    std::vector<string_t> identifiers_, identifiers;
    Split(prerelease_identifiers, L".", identifiers_);
    Split(version.prerelease_identifiers, L".", identifiers);

    size_t min_size = min(identifiers_.size(), identifiers.size());
    for (size_t i = 0; i < min_size; ++i) {
      if (IsNumeric(identifiers_.at(i)) && IsNumeric(identifiers.at(i))) {
        int lhs = ToInt(identifiers_.at(i));
        int rhs = ToInt(identifiers.at(i));
        if (lhs != rhs)
          return lhs < rhs ? kLessThan : kGreaterThan;
      } else {
        int result = CompareStrings(identifiers_.at(i), identifiers.at(i));
        if (result != 0)
          return result < 0 ? kLessThan : kGreaterThan;
      }
    }

    if (identifiers_.size() != identifiers.size())
      return identifiers_.size() < identifiers.size() ?
          kLessThan : kGreaterThan;
  }

  return kEqualTo;
}

void SemanticVersion::Parse(const string_t& version) {
  int identifier_index = kMajor;
  size_t current_identifier_pos = 0;

  for (size_t i = 0; i < version.size(); ++i) {
    switch (identifier_index) {
      case kMajor:
      case kMinor:
      case kPatch: {
        if (IsNumeric(version.at(i)))
          continue;
        string_t current_identifier = version.substr(
            current_identifier_pos, i - current_identifier_pos);
        switch (identifier_index) {
          case kMajor:
            major = ToInt(current_identifier);
            break;
          case kMinor:
            minor = ToInt(current_identifier);
            break;
          case kPatch:
            patch = ToInt(current_identifier);
            break;
        }
        switch (version.at(i)) {
          case L'.':
            ++identifier_index;
            break;
          case L'-':
            identifier_index = kPreRelease;
            break;
          case L'+':
            identifier_index = kBuildMetadata;
            break;
        }
        current_identifier_pos = i + 1;
        break;
      }

      case kPreRelease: {
        if (version.at(i) != L'+' && i < version.size() - 1)
          continue;
        prerelease_identifiers = version.substr(
            current_identifier_pos, i - current_identifier_pos);
        identifier_index = kBuildMetadata;
        current_identifier_pos = i + 1;
        break;
      }

      case kBuildMetadata: {
        if (i < version.size() - 1)
          continue;
        build_metadata = version.substr(
            current_identifier_pos, i - current_identifier_pos);
        break;
      }
    }
  }
}

}  // namespace base