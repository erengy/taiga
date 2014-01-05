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

#ifndef TAIGA_BASE_COMMON_H
#define TAIGA_BASE_COMMON_H

#include <string>
#include <vector>
#include <windows.h>

namespace anime {
class Item;
}

void ExecuteAction(std::wstring action, WPARAM wParam = 0, LPARAM lParam = 0);

std::wstring FormatError(DWORD dwError, LPCWSTR lpSource = NULL);

int GetEpisodeHigh(const std::wstring& episode_number);
int GetEpisodeLow(const std::wstring& episode_number);
void SplitEpisodeNumbers(const std::wstring& input, std::vector<int>& output);
std::wstring JoinEpisodeNumbers(const std::vector<int>& input);
int TranslateResolution(const std::wstring& str, bool return_validity = false);

void SetSharedCursor(LPCWSTR name);

int StatusToIcon(int status);

void ScanAvailableEpisodes(int anime_id, bool check_folder, bool silent);
std::wstring SearchFileFolder(anime::Item& anime_item, const std::wstring& root, int episode_number, bool search_folder);

#endif  // TAIGA_BASE_COMMON_H