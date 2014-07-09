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

#include <string>
#include <vector>

#include "track/feed.h"

namespace taiga {

class UpdateHelper {
public:
  UpdateHelper();
  virtual ~UpdateHelper() {}

  void Cancel();
  bool Check();
  bool Download();
  bool IsDownloadAllowed() const;
  bool IsRestartRequired() const;
  bool IsUpdateAvailable() const;
  bool ParseData(std::wstring data);
  bool RunInstaller();

  std::wstring GetDownloadPath() const;
  void SetDownloadPath(const std::wstring& path);

  std::vector<GenericFeedItem> items;

private:
  const GenericFeedItem* FindItem(const std::wstring& guid) const;

  std::wstring download_path_;
  std::wstring latest_guid_;
  bool restart_required_;
  bool update_available_;
  std::wstring client_uid_;
};

}  // namespace taiga

#endif  // TAIGA_TAIGA_UPDATE_H
