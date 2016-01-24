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

#include <cmath>

#include "base/file.h"
#include "base/foreach.h"
#include "base/string.h"
#include "base/time.h"
#include "base/url.h"
#include "base/xml.h"
#include "library/discover.h"
#include "sync/service.h"
#include "taiga/http.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "taiga/update.h"
#include "ui/dlg/dlg_main.h"
#include "ui/ui.h"

namespace taiga {

UpdateHelper::UpdateHelper()
    : restart_required_(false),
      update_available_(false) {
}

void UpdateHelper::Cancel() {
  ConnectionManager.CancelRequest(client_uid_);
}

void UpdateHelper::Check() {
  bool is_automatic = !ui::DlgMain.IsWindow();
  bool is_stable = Taiga.version.prerelease_identifiers.empty();

  HttpRequest http_request;
  http_request.url.host = L"taiga.moe";
  http_request.url.path = L"/update.php";
  http_request.url.query[L"channel"] = is_stable ? L"stable" : L"beta";
  http_request.url.query[L"check"] = is_automatic ? L"auto" : L"manual";
  http_request.url.query[L"version"] = std::wstring(Taiga.version);
  http_request.url.query[L"service"] = GetCurrentService()->canonical_name();
  http_request.url.query[L"username"] = GetCurrentUsername();

  client_uid_ = http_request.uid;

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTaigaUpdateCheck);
}

void UpdateHelper::CheckAnimeRelations() {
  if (!current_item_)
    return;

  if (current_item_->taiga_anime_relations_location.empty())
    return;

  Date local_modified(Settings[kRecognition_RelationsLastModified]);
  if (local_modified) {
    Date current_modified(current_item_->taiga_anime_relations_modified);
    if (!current_modified || current_modified <= local_modified)
      return;
  }

  HttpRequest http_request;
  http_request.url = current_item_->taiga_anime_relations_location;

  ConnectionManager.MakeRequest(http_request,
                                taiga::kHttpTaigaUpdateRelations);
}

bool UpdateHelper::ParseData(std::wstring data) {
  items.clear();
  download_path_.clear();
  current_item_.reset();
  latest_item_.reset();
  restart_required_ = false;
  update_available_ = false;

  xml_document document;
  xml_parse_result parse_result = document.load(data.c_str());

  if (parse_result.status != pugi::status_ok)
    return false;

  xml_node channel = document.child(L"rss").child(L"channel");
  foreach_xmlnode_(item, channel, L"item") {
    items.resize(items.size() + 1);
    items.back().guid = XmlReadStrValue(item, L"guid");
    items.back().category = XmlReadStrValue(item, L"category");
    items.back().link = XmlReadStrValue(item, L"link");
    items.back().description = XmlReadStrValue(item, L"description");
    items.back().pub_date = XmlReadStrValue(item, L"pubDate");
    items.back().taiga_anime_relations_location = XmlReadStrValue(item, L"taiga:animeRelationsLocation");
    items.back().taiga_anime_relations_modified = XmlReadStrValue(item, L"taiga:animeRelationsModified");
    items.back().taiga_anime_season_location = XmlReadStrValue(item, L"taiga:animeSeasonLocation");
    items.back().taiga_anime_season_max = XmlReadStrValue(item, L"taiga:animeSeasonMax");
  }

  auto current_version = Taiga.version;
  auto latest_version = current_version;
  foreach_(item, items) {
    base::SemanticVersion item_version(item->guid);
    if (item_version > latest_version) {
      latest_item_.reset(new Item(*item));
      latest_version = item_version;
    } else if (item_version == current_version) {
      current_item_.reset(new Item(*item));
      anime::Season season_max(item->taiga_anime_season_max);
      if (season_max && season_max > SeasonDatabase.available_seasons.second) {
        SeasonDatabase.available_seasons.second = season_max;
        Settings.Set(taiga::kApp_Seasons_MaxSeason, season_max.GetString());
      }
      if (!item->taiga_anime_season_location.empty())
        SeasonDatabase.remote_location = item->taiga_anime_season_location;
    }
  }

  if (latest_version > current_version)
    update_available_ = true;

  return true;
}

bool UpdateHelper::IsRestartRequired() const {
  return restart_required_;
}

bool UpdateHelper::IsUpdateAvailable() const {
  return update_available_;
}

bool UpdateHelper::IsDownloadAllowed() const {
  if (IsUpdateAvailable()) {
    ui::OnUpdateAvailable();
    return true;
  } else {
    ui::OnUpdateNotAvailable();
    return false;
  }
}

bool UpdateHelper::Download() {
  if (!latest_item_)
    return false;

  download_path_ = AddTrailingSlash(GetPathOnly(Taiga.GetModulePath()));
  download_path_ += GetFileName(latest_item_->link);

  HttpRequest http_request;
  http_request.url = latest_item_->link;

  client_uid_ = http_request.uid;

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTaigaUpdateDownload);

  return true;
}

bool UpdateHelper::RunInstaller() {
  if (!latest_item_)
    return false;

  // /S runs the installer silently, /D overrides the default installation
  // directory. Do not rely on the current directory here, as it isn't
  // guaranteed to be the same as the module path.
  std::wstring parameters = L"/S /D=" + GetPathOnly(Taiga.GetModulePath());

  restart_required_ = Execute(download_path_, parameters);

  return restart_required_;
}

std::wstring UpdateHelper::GetDownloadPath() const {
  return download_path_;
}

void UpdateHelper::SetDownloadPath(const std::wstring& path) {
  download_path_ = path;
}

}  // namespace taiga