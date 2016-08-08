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

#include <map>

#include "base/string.h"
#include "sync/manager.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"

namespace taiga {

std::wstring GetDataPath() {
#ifdef TAIGA_PORTABLE
  // Return current path in portable mode
  return AddTrailingSlash(GetPathOnly(Taiga.GetModulePath())) + L"data\\";
#else
  // Return %AppData% folder
  WCHAR buffer[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE,
                                nullptr, SHGFP_TYPE_CURRENT, buffer)))
    return AddTrailingSlash(buffer) + TAIGA_APP_NAME + L"\\";
#endif
}

std::wstring GetUserDirectoryName() {
  std::wstring username = GetCurrentUsername();
  auto service = GetCurrentService();
  return username + L"@" + service->canonical_name();
}

std::wstring GetPath(PathType type) {
  static const std::wstring data_path = GetDataPath();

  switch (type) {
    default:
    case kPathData:
      return data_path;
    case kPathDatabase:
      return data_path + L"db\\";
    case kPathDatabaseAnime:
      return data_path + L"db\\anime.xml";
    case kPathDatabaseAnimeRelations:
      return data_path + L"db\\anime-relations.txt";
    case kPathDatabaseImage:
      return data_path + L"db\\image\\";
    case kPathDatabaseSeason:
      return data_path + L"db\\season\\";
    case kPathFeed:
      return data_path + L"feed\\";
    case kPathFeedHistory:
      return data_path + L"feed\\history.xml";
    case kPathMedia:
      return data_path + L"media.xml";
    case kPathSettings:
      return data_path + L"settings.xml";
    case kPathTest:
      return data_path + L"test\\";
    case kPathTestRecognition:
      return data_path + L"test\\recognition.xml";
    case kPathTheme:
      return data_path + L"theme\\";
    case kPathThemeCurrent:
      return data_path + L"theme\\" + Settings[kApp_Interface_Theme] + L"\\theme.xml";
    case kPathUser:
      return data_path + L"user\\";
    case kPathUserHistory:
      return data_path + L"user\\" + GetUserDirectoryName() + L"\\history.xml";
    case kPathUserLibrary:
      return data_path + L"user\\" + GetUserDirectoryName() + L"\\anime.xml";
  }
}

}  // namespace taiga