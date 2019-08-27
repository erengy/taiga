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

#include <algorithm>
#include <regex>

#include "base/file.h"
#include "base/format.h"
#include "base/html.h"
#include "base/log.h"
#include "base/string.h"
#include "base/time.h"
#include "base/xml.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "taiga/http.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "track/feed.h"
#include "track/recognition.h"
#include "ui/dialog.h"
#include "ui/ui.h"

class Aggregator Aggregator;

Feed& Aggregator::GetFeed() {
  return feed_;
}

bool Aggregator::CheckFeed(const std::wstring& source, bool automatic) {
  if (source.empty())
    return false;

  Feed& feed = GetFeed();

  feed.link = source;

  HttpRequest http_request;
  http_request.url = feed.link;
  http_request.parameter = reinterpret_cast<LPARAM>(&feed);
  http_request.header[L"Accept"] = L"application/rss+xml, */*";
  http_request.header[L"Accept-Encoding"] = L"gzip";

  if (!automatic) {
    ui::ChangeStatusText(
      L"Checking new torrents via {}..."_format(http_request.url.host));
  }
  ui::EnableDialogInput(ui::Dialog::Torrents, false);

  auto client_mode = automatic ?
      taiga::kHttpFeedCheckAuto : taiga::kHttpFeedCheck;

  ConnectionManager.MakeRequest(http_request, client_mode);

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
      auto anime_item = AnimeDatabase.FindItem(episode_data.anime_id);
      if (anime_item) {
        int episode_number = anime::GetEpisodeHigh(episode_data);
        anime_item->SetLastAiredEpisodeNumber(episode_number);
      }
    }

    // Categorize
    feed_item.torrent_category = GetTorrentCategory(feed_item);
  }

  filter_manager.MarkNewEpisodes(feed);
  // Preferences have lower priority, so we need to handle other filters
  // first in order to avoid discarding items that we actually want.
  filter_manager.Filter(feed, false);
  filter_manager.Filter(feed, true);
  // Archived items must be discarded after other filters are processed.
  filter_manager.FilterArchived(feed);

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
          auto sort_by = Settings[taiga::kTorrent_Download_SortBy];
          auto sort_order = Settings[taiga::kTorrent_Download_SortOrder];
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
    ui::ChangeStatusText(L"Downloading \"" + feed_item->title + L"\"...");
    ui::EnableDialogInput(ui::Dialog::Torrents, false);

    HttpRequest http_request;
    http_request.header[L"Accept"] = L"application/x-bittorrent, */*";
    http_request.url = feed_item->link;
    http_request.parameter = reinterpret_cast<LPARAM>(&feed);

    ConnectionManager.MakeRequest(http_request, taiga::kHttpFeedDownload);
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
    switch (Settings.GetInt(taiga::kTorrent_Discovery_NewAction)) {
      case 1:  // Notify
        ui::OnFeedNotify(feed);
        break;
      case 2:  // Download
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
    file = feed_item->title;
    ValidateFileName(file);
    file = feed.GetDataPath() + file + L".torrent";

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
  AddToArchive(feed_item->title);
  SaveArchive();
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
  switch (Settings.GetInt(taiga::kTorrent_Download_AppMode)) {
    case 1:  // Default application
      return GetDefaultAppPath(L".torrent");
    case 2:  // Custom application
      return Settings[taiga::kTorrent_Download_AppPath];
    default:
      return {};
  }
}

std::wstring GetTorrentDownloadPath(const FeedItem::EpisodeData& episode_data) {
  std::wstring path;

  // Use anime folder as the download folder
  const auto anime_item = AnimeDatabase.FindItem(episode_data.anime_id);
  if (anime_item) {
    const auto anime_folder = anime_item->GetFolder();
    if (!anime_folder.empty() && FolderExists(anime_folder))
      path = anime_folder;
  }

  // If no anime folder is set, use an alternative folder
  if (path.empty() &&
      Settings.GetBool(taiga::kTorrent_Download_FallbackOnFolder)) {
    path = Settings[taiga::kTorrent_Download_Location];

    // Create a subfolder using the anime title as its name
    if (!path.empty() &&
        Settings.GetBool(taiga::kTorrent_Download_CreateSubfolder)) {
      auto subfolder =
          anime_item ? anime_item->GetTitle() : episode_data.anime_title();
      ValidateFileName(subfolder);
      AddTrailingSlash(path);
      path += subfolder;
      if (CreateFolder(path) && anime_item) {
        anime_item->SetFolder(path);
        Settings.Save();
      }
    }
  }

  RemoveTrailingSlash(path);  // gets mixed up as an escape character

  return path;
}

