/*
** Taiga
** Copyright (C) 2010-2018, Eren Okka
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
**
** Modified by Zokka
*/

#pragma once

#include <windows/win/thread.h>

namespace library {

  struct RemoveSettings {
  public:
    std::optional<int> eps_to_keep;
    std::optional<bool> remove_all_if_complete;
    std::optional<bool> prompt_remove;
    std::optional<bool> remove_permanent;
  };

  void PurgeWatchedEpisodes(int anime_id, RemoveSettings settings, bool silent);
  void PurgeWatchedEpisodes(anime::Item* item, RemoveSettings settings, bool silent);

  void DeleteEpisodes(std::vector<std::wstring> paths, bool permanent);
  bool PromptDeletion(anime::Item* item, std::vector<std::wstring> episode_paths, bool silent);
  std::vector<std::wstring> ReadEpisodePaths(anime::Item item, int episode_num_max, bool scan, bool silent);

  void ProcessPurges();
  void SchedulePurge(int anime_id, RemoveSettings);
  void SchedulePurge(int anime_id);
 
}  // namespace library
