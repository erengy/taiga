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

#include <regex>

#include "base/file.h"
#include "base/log.h"
#include "base/version.h"
#include "sync/service.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/recognition.h"

namespace track {
namespace recognition {

class Relation {
public:
  typedef std::pair<int, int> int_pair_t;

  void AddRange(int id, int_pair_t r1, int_pair_t r2);
  bool FindRange(int episode_number, int_pair_t& result) const;

private:
  struct Range {
    int id;
    int_pair_t r0;
    int_pair_t r1;
  };

  std::vector<Range> ranges_;
};

std::map<int, Relation> relations;

////////////////////////////////////////////////////////////////////////////////

void Relation::AddRange(int id, int_pair_t r1, int_pair_t r2) {
  ranges_.push_back({id, r1, r2});
}

bool Relation::FindRange(int episode_number, int_pair_t& result) const {
  for (const auto& it : ranges_) {
    int distance = episode_number - it.r0.first;
    if (distance >= 0) {
      if (it.r0.second - episode_number >= 0) {
        int destination = it.r1.first;
        if (it.r1.first != it.r1.second)
          destination += distance;
        if (destination <= it.r1.second) {
          result.first = it.id;
          result.second = destination;
          return true;
        }
      }
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////

static bool ParseRule(const std::wstring& rule) {
  static std::wstring id_pattern = L"(\\d+|[?~])";
  static std::wstring episode_pattern = L"(\\d+)(?:-(\\d+|\\?))?";
  static const std::wregex pattern(
      id_pattern + L"\\|" + id_pattern + L":" + episode_pattern + L" -> " +
      id_pattern + L"\\|" + id_pattern + L":" + episode_pattern + L"(!)?");

  std::match_results<std::wstring::const_iterator> match_results;

  if (std::regex_match(rule, match_results, pattern)) {
    auto get_id = [&](size_t first, size_t second) {
      switch (taiga::GetCurrentServiceId()) {
        case sync::kMyAnimeList:
          return ToInt(match_results[first].str());
        case sync::kHummingbird:
          return ToInt(match_results[second].str());
        default:
          return 0;
      }
    };

    auto get_range = [&](size_t first, size_t second) {
      std::pair<int, int> range;
      range.first = ToInt(match_results[first].str());
      if (match_results[second].matched) {
        if (IsNumericString(match_results[second].str())) {
          range.second = ToInt(match_results[second].str());
        } else {
          range.second = INT_MAX;
        }
      } else {
        range.second = range.first;
      }
      return range;
    };

    int id0 = get_id(1, 2);
    if (!id0)
      return false;
    auto r0 = get_range(3, 4);

    int id1 = get_id(5, 6);
    if (!id1)
      id1 = id0;
    auto r1 = get_range(7, 8);

    relations[id0].AddRange(id1, r0, r1);

    if (match_results[9].matched)
      relations[id1].AddRange(id1, r0, r1);

    return true;
  }

  return false;
}

bool Engine::ReadRelations() {
  std::wstring path = taiga::GetPath(taiga::kPathDatabaseAnimeRelations);
  std::string document;

  if (!ReadFromFile(path, document)) {
    LOG(LevelWarning, L"Could not read anime relations data.");
    Settings.Set(taiga::kRecognition_RelationsLastModified, std::wstring());
    return false;
  }

  return ReadRelations(document);
}

bool Engine::ReadRelations(const std::string& document) {
  relations.clear();

  std::vector<std::wstring> lines;
  Split(StrToWstr(document), L"\n", lines);

  enum FileSections {
    kUnknownSection,
    kMetaSection,
    kRulesSection,
  };
  auto current_section = kUnknownSection;

  for (auto& line : lines) {
    Trim(line, L"\r ");

    if (line.empty())
      continue;
    if (line.front() == L'#')  // comment
      continue;

    if (StartsWith(line, L"::")) {
      auto section = line.substr(2);
      if (section == L"meta") {
        current_section = kMetaSection;
      } else if (section == L"rules") {
        current_section = kRulesSection;
      } else {
        current_section = kUnknownSection;
      }
      continue;
    }

    switch (current_section) {
      case kMetaSection: {
        TrimLeft(line, L"- ");
        static const std::wregex pattern(L"([a-z_]+): (.+)");
        std::match_results<std::wstring::const_iterator> match_results;
        if (std::regex_match(line, match_results, pattern)) {
          auto name = match_results[1].str();
          auto value = match_results[2].str();
          if (name == L"version") {
            base::SemanticVersion version(value);
            if (version > Taiga.version)
              LOG(LevelDebug, L"Anime relations version is larger than "
                              L"application version.");
          } else if (name == L"last_modified") {
            Settings.Set(taiga::kRecognition_RelationsLastModified, value);
          }
        }
        break;
      }
      case kRulesSection: {
        TrimLeft(line, L"- ");
        if (!ParseRule(line))
          LOG(LevelWarning, L"Could not parse rule: " + line);
        break;
      }
    }
  }

  return !relations.empty();
}

////////////////////////////////////////////////////////////////////////////////

bool Engine::SearchEpisodeRedirection(
    int id, const std::pair<int, int>& range,
    int& destination_id, std::pair<int, int>& destination_range) const {

  auto it = relations.find(id);

  if (it == relations.end())
    return false;

  const auto& relation = it->second;

  std::pair<std::pair<int, int>, std::pair<int, int>> results;

  if (!relation.FindRange(range.first, results.first))
    return false;

  if (range.first != range.second) {
    if (!relation.FindRange(range.second, results.second))
      return false;
    if (results.first.first != results.second.first)
      return false;
  }

  destination_id = results.first.first;
  destination_range.first = results.first.second;
  destination_range.second = results.second.second;

  return true;
}

}  // namespace recognition
}  // namespace track