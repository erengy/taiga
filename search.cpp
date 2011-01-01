/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

#include "std.h"
#include "animelist.h"
#include "recognition.h"
#include "string.h"

// =============================================================================

wstring SearchFileFolder(int anime_index, wstring root, int episode_number, bool search_folder) {
  if (root.empty()) return L"";
  CheckSlash(root);
  WIN32_FIND_DATA WFD;
  wstring folder = root + L"*.*";
  HANDLE hFind = FindFirstFile(folder.c_str(), &WFD);
  if (hFind == INVALID_HANDLE_VALUE) return L"";
  CEpisode episode;

  do {
    // Folders
    if (WFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (!IsEqual(WFD.cFileName, L".") && !IsEqual(WFD.cFileName, L"..")) {
        // Check root folder
        if (search_folder == true) {
          if (Meow.ExamineTitle(WFD.cFileName, episode, false, false, false, false, false)) {
            if (Meow.CompareTitle(episode.Title, anime_index)) {
              FindClose(hFind);
              return root + WFD.cFileName + L"\\";
            }
          }
        }
        // Check sub folders
        folder = root + WFD.cFileName + L"\\";
        folder = SearchFileFolder(anime_index, folder, episode_number, search_folder);
        if (!folder.empty()) {
          FindClose(hFind);
          return folder;
        }
      }

    // Files
    } else {
      if (search_folder == false) {
        if (Meow.ExamineTitle(WFD.cFileName, episode, true, true, true, true, true)) {
          if (Meow.CompareTitle(episode.Title, anime_index)) {
            if (episode_number == 0 || episode_number == ToINT(episode.Number)) {
              FindClose(hFind);
              return root + WFD.cFileName;
            }
          }
        }
      }
    }
  } while (FindNextFile(hFind, &WFD));
  
  FindClose(hFind);
  return L"";
}