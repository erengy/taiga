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
#include "base/json.h"
#include "base/log.h"
#include "sync/service.h"
#include "taiga/path.h"
#include "taiga/settings.h"
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
      if (!it.r0.second || it.r0.second - episode_number >= 0) {
        int destination = it.r1.first + distance;
        if (!it.r1.second || destination <= it.r1.second) {
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
      id_pattern + L"\\|" + id_pattern + L":" + episode_pattern);

  std::match_results<std::wstring::const_iterator> match_results;

  if (std::regex_match(rule, match_results, pattern)) {
    int id0 = 0;
    switch (taiga::GetCurrentServiceId()) {
      case sync::kMyAnimeList:
        id0 = ToInt(match_results[1].str());
        break;
      case sync::kHummingbird:
        id0 = ToInt(match_results[2].str());
        break;
    }
    if (!id0)
      return false;

    std::pair<int, int> r0;
    r0.first = ToInt(match_results[3].str());
    if (match_results[4].matched)
      r0.second = ToInt(match_results[4].str());

    int id1 = 0;
    switch (taiga::GetCurrentServiceId()) {
      case sync::kMyAnimeList:
        id1 = ToInt(match_results[5].str());
        break;
      case sync::kHummingbird:
        id1 = ToInt(match_results[6].str());
        break;
    }
    if (!id1)
      id1 = id0;

    std::pair<int, int> r1;
    r1.first = ToInt(match_results[7].str());
    if (match_results[8].matched)
      r1.second = ToInt(match_results[8].str());

    auto& relation = relations[id0];
    relation.AddRange(id1, r0, r1);
    return true;
  }

  return false;
}

bool Engine::ReadRelations() {
  relations.clear();

  std::wstring path = taiga::GetPath(taiga::kPathDatabaseAnimeRelations);
  std::string document;

  if (ReadFromFile(path, document)) {
    // Erase comments
    size_t pos = 0;
    do {
      pos = document.find("//");
      if (pos != document.npos) {
        size_t pos_end = document.find('\n', pos);
        document.erase(pos, (pos_end - pos) + 1);
      }
    } while (pos != document.npos);

    Json::Reader reader;
    Json::Value root;
    if (reader.parse(document, root)) {
      for (const auto& item : root["rules"]) {
        auto rule = StrToWstr(item.asString());
        if (!ParseRule(rule)) {
          LOG(LevelWarning, L"Could not parse rule: " + rule);
        }
      }
      return true;
    }
  }

  LOG(LevelWarning, L"Could not read anime relations data.");
  return false;
}

////////////////////////////////////////////////////////////////////////////////

bool Engine::SearchEpisodeRedirection(int id, int episode_number,
                                      std::pair<int, int>& result) const {
  auto it = relations.find(id);
  if (it != relations.end()) {
    const auto& relation = it->second;
    if (relation.FindRange(episode_number, result))
      return true;
  }

  return false;
}

}  // namespace recognition
}  // namespace track