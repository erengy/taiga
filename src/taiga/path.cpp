/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include "taiga/path.h"

#include "base/file.h"
#include "base/format.h"
#include "base/string.h"
#include "sync/service.h"
#include "taiga/app.h"
#include "taiga/config.h"
#include "taiga/settings.h"

namespace taiga {

std::wstring GetDataPath() {
  std::wstring path;

#ifdef TAIGA_PORTABLE
  // Return current path in portable mode
  path = GetPathOnly(app.GetModulePath());
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
  return GetCurrentUsername() + L"@" + sync::GetServiceSlugById(service_id);
}

std::wstring GetUserDirectoryName() {
  return GetUserDirectoryName(sync::GetCurrentServiceId());
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
      return data_path + L"theme\\{}\\theme.xml"_format(settings.GetAppInterfaceTheme());
    case Path::User:
      return data_path + L"user\\";
    case Path::UserHistory:
      return data_path + L"user\\{}\\history.xml"_format(GetUserDirectoryName());
    case Path::UserLibrary:
      return data_path + L"user\\{}\\anime.xml"_format(GetUserDirectoryName());
  }
}

}  // namespace taiga
