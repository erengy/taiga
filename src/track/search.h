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

#include <string>

#include "base/file.h"
#include "library/anime_episode.h"

class TaigaFileSearchHelper : public FileSearchHelper {
public:
  TaigaFileSearchHelper();
  ~TaigaFileSearchHelper() {}

  bool OnDirectory(const std::wstring& root, const std::wstring& name, const WIN32_FIND_DATA& data);
  bool OnFile(const std::wstring& root, const std::wstring& name, const WIN32_FIND_DATA& data);

  const std::wstring& path_found() const;

  void set_anime_id(int anime_id);
  void set_episode_number(int episode_number);
  void set_path_found(const std::wstring& path_found);

private:
  int anime_id_;
  anime::Episode episode_;
  int episode_number_;
  std::wstring path_found_;
};

extern TaigaFileSearchHelper file_search_helper;

void ScanAvailableEpisodes(bool silent);
void ScanAvailableEpisodes(bool silent, int anime_id, int episode_number);
void ScanAvailableEpisodesQuick();
void ScanAvailableEpisodesQuick(int anime_id);
