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

#include <algorithm>

#include "base/file.h"
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

Aggregator::Aggregator() {
  // Add torrent feed
  feeds_.resize(feeds_.size() + 1);
  feeds_.back().category = kFeedCategoryLink;
}

Feed* Aggregator::GetFeed(FeedCategory category) {
  for (auto& feed : feeds_)
    if (feed.category == category)
      return &feed;

  return nullptr;
}

bool Aggregator::CheckFeed(FeedCategory category, const std::wstring& source,
                           bool automatic) {
  if (source.empty())
    return false;

  Feed& feed = *GetFeed(category);

  feed.link = source;

  HttpRequest http_request;
  http_request.url = feed.link;
  http_request.parameter = reinterpret_cast<LPARAM>(&feed);
  http_request.header[L"Accept-Encoding"] = L"gzip";

  switch (feed.category) {
    case kFeedCategoryLink:
      if (!automatic) {
        ui::ChangeStatusText(L"Checking new torrents via " +
                             http_request.url.host + L"...");
      }
      ui::EnableDialogInput(ui::kDialogTorrents, false);
      break;
  }

  auto client_mode = automatic ?
      taiga::kHttpFeedCheckAuto : taiga::kHttpFeedCheck;

  ConnectionManager.MakeRequest(http_request, client_mode);

  return true;
}

