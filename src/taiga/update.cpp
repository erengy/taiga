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

#include "base/std.h"
#include <cmath>

#include "update.h"

#include "base/common.h"
#include "base/foreach.h"
#include "settings.h"
#include "base/string.h"
#include "base/types.h"
#include "taiga.h"
#include "base/xml.h"

#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_update.h"

#include "win/win_taskdialog.h"

// =============================================================================

UpdateHelper::UpdateHelper()
    : app_(nullptr),
      restart_required_(false),
      update_available_(false) {
}

// =============================================================================

bool UpdateHelper::Check(win::App& app) {
  app_ = &app;

  HttpRequest http_request;
  http_request.host = L"taiga.erengy.com";
  http_request.path = L"/update.php";
  http_request.query.insert(std::make_pair(L"username", Settings.Account.MAL.user));
  http_request.query.insert(std::make_pair(L"version", APP_VERSION));
  http_request.query.insert(std::make_pair(L"check", MainDialog.IsWindow() ? L"manual" : L"auto"));

  client.set_download_path(L"");
  client.SetClientMode(HTTP_UpdateCheck);

  return client.MakeRequest(http_request);
}

bool UpdateHelper::ParseData(wstring data) {
  // Reset values
  items_.clear();
  download_path_.clear();
  latest_guid_.clear();
  restart_required_ = false;
  update_available_ = false;
  
  // Load XML data
  xml_document doc;
  xml_parse_result result = doc.load(data.c_str());
  if (result.status != status_ok) {
    return false;
  }

  // Read channel information
  xml_node channel = doc.child(L"rss").child(L"channel");

  // Read items
  for (xml_node item = channel.child(L"item"); item; item = item.next_sibling(L"item")) {
    items_.resize(items_.size() + 1);
    items_.back().guid = XML_ReadStrValue(item, L"guid");
    items_.back().category = XML_ReadStrValue(item, L"category");
    items_.back().link = XML_ReadStrValue(item, L"link");
    items_.back().description = XML_ReadStrValue(item, L"description");
    items_.back().pub_date = XML_ReadStrValue(item, L"pubDate");
  }

  // Get version information
  auto current_version = GetVersionValue(app_->GetVersionMajor(),
                                         app_->GetVersionMinor(),
                                         app_->GetVersionRevision());
  auto latest_version = current_version;
  foreach_(item, items_) {
    vector<wstring> numbers;
    Split(item->guid, L".", numbers);
    auto item_version = GetVersionValue(ToInt(numbers.at(0)),
                                        ToInt(numbers.at(1)),
                                        ToInt(numbers.at(2)));
    if (item_version > latest_version) {
      latest_guid_ = item->guid;
      latest_version = item_version;
    }
  }

  // Compare version information
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
  win::TaskDialog dlg(L"Update", TD_ICON_INFORMATION);
  dlg.SetFooter(L"Current version: " APP_VERSION);

  if (IsUpdateAvailable()) {
    auto feed_item = FindItem(latest_guid_);
    if (!feed_item) return false;

    dlg.SetMainInstruction(L"A new version of Taiga is available!");
    wstring content = L"Latest version: " + latest_guid_;
    dlg.SetContent(content.c_str());
    dlg.SetExpandedInformation(feed_item->description.c_str());
    
    dlg.AddButton(L"Download", IDYES);
    dlg.AddButton(L"Cancel", IDNO);
    dlg.Show(UpdateDialog.GetWindowHandle());
    if (dlg.GetSelectedButtonID() != IDYES)
      return false;
  
  } else {
    if (MainDialog.IsWindow()) {
      dlg.SetMainInstruction(L"No updates available. Taiga is up to date!");
      dlg.AddButton(L"OK", IDOK);
      dlg.Show(g_hMain);
    }
    return false;
  }

  return true;
}

bool UpdateHelper::Download() {
  auto feed_item = FindItem(latest_guid_);
  if (!feed_item) return false;

  // TODO: Use TEMP folder path
  download_path_ = AddTrailingSlash(GetPathOnly(app_->GetModulePath()));
  download_path_ += GetFileName(feed_item->link);

  win::http::Url url(feed_item->link);
  
  HttpRequest http_request;
  http_request.host = url.host;
  http_request.path = url.path;

  client.set_download_path(download_path_);
  client.SetClientMode(HTTP_UpdateDownload);

  return client.MakeRequest(http_request);
}

bool UpdateHelper::RunInstaller() {
  auto feed_item = FindItem(latest_guid_);
  if (!feed_item) return false;

  // /S runs the installer silently, /D overrides the default installation
  // directory. Do not rely on the current directory here, as it isn't
  // guaranteed to be the same as the module path.
  wstring parameters = L"/S /D=" + GetPathOnly(app_->GetModulePath());

  restart_required_ = Execute(download_path_, parameters);
  return restart_required_;
}

void UpdateHelper::SetDownloadPath(const wstring& path) {
  download_path_ = path;
}

const GenericFeedItem* UpdateHelper::FindItem(const wstring& guid) const {
  foreach_(item, items_)
    if (IsEqual(item->guid, latest_guid_))
      return &(*item);

  return nullptr;
}

unsigned long UpdateHelper::GetVersionValue(int major, int minor, int revision) const {
  return (major * static_cast<unsigned long>(pow(10.0, 12))) +
         (minor * static_cast<unsigned long>(pow(10.0, 8))) +
         revision;
}