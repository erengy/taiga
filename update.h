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

#ifndef UPDATE_H
#define UPDATE_H

#include "std.h"
#include "feed.h"
#include "http.h"

// =============================================================================

class UpdateHelper {
public:
  UpdateHelper();
  virtual ~UpdateHelper() {}

  bool Check(win32::App& app);
  bool Download();
  bool IsDownloadAllowed() const;
  bool IsRestartRequired() const;
  bool IsUpdateAvailable() const;
  bool ParseData(wstring data);
  bool RunInstaller();

  HttpClient client;

private:
  const GenericFeedItem* FindItem(const wstring& guid) const;
  unsigned long GetVersionValue(int major, int minor, int revision) const;

  win32::App* app_;
  vector<GenericFeedItem> items_;
  wstring latest_guid_;
  bool restart_required_;
  bool update_available_;
};

#endif // UPDATE_H