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
#include "base/url.h"
#include "base/xml.h"
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
  ConnectionManager.CancelRequest(client_uuid_);
}

bool UpdateHelper::Check() {
  HttpRequest http_request;
  http_request.url.host = L"taiga.erengy.com";
  http_request.url.path = L"/update.php";
  http_request.url.query[L"channel"] = L"beta";
  http_request.url.query[L"check"] = ui::DlgMain.IsWindow() ? L"manual" : L"auto";
  http_request.url.query[L"version"] = std::wstring(Taiga.version);
  http_request.url.query[L"service"] = GetCurrentService()->canonical_name();
  http_request.url.query[L"username"] = GetCurrentUsername();

  client_uuid_ = http_request.uuid;

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTaigaUpdateCheck);
  return true;
}

bool UpdateHelper::ParseData(std::wstring data) {
  items.clear();
  download_path_.clear();
  latest_guid_.clear();
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
  }

  auto current_version = Taiga.version;
  auto latest_version = current_version;
  foreach_(item, items) {
    base::SemanticVersion item_version(item->guid);
    if (item_version > latest_version) {
      latest_guid_ = item->guid;
      latest_version = item_version;
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
  auto feed_item = FindItem(latest_guid_);
  if (!feed_item)
    return false;

  download_path_ = AddTrailingSlash(GetPathOnly(Taiga.GetModulePath()));
  download_path_ += GetFileName(feed_item->link);

  HttpRequest http_request;
  http_request.url = feed_item->link;

  client_uuid_ = http_request.uuid;

  auto& client = ConnectionManager.GetNewClient(http_request.uuid);
  client.set_download_path(download_path_);
  ConnectionManager.MakeRequest(client, http_request,
                                taiga::kHttpTaigaUpdateDownload);
  return true;
}

bool UpdateHelper::RunInstaller() {
  auto feed_item = FindItem(latest_guid_);
  if (!feed_item)
    return false;

  // /S runs the installer silently, /D overrides the default installation
  // directory. Do not rely on the current directory here, as it isn't
  // guaranteed to be the same as the module path.
  std::wstring parameters = L"/S /D=" + GetPathOnly(Taiga.GetModulePath());

  restart_required_ = Execute(download_path_, parameters);

  return restart_required_;
}

void UpdateHelper::SetDownloadPath(const std::wstring& path) {
  download_path_ = path;
}

const GenericFeedItem* UpdateHelper::FindItem(const std::wstring& guid) const {
  foreach_(item, items)
    if (IsEqual(item->guid, latest_guid_))
      return &(*item);

  return nullptr;
}

}  // namespace taiga