void Aggregator::HandleFeedDownloadOpen(FeedItem& feed_item,
                                        const std::wstring& file) {
  if (!Settings.GetBool(taiga::kTorrent_Download_AppOpen))
    return;

  const auto app_path = GetTorrentApplicationPath();
  
  if (app_path.empty()) {
    LOGD(L"BitTorrent client path is empty.");
    Execute(file);
    return;
  }

  std::wstring parameters = LR"("{}")"_format(file);
  int show_command = SW_SHOWNORMAL;

  if (Settings.GetBool(taiga::kTorrent_Download_UseAnimeFolder)) {
    const auto download_path = GetTorrentDownloadPath(feed_item.episode_data);
    if (!download_path.empty()) {
      const auto app_filename = GetFileName(app_path);
      // uTorrent
      if (InStr(app_filename, L"utorrent", 0, true) > -1) {
        parameters = LR"(/directory "{}" "{}")"_format(download_path, file);
      // Deluge
      } else if (InStr(app_filename, L"deluge-console", 0, true) > -1) {
        parameters = LR"(add -p \"{}\" \"{}\")"_format(download_path, file);
        show_command = SW_HIDE;
      // Transmission
      } else if (InStr(app_filename, L"transmission-remote", 0, true) > -1) {
        parameters = LR"(-a "{}" -w "{}")"_format(file, download_path);
        show_command = SW_HIDE;
      } else if (InStr(app_filename, L"qbittorrent", 0, true) > -1) {
        parameters = LR"(--save-path="{}" --skip-dialog=true "{}")"_format(download_path, file);
      } else {
        LOGD(L"Unknown BitTorrent client: {}", app_path);
      }
    }
  }

  LOGD(L"Application: {}\nParameters: {}", app_path, parameters);

  Execute(app_path, parameters, show_command);
}

bool Aggregator::IsMagnetLink(const FeedItem& feed_item) const {
  if (Settings.GetBool(taiga::kTorrent_Download_UseMagnet) &&
      !feed_item.magnet_link.empty())
    return true;

  if (StartsWith(feed_item.link, L"magnet"))
    return true;

  return false;
}

bool Aggregator::ValidateFeedDownload(const HttpRequest& http_request,
                                      HttpResponse& http_response) {
  // Check response code
  if (http_response.code >= 400) {
    if (http_response.code == 404) {
      const auto location = http_request.url.Build();
      ui::OnFeedDownloadError(L"File not found at " + location);
    } else {
      const auto code = ToWstr(http_response.code);
      ui::OnFeedDownloadError(L"Invalid HTTP response (" + code + L")");
    }
    return false;
  }

  // Check response body
  if (StartsWith(http_response.body, L"<!DOCTYPE html>")) {
    const auto location = http_request.url.Build();
    ui::OnFeedDownloadError(L"Invalid torrent file: " + location);
    return false;
  }

  auto verify_content_type = [&]() {
    static const std::vector<std::wstring> allowed_types{
      L"application/x-bittorrent",
      // The following MIME types are invalid for .torrent files, but we allow
      // them to handle misconfigured servers.
      L"application/force-download",
      L"application/octet-stream",
      L"application/torrent",
      L"application/x-torrent",
    };
    auto it = http_response.header.find(L"Content-Type");
    if (it == http_response.header.end())
      return true;  // We can't check the header if it doesn't exist
    const auto& content_type = it->second;
    return std::find(allowed_types.begin(), allowed_types.end(),
                     ToLower_Copy(content_type)) != allowed_types.end();
  };

  auto has_content_disposition = [&]() {
    return http_response.header.count(L"Content-Disposition") > 0;
  };

  if (!verify_content_type()) {
    // Allow invalid MIME types when Content-Disposition field is present
    if (!has_content_disposition()) {
      ui::OnFeedDownloadError(L"Invalid content type: " +
                              http_response.header[L"Content-Type"]);
      return false;
    }
  }

  return true;
}

void Aggregator::FindFeedSource(Feed& feed) const {
  static const std::map<std::wstring, FeedSource> sources{
    {L"anidex", FeedSource::AniDex},
    {L"animebytes", FeedSource::AnimeBytes},
    {L"minglong", FeedSource::Minglong},
    {L"nyaa.pantsu", FeedSource::NyaaPantsu},
    {L"nyaa.si", FeedSource::NyaaSi},
    {L"tokyotosho", FeedSource::TokyoToshokan},
  };

  const Url url(feed.link);

  for (const auto& pair : sources) {
    if (InStr(url.host, pair.first, 0, true) > -1) {
      feed.source = pair.second;
      return;
    }
  }
}

