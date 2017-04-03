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

#include <map>

#include "base/file.h"
#include "base/string.h"
#include "sync/manager.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"

namespace taiga {

std::wstring GetDataPath() {
  std::wstring path;

#ifdef TAIGA_PORTABLE
  // Return current path in portable mode
  path = GetPathOnly(Taiga.GetModulePath());
#else
  // Return %AppData% folder
  path = GetKnownFolderPath(FOLDERID_RoamingAppData);
  AddTrailingSlash(path);
  path += TAIGA_APP_NAME;
#endif

  AddTrailingSlash(path);
  return path + L"data\\";
}

std::wstring GetUserDirectoryName(const sync::ServiceId service_id) {
  return GetCurrentUsername() + L"@" +
         ServiceManager.service(service_id)->canonical_name();
}

std::wstring GetUserDirectoryName() {
  return GetUserDirectoryName(GetCurrentServiceId());
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