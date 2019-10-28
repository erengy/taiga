/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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
#include "base/string.h"
#include "base/time.h"
#include "base/url.h"
#include "base/xml.h"
#include "media/anime_season_db.h"
#include "sync/service.h"
#include "taiga/http.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "taiga/version.h"
#include "taiga/update.h"
#include "ui/dlg/dlg_main.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace taiga::detail {

void UpdateHelper::Cancel() {
  ConnectionManager.CancelRequest(client_uid_);
}

void UpdateHelper::Check() {
  const std::wstring channel = taiga::version().prerelease.empty() ?
      L"stable" : StrToWstr(taiga::version().prerelease);
  const std::wstring method = ui::DlgMain.IsWindow() ? L"manual" : L"auto";

  HttpRequest http_request;
  http_request.url.protocol = base::http::Protocol::Https;
  http_request.url.host = L"taiga.moe";
  http_request.url.path = L"/update.php";
  http_request.url.query[L"channel"] = channel;
  http_request.url.query[L"check"] = method;
  http_request.url.query[L"version"] = StrToWstr(taiga::version().to_string());
  http_request.url.query[L"service"] = GetCurrentService()->canonical_name();
  http_request.url.query[L"username"] = GetCurrentUsername();

  client_uid_ = http_request.uid;

  ConnectionManager.MakeRequest(http_request, taiga::kHttpTaigaUpdateCheck);
}

void UpdateHelper::CheckAnimeRelations() {
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

  XmlDocument document;
  const auto parse_result = document.load_string(data.c_str());

  if (!parse_result)
    return false;

  auto channel = document.child(L"rss").child(L"channel");
  for (auto item : channel.children(L"item")) {
    items.resize(items.size() + 1);
    items.back().guid.value = XmlReadStr(item, L"guid");
    items.back().category.value = XmlReadStr(item, L"category");
    items.back().link = XmlReadStr(item, L"link");
    items.back().description = XmlReadStr(item, L"description");
    items.back().pub_date = XmlReadStr(item, L"pubDate");
    items.back().taiga_anime_relations_location = XmlReadStr(item, L"taiga:animeRelationsLocation");
    items.back().taiga_anime_relations_modified = XmlReadStr(item, L"taiga:animeRelationsModified");
  }

  auto current_version = taiga::version();
  auto latest_version = current_version;
  for (const auto& item : items) {
    semaver::Version item_version(WstrToStr(item.guid.value));
    if (item_version > latest_version) {
      latest_item_.reset(new Item(item));
      latest_version = item_version;
    } else if (item_version == current_version) {
      current_item_.reset(new Item(item));
    }
  }

  if (latest_version > current_version)
    update_available_ = true;

  return true;
}

bool UpdateHelper::IsAnimeRelationsAvailable() const {
  if (!current_item_)
    return false;

  if (current_item_->taiga_anime_relations_location.empty())
    return false;

  Date local_modified(settings.GetRecognitionRelationsLastModified());
  if (local_modified) {
    Date current_modified(current_item_->taiga_anime_relations_modified);
    if (!current_modified || current_modified <= local_modified)
      return false;
  }

  return true;
}

bool UpdateHelper::IsRestartRequired() const {
  return restart_required_;
}

bool UpdateHelper::IsUpdateAvailable() const {
  return update_available_;
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

std::wstring UpdateHelper::GetCurrentAnimeRelationsModified() const {
  return current_item_ ? current_item_->taiga_anime_relations_modified :
                         std::wstring();
}

std::wstring UpdateHelper::GetDownloadPath() const {
  return download_path_;
}

void UpdateHelper::SetDownloadPath(const std::wstring& path) {
  download_path_ = path;
}

}  // namespace taiga::detail
