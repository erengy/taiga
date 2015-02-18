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
    int_pair_t r1;
    int_pair_t r2;
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
    int distance = episode_number - it.r1.first;
    if (distance >= 0) {
      if (!it.r1.second || it.r1.second - episode_number > 0) {
        int destination = it.r2.first + distance;
        if (!it.r2.second || destination <= it.r2.second) {
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

static bool ParseRule(const std::wstring& entry) {
  static std::wstring episode_range_pattern = L"(\\d+)(?:-(\\d+|\\?))?";
  static const std::wregex pattern(L"(\\d+):" + episode_range_pattern +
                                   L" -\\> (\\d+|~):" + episode_range_pattern);

  std::match_results<std::wstring::const_iterator> match_results;

  if (std::regex_match(entry, match_results, pattern)) {
    int id1 = ToInt(match_results[1].str());

    std::pair<int, int> r1;
    r1.first = ToInt(match_results[2].str());
    if (match_results[3].matched)
      r1.second = ToInt(match_results[3].str());

    int id2 = ToInt(match_results[4].str());
    if (!id2)
      id2 = id1;

    std::pair<int, int> r2;
    r2.first = ToInt(match_results[5].str());
    if (match_results[6].matched)
      r2.second = ToInt(match_results[6].str());

    auto& relation = relations[id1];
    relation.AddRange(id2, r1, r2);
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
      for (const auto& rule : root["rules"]) {
        ParseRule(StrToWstr(rule.asString()));
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
  if (taiga::GetCurrentServiceId() != sync::kMyAnimeList)  // TEMP
    return false;

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