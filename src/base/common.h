/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#ifndef COMMON_H
#define COMMON_H

#include "std.h"

namespace anime {
class Item;
}

wstring CalculateCRC(const wstring& file);

void ExecuteAction(wstring action, WPARAM wParam = 0, LPARAM lParam = 0);

wstring FormatError(DWORD dwError, LPCWSTR lpSource = NULL);

int GetEpisodeHigh(const wstring& episode_number);
int GetEpisodeLow(const wstring& episode_number);
void SplitEpisodeNumbers(const wstring& input, vector<int>& output);
wstring JoinEpisodeNumbers(const vector<int>& input);
int TranslateResolution(const wstring& str, bool return_validity = false);

void SetSharedCursor(LPCWSTR name);

int StatusToIcon(int status);

void ScanAvailableEpisodes(int anime_id, bool check_folder, bool silent);
wstring SearchFileFolder(anime::Item& anime_item, const wstring& root, int episode_number, bool search_folder);

#endif // COMMON_H