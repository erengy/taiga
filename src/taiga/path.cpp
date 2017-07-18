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

std::wstring GetPath(Path path) {
  static const std::wstring data_path = GetDataPath();

  switch (path) {
    default:
    case Path::Data:
      return data_path;
    case Path::Database:
      return data_path + L"db\\";
    case Path::DatabaseAnime:
      return data_path + L"db\\anime.xml";
    case Path::DatabaseAnimeRelations:
      return data_path + L"db\\anime-relations.txt";
    case Path::DatabaseImage:
      return data_path + L"db\\image\\";
    case Path::DatabaseSeason:
      return data_path + L"db\\season\\";
    case Path::Feed:
      return data_path + L"feed\\";
    case Path::FeedHistory:
      return data_path + L"feed\\history.xml";
    case Path::Media:
      return data_path + L"players.anisthesia";
    case Path::Settings:
      return data_path + L"settings.xml";
    case Path::Test:
      return data_path + L"test\\";
    case Path::TestRecognition:
      return data_path + L"test\\recognition.xml";
    case Path::Theme:
      return data_path + L"theme\\";
    case Path::ThemeCurrent:
      return data_path + L"theme\\" + Settings[kApp_Interface_Theme] + L"\\theme.xml";
    case Path::User:
      return data_path + L"user\\";
    case Path::UserHistory:
      return data_path + L"user\\" + GetUserDirectoryName() + L"\\history.xml";
    case Path::UserLibrary:
      return data_path + L"user\\" + GetUserDirectoryName() + L"\\anime.xml";
  }
}

}  // namespace taiga