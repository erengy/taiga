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

#include <algorithm>
#include <regex>
#include <set>

#include "track/feed_aggregator.h"

#include "base/file.h"
#include "base/format.h"
#include "base/log.h"
#include "base/string.h"
#include "base/time.h"
#include "base/xml.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "taiga/http.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "track/episode_util.h"
#include "track/feed_filter_manager.h"
#include "track/recognition.h"
#include "ui/dialog.h"
#include "ui/ui.h"

namespace track {

static bool HandleFeedError(const std::wstring& host,
                            const taiga::http::Response& response) {
  if (response.error()) {
    ui::ChangeStatusText(taiga::http::util::to_string(response.error(), host));
    ui::EnableDialogInput(ui::Dialog::Torrents, true);
    return true;
  }

  if (taiga::http::util::IsDdosProtectionEnabled(response)) {
    ui::ChangeStatusText(
        L"Cannot connect to {} because of DDoS protection (Server: {})"_format(
            host, StrToWstr(response.header("server"))));
    ui::EnableDialogInput(ui::Dialog::Torrents, true);
    return true;
  }

  return false;
}

Feed& Aggregator::GetFeed() {
  return feed_;
}

bool Aggregator::CheckFeed(const std::wstring& source, bool automatic) {
  if (source.empty())
    return false;

  auto& feed = GetFeed();

  feed.channel.link = source;

  taiga::http::Request request;
  request.set_target(WstrToStr(feed.channel.link));
  request.set_headers({
      {"Accept", "application/rss+xml, */*"},
      {"Accept-Encoding", "gzip"}});

  const auto host = StrToWstr(request.target().uri.authority->host);

  if (!automatic) {
    ui::ChangeStatusText(L"Checking new torrents via {}..."_format(host));
  }
  ui::EnableDialogInput(ui::Dialog::Torrents, false);

  const auto on_transfer = [host](const taiga::http::Transfer& transfer) {
    ui::ChangeStatusText(L"Checking new torrents via {}... ({})"_format(
        host, taiga::http::util::to_string(transfer)));
    return true;
  };

  const auto on_response = [automatic, &feed, host,
                            this](const taiga::http::Response& response) {
    if (HandleFeedError(host, response)) {
      return;
    }

    switch (response.status_class()) {
      case hypp::status::k4xx_Client_Error:
      case hypp::status::k5xx_Server_Error:
        ui::ChangeStatusText(L"{} returned an error ({} {})"_format(
            host, response.status_code(), StrToWstr(response.reason_phrase())));
        ui::EnableDialogInput(ui::Dialog::Torrents, true);
        return;
    }

    HandleFeedCheck(feed, response.body(), automatic);
  };

  taiga::http::Send(request, on_transfer, on_response);

  return true;
}

void Aggregator::ExamineData(Feed& feed) {
  for (auto& feed_item : feed.items) {
    auto title = feed_item.title;
    switch (feed.source) {
      case FeedSource::AnimeBytes: {
        // Anitomy cannot parse AnimeBytes' titles as is. To avoid writing
        // another parser, we pre-process (i.e. hack) the title instead:
        // 1. Ignore anime type and year (because we normally assume that they
        //    are only used to differentiate)
        // 2. Insert a pseudo-keyword (to make Anitomy stop there while parsing
        //    anime title)
        std::wsmatch matches;
        static const std::wregex pattern{L"(.+) - .+ \\[\\d{4}\\] :: (.+)"};
        if (std::regex_match(title, matches, pattern))
          title = matches[1].str() + L" [REMASTER] " + matches[2].str();
        break;
      }
    }

    auto& episode_data = feed_item.episode_data;

    // Examine title and compare with anime list items
    static track::recognition::ParseOptions parse_options;
    parse_options.parse_path = false;
    parse_options.streaming_media = false;
    Meow.Parse(title, parse_options, episode_data);
    static track::recognition::MatchOptions match_options;
    match_options.allow_sequels = true;
    match_options.check_airing_date = true;
    match_options.check_anime_type = true;
    match_options.check_episode_number = true;
    match_options.streaming_media = false;
    Meow.Identify(episode_data, false, match_options);

    // Update last aired episode number
    if (anime::IsValidId(episode_data.anime_id)) {
      if (auto anime_item = anime::db.Find(episode_data.anime_id)) {
        const int episode_number = anime::GetEpisodeHigh(episode_data);
        if (anime::IsValidEpisodeNumber(episode_number,
                                        anime_item->GetEpisodeCount())) {
          anime_item->SetLastAiredEpisodeNumber(episode_number);
        }
      }
    }

    // Categorize
    feed_item.torrent_category = GetTorrentCategory(feed_item);
  }

  feed_filter_manager.MarkNewEpisodes(feed);
  // Preferences have lower priority, so we need to handle other filters
  // first in order to avoid discarding items that we actually want.
  feed_filter_manager.Filter(feed, false);
  feed_filter_manager.Filter(feed, true);
  // Archived items must be discarded after other filters are processed.
  feed_filter_manager.FilterArchived(feed);

  // Sort items
  std::stable_sort(feed.items.begin(), feed.items.end());
}

bool Aggregator::Download(const FeedItem* feed_item) {
  Feed& feed = GetFeed();

  if (feed_item) {
    download_queue_.push_back(feed_item->link);
    feed_item = nullptr;
  } else if (download_queue_.empty()) {
    std::vector<const FeedItem*> selected_feed_items;
    for (const auto& item : feed.items) {
      if (item.state == FeedItemState::Selected)
        selected_feed_items.push_back(&item);
    }
    std::sort(selected_feed_items.begin(), selected_feed_items.end(),
        [&](const FeedItem* item1, const FeedItem* item2) {
          if (item1->episode_data.anime_id != item2->episode_data.anime_id)
            return item1->episode_data.anime_id < item2->episode_data.anime_id;
          auto sort_by = taiga::settings.GetTorrentDownloadSortBy();
          auto sort_order = taiga::settings.GetTorrentDownloadSortOrder();
          if (sort_by == L"episode_number") {
            if (sort_order == L"descending") {
              return item2->episode_data.episode_number() <
                     item1->episode_data.episode_number();
            } else {
              return item1->episode_data.episode_number() <
                     item2->episode_data.episode_number();
            }
          } else if (sort_by == L"release_date") {
            if (sort_order == L"descending") {
              return ConvertRfc822(item2->pub_date) <
                     ConvertRfc822(item1->pub_date);
            } else {
              return ConvertRfc822(item1->pub_date) <
                     ConvertRfc822(item2->pub_date);
            }
          } else {
            return false;
          }
        });
    for (const auto& item : selected_feed_items) {
      download_queue_.push_back(item->link);
    }
  }

  while (!feed_item && !download_queue_.empty()) {
    feed_item = FindFeedItemByLink(feed, download_queue_.front());
    if (!feed_item)
      download_queue_.erase(download_queue_.begin());
  }

  if (!feed_item)
    return false;

  if (IsMagnetLink(*feed_item)) {
    ui::ChangeStatusText(L"Opening magnet link for \"" + feed_item->title + L"\"...");
    const std::string empty_data;
    HandleFeedDownload(feed, empty_data);

  } else {
    const auto title = feed_item->title;
    ui::ChangeStatusText(L"Downloading \"{}\"..."_format(title));
    ui::EnableDialogInput(ui::Dialog::Torrents, false);

    taiga::http::Request request;
    request.set_target(WstrToStr(feed_item->link));
    request.set_header("Accept", "application/x-bittorrent, */*");

    const auto host = taiga::http::util::GetUrlHost(request.target().uri);

    const auto on_transfer = [title](const taiga::http::Transfer& transfer) {
      ui::ChangeStatusText(L"Downloading \"{}\"... ({})"_format(
          title, taiga::http::util::to_string(transfer)));
      return true;
    };

    const auto on_response = [&feed, host, this](const taiga::http::Response& response) {
      if (HandleFeedError(host, response)) {
        return;
      }
      if (ValidateFeedDownload(response)) {
        HandleFeedDownload(feed, response.body());
      } else {
        HandleFeedDownloadError(feed);
      }
    };

    taiga::http::Send(request, on_transfer, on_response);
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

FeedItem* Aggregator::FindFeedItemByLink(Feed& feed, const std::wstring& link) {
  for (auto& item : feed.items) {
    if (item.link == link)
      return &item;
  }

  return nullptr;
}

void Aggregator::HandleFeedCheck(Feed& feed, const std::string& data,
                                 bool automatic) {
  std::wstring file = feed.GetDataPath() + L"feed.xml";
  SaveToFile(data, file);

  feed.Load(StrToWstr(data));
  ExamineData(feed);
  download_queue_.clear();

  bool success = false;
  for (const auto& item : feed.items) {
    if (item.state == FeedItemState::Selected) {
      success = true;
      break;
    }
  }

  ui::OnFeedCheck(success);

  if (automatic) {
    switch (taiga::settings.GetTorrentDiscoveryNewAction()) {
      case kTorrentActionNotify:
        ui::OnFeedNotify(feed);
        break;
      case kTorrentActionDownload:
        Download(nullptr);
        break;
    }
  }
}

void Aggregator::HandleFeedDownload(Feed& feed, const std::string& data) {
  FeedItem* feed_item = nullptr;

  if (!download_queue_.empty()) {
    feed_item = FindFeedItemByLink(feed, download_queue_.front());
    download_queue_.erase(download_queue_.begin());
  }

  if (!feed_item)
    return;

  std::wstring file;

  if (!data.empty()) {
    auto path = AddTrailingSlash(taiga::settings.GetTorrentDownloadFileLocation());
    if (path.empty())
      path = feed.GetDataPath();

    file = feed_item->title;
    ValidateFileName(file);
    file = path + file + L".torrent";

    SaveToFile(data, file);

    if (!FileExists(file)) {
      ui::OnFeedDownloadError(L"Torrent file doesn't exist");
      return;
    }
  }

  const bool is_magnet_link = IsMagnetLink(*feed_item);

  if (is_magnet_link) {
    file = !feed_item->magnet_link.empty() ? feed_item->magnet_link :
                                             feed_item->link;
  }

  feed_item->state = FeedItemState::DiscardedNormal;
  archive.Add(feed_item->title);
  archive.Save();
  ui::OnFeedDownloadSuccess(is_magnet_link);

  HandleFeedDownloadOpen(*feed_item, file);

  if (!download_queue_.empty())
    Download(nullptr);
}

void Aggregator::HandleFeedDownloadError(Feed& feed) {
  if (!download_queue_.empty()) {
    download_queue_.erase(download_queue_.begin());
  }
}

std::wstring GetTorrentApplicationPath() {
  switch (taiga::settings.GetTorrentDownloadAppMode()) {
    case track::kTorrentAppDefault:
      return GetDefaultAppPath(L".torrent");
    case track::kTorrentAppCustom:
      return taiga::settings.GetTorrentDownloadAppPath();
    default:
      return {};
  }
}

std::wstring GetTorrentDownloadPath(const FeedItem::EpisodeData& episode_data) {
  std::wstring path;

  // Use anime folder as the download folder
  const auto anime_item = anime::db.Find(episode_data.anime_id);
  if (anime_item) {
    const auto anime_folder = anime_item->GetFolder();
    if (!anime_folder.empty() && FolderExists(anime_folder))
      path = anime_folder;
  }

  // If no anime folder is set, use an alternative folder
  if (path.empty() &&
      taiga::settings.GetTorrentDownloadFallbackOnFolder()) {
    path = taiga::settings.GetTorrentDownloadLocation();

    // Create a subfolder using the anime title as its name
    if (!path.empty() &&
        taiga::settings.GetTorrentDownloadCreateSubfolder()) {
      auto subfolder =
          anime_item ? anime_item->GetTitle() : episode_data.anime_title();
      ValidateFileName(subfolder);
      AddTrailingSlash(path);
      path += subfolder;
      if (CreateFolder(path) && anime_item) {
        anime_item->SetFolder(path);
        taiga::settings.Save();
      }
    }
  }

  RemoveTrailingSlash(path);  // gets mixed up as an escape character

  return path;
}

void Aggregator::HandleFeedDownloadOpen(FeedItem& feed_item,
                                        const std::wstring& file) {
  if (!taiga::settings.GetTorrentDownloadAppOpen())
    return;

  const auto app_path = GetTorrentApplicationPath();
  
  if (app_path.empty()) {
    LOGD(L"BitTorrent client path is empty.");
    Execute(file);
    return;
  }

  std::wstring parameters = LR"("{}")"_format(file);
  int show_command = SW_SHOWNORMAL;

  if (taiga::settings.GetTorrentDownloadUseAnimeFolder()) {
    const auto download_path = GetTorrentDownloadPath(feed_item.episode_data);
    if (!download_path.empty()) {
      const auto app_filename = GetFileName(app_path);

      // aria2
      if (InStr(app_filename, L"aria2c", 0, true) > -1) {
        parameters = LR"(--dir="{}" "{}")"_format(download_path, file);

      // Deluge
      } else if (InStr(app_filename, L"deluge-console", 0, true) > -1) {
        parameters = LR"(add -p \"{}\" \"{}\")"_format(download_path, file);
        show_command = SW_HIDE;

      // PicoTorrent
      } else if (InStr(app_filename, L"picotorrent", 0, true) > -1) {
        parameters = LR"(--save-path="{}" --silent "{}")"_format(download_path, file);

      // qBittorrent
      } else if (InStr(app_filename, L"qbittorrent", 0, true) > -1) {
        parameters = LR"(--save-path="{}" --skip-dialog=true "{}")"_format(download_path, file);

      // Transmission
      } else if (InStr(app_filename, L"transmission-remote", 0, true) > -1) {
        parameters = LR"(-a "{}" -w "{}")"_format(file, download_path);
        show_command = SW_HIDE;

      // uTorrent
      } else if (InStr(app_filename, L"utorrent", 0, true) > -1) {
        parameters = LR"(/directory "{}" "{}")"_format(download_path, file);

      } else {
        LOGD(L"Unknown BitTorrent client: {}", app_path);
      }
    }
  }

  LOGD(L"Application: {}\nParameters: {}", app_path, parameters);

  Execute(app_path, parameters, show_command);
}

bool Aggregator::IsMagnetLink(const FeedItem& feed_item) const {
  if (taiga::settings.GetTorrentDownloadUseMagnet() &&
      !feed_item.magnet_link.empty())
    return true;

  if (StartsWith(feed_item.link, L"magnet"))
    return true;

  return false;
}

bool Aggregator::ValidateFeedDownload(const taiga::http::Response& response) {
  // Check response code
  if (response.status_code() >= 400) {
    if (response.status_code() == 404) {
      ui::OnFeedDownloadError(
          L"File not found at " + StrToWstr(response.url()));
    } else {
      ui::OnFeedDownloadError(
          L"Invalid HTTP response ({})"_format(response.status_code()));
    }
    return false;
  }

  // Check response body
  if (StartsWith(StrToWstr(response.body()), L"<!DOCTYPE html>")) {
    ui::OnFeedDownloadError(L"Invalid torrent file: " +
                            StrToWstr(response.url()));
    return false;
  }

  const auto verify_content_type = [&]() {
    static const std::set<std::wstring> allowed_types{
      L"application/x-bittorrent",
      // The following MIME types are invalid for .torrent files, but we allow
      // them to handle misconfigured servers.
      L"application/force-download",
      L"application/octet-stream",
      L"application/torrent",
      L"application/x-torrent",
    };
    const auto content_type = response.header("content-type");
    if (content_type.empty())
      return true;  // We can't check the header if it doesn't exist

    return allowed_types.count(ToLower_Copy(StrToWstr(content_type))) > 0;
  };

  auto has_content_disposition = [&]() {
    return !response.header("content-disposition").empty();
  };

  if (!verify_content_type()) {
    // Allow invalid MIME types when Content-Disposition field is present
    if (!has_content_disposition()) {
      ui::OnFeedDownloadError(L"Invalid content type: " +
                              StrToWstr(response.header("content-type")));
      return false;
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

bool TorrentArchive::Load() {
  XmlDocument document;
  const auto path = taiga::GetPath(taiga::Path::FeedHistory);
  const auto parse_result = XmlLoadFileToDocument(document, path);

  if (!parse_result)
    return false;

  // Read discarded
  files_.clear();
  auto archive_node = document.child(L"archive");
  for (auto node : archive_node.children(L"item")) {
    files_.push_back(node.attribute(L"title").value());
  }

  return true;
}

bool TorrentArchive::Save() const {
  XmlDocument document;
  auto archive_node = document.append_child(L"archive");

  size_t max_count = taiga::settings.GetTorrentFilterArchiveMaxCount();

  if (max_count > 0) {
    size_t length = files_.size();
    size_t i = 0;
    if (length > max_count)
      i = length - max_count;
    for ( ; i < files_.size(); i++) {
      auto xml_item = archive_node.append_child(L"item");
      xml_item.append_attribute(L"title") = files_[i].c_str();
    }
  }

  const auto path = taiga::GetPath(taiga::Path::FeedHistory);
  return XmlSaveDocumentToFile(document, path);
}

bool TorrentArchive::Contains(const std::wstring& file) const {
  const auto it = std::find(files_.begin(), files_.end(), file);
  return it != files_.end();
}

size_t TorrentArchive::Size() const {
  return files_.size();
}

void TorrentArchive::Add(const std::wstring& file) {
  if (!Contains(file))
    files_.push_back(file);
}

void TorrentArchive::Clear() {
  files_.clear();
}

}  // namespace track
