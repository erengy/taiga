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
#include "common.h"
#include "recognition.h"
#include "string.h"

// =============================================================================

wstring SearchFileFolder(Anime& anime, wstring root, int episode_number, bool search_folder) {
  if (root.empty()) return L"";
  CheckSlash(root);
  WIN32_FIND_DATA wfd;
  wstring folder = root + L"*.*";
  HANDLE hFind = FindFirstFile(folder.c_str(), &wfd);
  if (hFind == INVALID_HANDLE_VALUE) return L"";
  Episode episode;

  do {
    // Folders
    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (wcscmp(wfd.cFileName, L".") != 0 && wcscmp(wfd.cFileName, L"..") != 0) {
        // Check root folder
        if (search_folder == true) {
          if (Meow.ExamineTitle(wfd.cFileName, episode, false, false, false, false, false)) {
            if (Meow.CompareEpisode(episode, anime, true, false, false)) {
              FindClose(hFind);
              return root + wfd.cFileName + L"\\";
            }
          }
        }
        // Check sub folders
        folder = root + wfd.cFileName + L"\\";
        folder = SearchFileFolder(anime, folder, episode_number, search_folder);
        if (!folder.empty()) {
          FindClose(hFind);
          return folder;
        }
      }

    // Files
    } else {
      if (search_folder == false) {
        // Check file size - anything less than 10 MB can't be a new episode
        if (wfd.nFileSizeLow > 1024 * 1024 * 10) {
          // Examine file name and extract episode data
          if (Meow.ExamineTitle(wfd.cFileName, episode, true, true, true, true, true)) {
            // Compare episode data with anime title
            if (Meow.CompareEpisode(episode, anime)) {
              int number = GetEpisodeHigh(episode.number);
              int numberlow = GetEpisodeLow(episode.number);
              for (int i = numberlow; i <= number; i++) {
                anime.SetEpisodeAvailability(i, true, root + wfd.cFileName);
              }
              if (episode_number == 0 || (episode_number >= numberlow && episode_number <= number)) {
                FindClose(hFind);
                return root + wfd.cFileName;
              }
            }
          }
        }
      }
    }
  } while (FindNextFile(hFind, &wfd));
  
  FindClose(hFind);
  return L"";
}