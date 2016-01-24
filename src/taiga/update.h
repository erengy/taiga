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

#ifndef TAIGA_TAIGA_UPDATE_H
#define TAIGA_TAIGA_UPDATE_H

#include <memory>
#include <string>
#include <vector>

#include "track/feed.h"

namespace taiga {

class UpdateHelper {
public:
  UpdateHelper();
  virtual ~UpdateHelper() {}

  void Cancel();
  void Check();
  void CheckAnimeRelations();
  bool Download();
  bool IsDownloadAllowed() const;
  bool IsRestartRequired() const;
  bool IsUpdateAvailable() const;
  bool ParseData(std::wstring data);
  bool RunInstaller();

  std::wstring GetDownloadPath() const;
  void SetDownloadPath(const std::wstring& path);

  class Item : public GenericFeedItem {
  public:
    std::wstring taiga_anime_relations_location;
    std::wstring taiga_anime_relations_modified;
    std::wstring taiga_anime_season_location;
    std::wstring taiga_anime_season_max;
  };
  std::vector<Item> items;

private:
  std::wstring download_path_;
  std::unique_ptr<Item> current_item_;
  std::unique_ptr<Item> latest_item_;
  bool restart_required_;
  bool update_available_;
  std::wstring client_uid_;
};

}  // namespace taiga

#endif  // TAIGA_TAIGA_UPDATE_H