void Aggregator::ParseFeedItem(FeedSource source, FeedItem& feed_item) {
  auto parse_magnet_link = [&feed_item]() {
    feed_item.magnet_link = InStr(feed_item.description,
                                  L"<a href=\"magnet:?", L"\">");
    if (!feed_item.magnet_link.empty())
      feed_item.magnet_link = L"magnet:?" + feed_item.magnet_link;
  };

  switch (source) {
    // AniDex
    case FeedSource::AniDex:
      feed_item.file_size = ParseSizeString(InStr(feed_item.description, L"Size: ", L" |"));
      if (InStr(feed_item.description, L" Batch ") > -1)
        feed_item.torrent_category = TorrentCategory::Batch;
      parse_magnet_link();
      break;

    // minglong
    case FeedSource::Minglong:
      feed_item.file_size = ParseSizeString(InStr(feed_item.description, L"Size: ", L"<"));
      break;

    // Nyaa Pantsu
    case FeedSource::NyaaPantsu:
      feed_item.info_link = feed_item.guid;
      feed_item.file_size = std::wcstoull(feed_item.enclosure_length.c_str(), nullptr, 10);
      break;

    // Nyaa.si
    case FeedSource::NyaaSi: {
      feed_item.info_link = feed_item.guid;
      if (feed_item.elements.count(L"nyaa:size"))
        feed_item.file_size = ParseSizeString(feed_item.elements[L"nyaa:size"]);
      auto get_torrent_stat = [&](const std::wstring& name, std::optional<size_t>& result) {
        if (feed_item.elements.count(name))
          result = ToInt(feed_item.elements[name]);
      };
      get_torrent_stat(L"nyaa:seeders", feed_item.seeders);
      get_torrent_stat(L"nyaa:leechers", feed_item.leechers);
      get_torrent_stat(L"nyaa:downloads", feed_item.downloads);
      break;
    }

    // Tokyo Toshokan
    case FeedSource::TokyoToshokan:
      feed_item.file_size = ParseSizeString(InStr(feed_item.description, L"Size: ", L"<"));
      parse_magnet_link();
      feed_item.description = InStr(feed_item.description, L"Comment: ", L"");
      feed_item.info_link = feed_item.guid;
      break;
  }
}

void Aggregator::CleanupDescription(std::wstring& description) {
  ReplaceString(description, L"</p>", L"\n");
  ReplaceString(description, L"<br/>", L"\n");
  ReplaceString(description, L"<br />", L"\n");
  StripHtmlTags(description);
  Trim(description, L" \n");
  while (ReplaceString(description, L"\n\n", L"\n"));
  ReplaceString(description, L"\n", L" | ");
  while (ReplaceString(description, L"  ", L" "));
}

////////////////////////////////////////////////////////////////////////////////

size_t Aggregator::GetArchiveSize() const {
  return file_archive_.size();
}

bool Aggregator::LoadArchive() {
  xml_document document;
  std::wstring path = taiga::GetPath(taiga::Path::FeedHistory);
  xml_parse_result parse_result = document.load_file(path.c_str());

  if (parse_result.status != pugi::status_ok)
    return false;

  // Read discarded
  file_archive_.clear();
  xml_node archive_node = document.child(L"archive");
  for (auto node : archive_node.children(L"item")) {
    file_archive_.push_back(node.attribute(L"title").value());
  }

  return true;
}

bool Aggregator::SaveArchive() const {
  xml_document document;
  xml_node archive_node = document.append_child(L"archive");

  size_t max_count = Settings.GetInt(taiga::kTorrent_Filter_ArchiveMaxCount);

  if (max_count > 0) {
    size_t length = file_archive_.size();
    size_t i = 0;
    if (length > max_count)
      i = length - max_count;
    for ( ; i < file_archive_.size(); i++) {
      xml_node xml_item = archive_node.append_child(L"item");
      xml_item.append_attribute(L"title") = file_archive_[i].c_str();
    }
  }

  std::wstring path = taiga::GetPath(taiga::Path::FeedHistory);
  return XmlWriteDocumentToFile(document, path);
}

void Aggregator::AddToArchive(const std::wstring& file) {
  if (!SearchArchive(file))
    file_archive_.push_back(file);
}

void Aggregator::ClearArchive() {
  file_archive_.clear();
}

bool Aggregator::SearchArchive(const std::wstring& file) const {
  auto it = std::find(file_archive_.begin(), file_archive_.end(), file);
  return it != file_archive_.end();
}
