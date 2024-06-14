/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <cmath>

#include "taiga/update.h"

#include "base/file.h"
#include "base/log.h"
#include "base/string.h"
#include "base/time.h"
#include "base/xml.h"
#include "sync/service.h"
#include "taiga/app.h"
#include "taiga/http.h"
#include "taiga/path.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "taiga/version.h"
#include "track/recognition.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_update.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace taiga::detail {

void UpdateHelper::Cancel() {
  transfer_cancelled_ = true;
}

void UpdateHelper::Check() {
  const std::string channel = taiga::version().prerelease.empty() ?
      "stable" : taiga::version().prerelease;
  const std::string method = ui::DlgMain.IsWindow() ? "manual" : "auto";

  taiga::http::Request request;
  request.set_target("https://taiga.moe/update.php");
  request.set_query({
      {"channel", channel},
      {"check", method},
      {"version", taiga::version().to_string()},
      {"service", WstrToStr(sync::GetCurrentServiceSlug())},
      {"username", WstrToStr(GetCurrentUsername())}});

  ui::DlgUpdate.progressbar.SetPosition(0);

  transfer_cancelled_ = false;

  const auto on_transfer = [this](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer);
  };

  const auto on_response = [this](const taiga::http::Response& response) {
    if (transfer_cancelled_) {
      transfer_cancelled_ = false;
      return;
    }
    if (response.error()) {
      if (ui::DlgMain.IsWindow()) {
        MessageBox(ui::DlgUpdate.GetWindowHandle(),
                   StrToWstr(response.error().str()).c_str(), L"Update Error",
                   MB_ICONERROR | MB_OK);
      }
      ui::OnUpdateFinished();
      return;
    }

    if (!ParseData(StrToWstr(response.body()))) {
      if (ui::DlgMain.IsWindow()) {
        MessageBox(ui::DlgUpdate.GetWindowHandle(),
                   L"Could not parse update data.", L"Update Error",
                   MB_ICONERROR | MB_OK);
      }
      ui::OnUpdateFinished();
      return;
    }

    if (update_available_) {
      ui::OnUpdateAvailable();
      return;
    }
    if (IsAnimeRelationsAvailable()) {
      CheckAnimeRelations();
      return;
    }
    ui::OnUpdateNotAvailable(false);
    ui::OnUpdateFinished();
  };

  taiga::http::Send(request, on_transfer, on_response);
}

void UpdateHelper::CheckAnimeRelations() {
  taiga::http::Request request;
  request.set_target(WstrToStr(current_item_->taiga_anime_relations_location));

  ui::DlgUpdate.progressbar.SetPosition(0);
  ui::DlgUpdate.SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS,
                               L"Checking latest anime relations...");

  const auto on_transfer = [this](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer);
  };

  const auto on_response = [](const taiga::http::Response& response) {
    if (response.error()) {
      ui::OnUpdateFinished();
      return;
    }

    if (Meow.ReadRelations(response.body()) &&
        SaveToFile(response.body(), GetPath(Path::DatabaseAnimeRelations))) {
      LOGD(L"Updated anime relation data.");
      ui::OnUpdateNotAvailable(true);
    } else {
      Meow.ReadRelations();
      LOGD(L"Anime relation data update failed.");
      ui::OnUpdateNotAvailable(false);
    }
    ui::OnUpdateFinished();
  };

  taiga::http::Send(request, on_transfer, on_response);
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
    items.back().taiga_anime_relations_location =
        XmlReadStr(item, L"taiga:animeRelationsLocation");
    items.back().taiga_anime_relations_modified =
        XmlReadStr(item, L"taiga:animeRelationsModified");
  }

  const auto current_version = taiga::version();
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

  const Date local_modified(settings.GetRecognitionRelationsLastModified());
  if (local_modified) {
    const Date current_modified(current_item_->taiga_anime_relations_modified);
    if (!current_modified || current_modified <= local_modified)
      return false;
  }

  return true;
}

bool UpdateHelper::IsRestartRequired() const {
  return restart_required_;
}

bool UpdateHelper::Download() {
  if (!latest_item_)
    return false;

  download_path_ = AddTrailingSlash(GetPathOnly(app.GetModulePath()));
  download_path_ += GetFileName(latest_item_->link);

  transfer_cancelled_ = false;

  taiga::http::Request request;
  request.set_target(WstrToStr(latest_item_->link));

  ui::DlgUpdate.progressbar.SetPosition(0);
  ui::DlgUpdate.SetDlgItemText(IDC_STATIC_UPDATE_PROGRESS,
                               L"Downloading latest update...");

  const auto on_transfer = [this](const taiga::http::Transfer& transfer) {
    return OnTransfer(transfer);
  };

  const auto on_response = [this](const taiga::http::Response& response) {
    if (transfer_cancelled_) {
      transfer_cancelled_ = false;
      return;
    }
    if (response.error()) {
      MessageBox(ui::DlgUpdate.GetWindowHandle(),
                  StrToWstr(response.error().str()).c_str(), L"Download Error",
                  MB_ICONERROR | MB_OK);
      ui::OnUpdateFinished();
      return;
    }

    const auto path = GetPathOnly(download_path_);
    const auto file = GetFileName(StrToWstr(response.url()));
    download_path_ = path + file;

    if (response.status_class() == 200 &&
        SaveToFile(response.body(), download_path_)) {
      RunInstaller();
    } else {
      ui::OnUpdateFailed();
    }
    ui::OnUpdateFinished();
  };

  taiga::http::Send(request, on_transfer, on_response);
  return true;
}

bool UpdateHelper::RunInstaller() {
  if (!latest_item_)
    return false;

  // Custom options:
  //
  // `/NOCLOSE` skips closing application instance.
  //
  // `/RUN` restarts the application after installation.
  //
  // NSIS options:
  //
  // `/S` runs the installer silently.
  //
  // `/D` overrides the default installation directory.
  //
  //   - It must be the last parameter.
  //   - It must not contain any quotes, even if the path contains spaces.
  //   - Only absolute paths are supported.
  //   - Do not rely on the current directory here, as it is not guaranteed to
  //     be the same as the module path.
  //
  // See: https://nsis.sourceforge.io/Docs/Chapter3.html#installerusage
  std::wstring parameters =
      L"/NOCLOSE /RUN /S /D=" + GetPathOnly(app.GetModulePath());

  restart_required_ = Execute(download_path_, parameters);

  return restart_required_;
}

std::wstring UpdateHelper::GetCurrentAnimeRelationsModified() const {
  return current_item_ ? current_item_->taiga_anime_relations_modified :
                         std::wstring();
}

bool UpdateHelper::OnTransfer(const taiga::http::Transfer& transfer) {
  if (transfer_cancelled_) {
    transfer_cancelled_ = false;
    return false;
  }

  if (!ui::DlgUpdate.progressbar.GetPosition()) {
    if (transfer.total > 0) {
      ui::DlgUpdate.progressbar.SetMarquee(false);
      ui::DlgUpdate.progressbar.SetRange(0, static_cast<UINT>(transfer.total));
    } else {
      ui::DlgUpdate.progressbar.SetMarquee(true);
    }
  }

  if (transfer.total > 0) {
    ui::DlgUpdate.progressbar.SetPosition(static_cast<UINT>(transfer.current));
  }

  return true;
}

}  // namespace taiga::detail