void Aggregator::ExamineData(Feed& feed) {
  for (auto& feed_item : feed.items) {
    auto& episode_data = feed_item.episode_data;

    // Examine title and compare with anime list items
    static track::recognition::ParseOptions parse_options;
    parse_options.parse_path = false;
    parse_options.streaming_media = false;
    Meow.Parse(feed_item.title, parse_options, episode_data);
    static track::recognition::MatchOptions match_options;
    match_options.allow_sequels = true;
    match_options.check_airing_date = true;
    match_options.check_anime_type = true;
    match_options.check_episode_number = true;
    Meow.Identify(episode_data, false, match_options);

    // Update last aired episode number
    if (anime::IsValidId(episode_data.anime_id)) {
      auto anime_item = AnimeDatabase.FindItem(episode_data.anime_id);
      if (anime_item) {
        int episode_number = anime::GetEpisodeHigh(episode_data);
        anime_item->SetLastAiredEpisodeNumber(episode_number);
      }
    }
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

bool Aggregator::Download(FeedCategory category, const FeedItem* feed_item) {
  Feed& feed = *GetFeed(category);

  if (feed_item) {
    download_queue_.push_back(feed_item->link);
    feed_item = nullptr;
  } else if (download_queue_.empty()) {
    std::vector<const FeedItem*> selected_feed_items;
    for (const auto& item : feed.items) {
      if (item.state == kFeedItemSelected)
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

  if (StartsWith(feed_item->link, L"magnet")) {
    ui::ChangeStatusText(L"Opening magnet link for \"" + feed_item->title + L"\"...");
    const std::string empty_data;
    HandleFeedDownload(feed, empty_data);

  } else {
    ui::ChangeStatusText(L"Downloading \"" + feed_item->title + L"\"...");
    ui::EnableDialogInput(ui::kDialogTorrents, false);

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
    if (item.state == kFeedItemSelected) {
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
        Download(feed.category, nullptr);
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
      ui::OnFeedDownload(false, L"Torrent file doesn't exist");
      return;
    }
  }

  feed_item->state = kFeedItemDiscardedNormal;
  AddToArchive(feed_item->title);
  ui::OnFeedDownload(true, L"");

  if (!file.empty()) {
    HandleFeedDownloadOpen(*feed_item, file);
  } else {
    ExecuteLink(feed_item->link);  // magnet link
  }

  if (!download_queue_.empty())
    Download(feed.category, nullptr);
}

void Aggregator::HandleFeedDownloadOpen(FeedItem& feed_item,
                                        const std::wstring& file) {
  if (!Settings.GetBool(taiga::kTorrent_Download_AppOpen))
    return;

  std::wstring app_path;
  switch (Settings.GetInt(taiga::kTorrent_Download_AppMode)) {
    case 1:  // Default application
      app_path = GetDefaultAppPath(L".torrent", L"");
      break;
    case 2:  // Custom application
      app_path = Settings[taiga::kTorrent_Download_AppPath];
      break;
  }

  std::wstring download_path;
  if (Settings.GetBool(taiga::kTorrent_Download_UseAnimeFolder)) {
    // Use anime folder as the download folder
    auto anime_id = feed_item.episode_data.anime_id;
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    if (anime_item) {
      std::wstring anime_folder = anime_item->GetFolder();
      if (!anime_folder.empty() && FolderExists(anime_folder))
        download_path = anime_folder;
    }
    // If no anime folder is set, use an alternative folder
    if (download_path.empty()) {
      if (Settings.GetBool(taiga::kTorrent_Download_FallbackOnFolder) &&
          !Settings[taiga::kTorrent_Download_Location].empty()) {
        download_path = Settings[taiga::kTorrent_Download_Location];
      }
      if (!download_path.empty() && !FolderExists(download_path)) {
        LOG(LevelWarning, L"Folder doesn't exist.\n"
                          L"Path: " + download_path);
        download_path.clear();
      }
      // Create a subfolder using the anime title as its name
      if (!download_path.empty() &&
          Settings.GetBool(taiga::kTorrent_Download_CreateSubfolder)) {
        std::wstring anime_title;
        if (anime_item) {
          anime_title = anime_item->GetTitle();
        } else {
          anime_title = feed_item.episode_data.anime_title();
        }
        ValidateFileName(anime_title);
        TrimRight(anime_title, L".");
        AddTrailingSlash(download_path);
        download_path += anime_title;
        if (!CreateFolder(download_path)) {
          LOG(LevelError, L"Subfolder could not be created.\n"
                          L"Path: " + download_path);
          download_path.clear();
        } else {
          if (anime_item) {
            anime_item->SetFolder(download_path);
            Settings.Save();
          }
        }
      }
    }
  }

  std::wstring parameters = L"\"" + file + L"\"";
  int show_command = SW_SHOWNORMAL;

  if (!download_path.empty()) {
    // uTorrent
    if (InStr(GetFileName(app_path), L"utorrent", 0, true) > -1) {
      parameters = L"/directory \"" + download_path + L"\" " + parameters;
    // Deluge
    } else if (InStr(GetFileName(app_path), L"deluge", 0, true) > -1) {
      app_path = GetPathOnly(app_path) + L"deluge-console.exe";
      parameters = L"add -p \\\"" + download_path + L"\\\" \\\"" + file + L"\\\"";
      show_command = SW_HIDE;
    } else {
      LOG(LevelDebug, L"Application is not a supported torrent client.\n"
                      L"Path: " + app_path);
    }
  }

  Execute(app_path, parameters, show_command);
}

bool Aggregator::ValidateFeedDownload(const HttpRequest& http_request,
                                      HttpResponse& http_response) {
  if (http_response.code >= 400) {
    if (http_response.code == 404) {
      auto location = http_request.url.Build();
      ui::OnFeedDownload(false, L"File not found at " + location);
    } else {
      auto code = ToWstr(http_response.code);
      ui::OnFeedDownload(false, L"Invalid HTTP response (" + code + L")");
    }
    return false;
  }

  auto it = http_response.header.find(L"Content-Type");
  if (it != http_response.header.end()) {
    static const std::vector<std::wstring> allowed_types{
      L"application/x-bittorrent",
      // The following MIME types are invalid for .torrent files, but we allow
      // them to handle misconfigured servers.
      L"application/force-download",
      L"application/octet-stream",
      L"application/torrent",
      L"application/x-torrent",
    };
    if (std::find(allowed_types.begin(), allowed_types.end(),
                  ToLower_Copy(it->second)) == allowed_types.end()) {
      ui::OnFeedDownload(false, L"Invalid content type: " + it->second);
      return false;
    }
  }

  if (StartsWith(http_response.body, L"<!DOCTYPE html>")) {
    auto location = http_request.url.Build();
    ui::OnFeedDownload(false, L"Invalid torrent file: " + location);
    return false;
  }

  return true;
}

void Aggregator::ParseDescription(FeedItem& feed_item,
                                  const std::wstring& source) {
  // Baka-Updates
  if (InStr(source, L"baka-updates", 0, true) > -1) {
    int index_begin = InStr(feed_item.description, L"Released on");
    int index_end = feed_item.description.length();
    if (index_begin > -1)
      index_end -= index_begin;
    if (index_begin == -1)
      index_begin = 0;
    feed_item.description =
        feed_item.description.substr(index_begin, index_end);

  // Haruhichan
  } else if (InStr(source, L"haruhichan", 0, true) > -1) {
    feed_item.info_link = feed_item.description;

  // NyaaTorrents
  } else if (InStr(source, L"nyaa", 0, true) > -1) {
    std::vector<std::wstring> description_vector;
    Split(feed_item.description, L" - ", description_vector);
    if (description_vector.size() > 1) {
      feed_item.episode_data.file_size = description_vector.at(1);
      description_vector.erase(description_vector.begin() + 1);
      feed_item.description = Join(description_vector, L" - ");
    }
    feed_item.info_link = feed_item.guid;

  // TokyoTosho
  } else if (InStr(source, L"tokyotosho", 0, true) > -1) {
    std::wstring size_str = L"Size: ";
    std::wstring comment_str = L"Comment: ";
    std::vector<std::wstring> description_vector;
    Split(feed_item.description, L"\n", description_vector);
    feed_item.description.clear();
    for (const auto& it : description_vector) {
      if (StartsWith(it, size_str)) {
        feed_item.episode_data.file_size = it.substr(size_str.length());
      } else if (StartsWith(it, comment_str)) {
        feed_item.description = it.substr(comment_str.length());
      } else if (InStr(it, L"magnet:?") > -1) {
        // TODO: This doesn't work because we've used StripHtmlTags beforehand.
        feed_item.magnet_link = L"magnet:?" +
            InStr(it, L"<a href=\"magnet:?", L"\">Magnet Link</a>");
      }
    }
    feed_item.info_link = feed_item.guid;
  }
}

////////////////////////////////////////////////////////////////////////////////

bool Aggregator::LoadArchive() {
  xml_document document;
  std::wstring path = taiga::GetPath(taiga::kPathFeedHistory);
  xml_parse_result parse_result = document.load_file(path.c_str());

  if (parse_result.status != pugi::status_ok)
    return false;

  // Read discarded
  file_archive_.clear();
  xml_node archive_node = document.child(L"archive");
  foreach_xmlnode_(node, archive_node, L"item") {
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

  std::wstring path = taiga::GetPath(taiga::kPathFeedHistory);
  return XmlWriteDocumentToFile(document, path);
}

void Aggregator::AddToArchive(const std::wstring& file) {
  if (!SearchArchive(file))
    file_archive_.push_back(file);
}

bool Aggregator::SearchArchive(const std::wstring& file) const {
  auto it = std::find(file_archive_.begin(), file_archive_.end(), file);
  return it != file_archive_.end();
}