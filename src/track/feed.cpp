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

#include "base/base64.h"
#include "base/file.h"
#include "base/foreach.h"
#include "base/html.h"
#include "base/log.h"
#include "base/string.h"
#include "base/url.h"
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

GenericFeedItem::GenericFeedItem()
    : permalink(true) {
}

FeedItem::FeedItem()
    : index(-1),
      state(kFeedItemBlank) {
}

void FeedItem::Discard(int option) {
  switch (option) {
    default:
    case kFeedFilterOptionDefault:
      state = kFeedItemDiscardedNormal;
      break;
    case kFeedFilterOptionDeactivate:
      state = kFeedItemDiscardedInactive;
      break;
    case kFeedFilterOptionHide:
      state = kFeedItemDiscardedHidden;
      break;
  }
}

bool FeedItem::IsDiscarded() const {
  switch (state) {
    case kFeedItemDiscardedNormal:
    case kFeedItemDiscardedInactive:
    case kFeedItemDiscardedHidden:
      return true;
    default:
      return false;
  }
}

bool FeedItem::operator<(const FeedItem& item) const {
  // Initialize priority list
  static const int state_priorities[] = {1, 2, 3, 4, 0};

  // Sort items by the priority of their state
  return state_priorities[this->state] < state_priorities[item.state];
}

////////////////////////////////////////////////////////////////////////////////

Feed::Feed()
    : category(kFeedCategoryLink),
      download_index(-1) {
}

bool Feed::Check(const std::wstring& source, bool automatic) {
  if (source.empty())
    return false;

  link = source;

  switch (category) {
    case kFeedCategoryLink:
      ui::EnableDialogInput(ui::kDialogTorrents, false);
      break;
  }

  HttpRequest http_request;
  http_request.url = link;
  http_request.parameter = reinterpret_cast<LPARAM>(this);

  auto client_mode = automatic ?
      taiga::kHttpFeedCheckAuto : taiga::kHttpFeedCheck;
  auto& client = ConnectionManager.GetNewClient(http_request.uuid);
  client.set_download_path(GetDataPath() + L"feed.xml");
  ConnectionManager.MakeRequest(client, http_request, client_mode);

  return true;
}

bool Feed::Download(int index) {
  if (category != kFeedCategoryLink)
    return false;

  auto client_mode = taiga::kHttpFeedDownload;
  if (index == -1) {
    for (size_t i = 0; i < items.size(); i++) {
      if (items.at(i).state == kFeedItemSelected) {
        client_mode = taiga::kHttpFeedDownloadAll;
        index = i;
        break;
      }
    }
  }
  if (index < 0 || index > static_cast<int>(items.size()))
    return false;
  download_index = index;

  ui::ChangeStatusText(L"Downloading \"" + items[index].title + L"\"...");
  ui::EnableDialogInput(ui::kDialogTorrents, false);

  std::wstring file = items[index].title + L".torrent";
  ValidateFileName(file);
  file = GetDataPath() + file;

  HttpRequest http_request;
  http_request.url = items[index].link;
  http_request.parameter = reinterpret_cast<LPARAM>(this);

  auto& client = ConnectionManager.GetNewClient(http_request.uuid);
  client.set_download_path(file);
  ConnectionManager.MakeRequest(client, http_request, client_mode);

  return true;
}

bool Feed::ExamineData() {
  foreach_(it, items) {
    // Examine title and compare with anime list items
    Meow.ExamineTitle(it->title, it->episode_data,
                      true, true, true, true, false);
    Meow.MatchDatabase(it->episode_data, true, true);

    // Update last aired episode number
    if (it->episode_data.anime_id > anime::ID_UNKNOWN) {
      auto anime_item = AnimeDatabase.FindItem(it->episode_data.anime_id);
      int episode_number = anime::GetEpisodeHigh(it->episode_data.number);
      anime_item->SetLastAiredEpisodeNumber(episode_number);
    }
  }

  Aggregator.filter_manager.MarkNewEpisodes(*this);
  // Preferences have lower priority, so we need to handle other filters
  // first in order to avoid discarding items that we actually want.
  Aggregator.filter_manager.Filter(*this, false);
  Aggregator.filter_manager.Filter(*this, true);
  // Archived items must be discarded after other filters are processed.
  Aggregator.filter_manager.FilterArchived(*this);

  // Sort items
  std::stable_sort(items.begin(), items.end());
  // Re-assign item indexes
  for (size_t i = 0; i < items.size(); i++)
    items.at(i).index = i;

  return Aggregator.filter_manager.IsItemDownloadAvailable(*this);
}

std::wstring Feed::GetDataPath() {
  std::wstring path = taiga::GetPath(taiga::kPathFeed);

  if (!link.empty()) {
    Url url(link);
    path += Base64Encode(url.host, true) + L"\\";
  }

  return path;
}

bool Feed::Load() {
  std::wstring file = GetDataPath() + L"feed.xml";
  items.clear();

  xml_document document;
  xml_parse_result parse_result = document.load_file(file.c_str());

  if (parse_result.status != pugi::status_ok)
    return false;

  // Read channel information
  xml_node channel = document.child(L"rss").child(L"channel");
  title = XmlReadStrValue(channel, L"title");
  link = XmlReadStrValue(channel, L"link");
  description = XmlReadStrValue(channel, L"description");

  // Read items
  foreach_xmlnode_(item, channel, L"item") {
    // Read data
    items.resize(items.size() + 1);
    items.back().index = items.size() - 1;
    items.back().category = XmlReadStrValue(item, L"category");
    items.back().title = XmlReadStrValue(item, L"title");
    items.back().link = XmlReadStrValue(item, L"link");
    items.back().description = XmlReadStrValue(item, L"description");

    // Remove if title or link is empty
    if (category == kFeedCategoryLink) {
      if (items.back().title.empty() || items.back().link.empty()) {
        items.pop_back();
        continue;
      }
    }

    // Clean up title
    DecodeHtmlEntities(items.back().title);
    Replace(items.back().title, L"\\'", L"'");
    // Clean up description
    Replace(items.back().description, L"<br/>", L"\n");
    Replace(items.back().description, L"<br />", L"\n");
    StripHtmlTags(items.back().description);
    DecodeHtmlEntities(items.back().description);
    Trim(items.back().description, L" \n");
    Aggregator.ParseDescription(items.back(), link);
    Replace(items.back().description, L"\n", L" | ");
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

Aggregator::Aggregator() {
  // Add torrent feed
  feeds.resize(feeds.size() + 1);
  feeds.back().category = kFeedCategoryLink;
}

Feed* Aggregator::Get(FeedCategory category) {
  foreach_(it, feeds)
    if (it->category == category)
      return &(*it);

  return nullptr;
}

bool Aggregator::Notify(const Feed& feed) {
  return ui::OnFeedNotify(feed);
}

bool Aggregator::SearchArchive(const std::wstring& file) {
  foreach_(it, file_archive)
    if (*it == file)
      return true;

  return false;
}

void Aggregator::HandleFeedCheck(Feed& feed, bool automatic) {
  feed.Load();

  bool success = feed.ExamineData();
  ui::OnFeedCheck(success);

  if (automatic) {
    switch (Settings.GetInt(taiga::kTorrent_Discovery_NewAction)) {
      case 1:  // Notify
        Notify(feed);
        break;
      case 2:  // Download
        feed.Download(-1);
        break;
    }
  }
}

void Aggregator::HandleFeedDownload(Feed& feed, bool download_all) {
  auto feed_item = reinterpret_cast<FeedItem*>(&feed.items.at(feed.download_index));

  file_archive.push_back(feed_item->title);

  std::wstring file = feed_item->title;
  ValidateFileName(file);
  file = feed.GetDataPath() + file + L".torrent";

  if (FileExists(file)) {
    std::wstring app_path;
    std::wstring parameters;

    switch (Settings.GetInt(taiga::kTorrent_Download_AppMode)) {
      case 1:  // Default application
        app_path = GetDefaultAppPath(L".torrent", L"");
        break;
      case 2:  // Custom application
        app_path = Settings[taiga::kTorrent_Download_AppPath];
        break;
    }

    if (Settings.GetBool(taiga::kTorrent_Download_UseAnimeFolder) &&
        InStr(app_path, L"utorrent", 0, true) > -1) {
      std::wstring download_path;
      // Use anime folder as the download folder
      auto anime_id = feed_item->episode_data.anime_id;
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
        // Create a subfolder using the anime title as its name
        if (!download_path.empty() &&
            Settings.GetBool(taiga::kTorrent_Download_CreateSubfolder)) {
          std::wstring anime_title;
          if (anime_item) {
            anime_title = anime_item->GetTitle();
          } else {
            anime_title = feed_item->episode_data.title;
          }
          ValidateFileName(anime_title);
          TrimRight(anime_title, L".");
          AddTrailingSlash(download_path);
          download_path += anime_title;
          if (!CreateFolder(download_path))
            LOG(LevelWarning, L"Subfolder could not be created.");
          if (anime_item) {
            anime_item->SetFolder(download_path);
            Settings.Save();
          }
        }
      }

      // Set the command line parameter
      if (!download_path.empty())
        parameters = L"/directory \"" + download_path + L"\" ";
    }

    parameters += L"\"" + file + L"\"";
    Execute(app_path, parameters);

    feed_item->state = kFeedItemDiscardedNormal;
    ui::OnFeedDownload(true);
  }

  feed.download_index = -1;

  if (download_all)
    if (feed.Download(-1))
      return;

  ui::OnFeedDownload(false);
}

void Aggregator::ParseDescription(FeedItem& feed_item,
                                  const std::wstring& source) {
  // AnimeSuki
  if (InStr(source, L"animesuki", 0, true) > -1) {
    std::wstring size_str = L"Filesize: ";
    std::vector<std::wstring> description_vector;
    Split(feed_item.description, L"\n", description_vector);
    if (description_vector.size() > 2) {
      feed_item.episode_data.file_size =
          description_vector[2].substr(size_str.length());
    }
    if (description_vector.size() > 1) {
      feed_item.description =
          description_vector[0] + L" " + description_vector[1];
      return;
    }
    feed_item.description.clear();

  // Baka-Updates
  } else if (InStr(source, L"baka-updates", 0, true) > -1) {
    int index_begin = InStr(feed_item.description, L"Released on");
    int index_end = feed_item.description.length();
    if (index_begin > -1)
      index_end -= index_begin;
    if (index_begin == -1)
      index_begin = 0;
    feed_item.description =
        feed_item.description.substr(index_begin, index_end);

  // NyaaTorrents
  } else if (InStr(source, L"nyaa", 0, true) > -1) {
    feed_item.episode_data.file_size =
        InStr(feed_item.description, L" - ", L" - ");
    Erase(feed_item.description, feed_item.episode_data.file_size);
    Replace(feed_item.description, L"-  -", L"-");

  // TokyoTosho
  } else if (InStr(source, L"tokyotosho", 0, true) > -1) {
    std::wstring size_str = L"Size: ";
    std::wstring comment_str = L"Comment: ";
    std::vector<std::wstring> description_vector;
    Split(feed_item.description, L"\n", description_vector);
    feed_item.description.clear();
    foreach_(it, description_vector) {
      if (StartsWith(*it, size_str)) {
        feed_item.episode_data.file_size = it->substr(size_str.length());
      } else if (StartsWith(*it, comment_str)) {
        feed_item.description = it->substr(comment_str.length());
      } else if (InStr(*it, L"magnet:?") > -1) {
        feed_item.magnet_link = L"magnet:?" +
            InStr(*it, L"<a href=\"magnet:?", L"\">Magnet Link</a>");
      }
    }

  // Yahoo! Pipes
  } else if (InStr(source, L"pipes.yahoo.com", 0, true) > -1) {
    Erase(feed_item.title, L"<span class=\"s\"> </span>");
  }
}

bool Aggregator::LoadArchive() {
  xml_document document;
  std::wstring path = taiga::GetPath(taiga::kPathFeedHistory);
  xml_parse_result parse_result = document.load_file(path.c_str());

  if (parse_result.status != pugi::status_ok)
    return false;

  // Read discarded
  file_archive.clear();
  xml_node archive_node = document.child(L"archive");
  foreach_xmlnode_(node, archive_node, L"item") {
    file_archive.push_back(node.attribute(L"title").value());
  }

  return true;
}

bool Aggregator::SaveArchive() {
  xml_document document;
  xml_node archive_node = document.append_child(L"archive");

  size_t max_count = Settings.GetInt(taiga::kTorrent_Filter_ArchiveMaxCount);

  if (max_count > 0) {
    size_t length = file_archive.size();
    size_t i = 0;
    if (length > max_count)
      i = length - max_count;
    for ( ; i < file_archive.size(); i++) {
      xml_node xml_item = archive_node.append_child(L"item");
      xml_item.append_attribute(L"title") = file_archive[i].c_str();
    }
  }

  std::wstring path = taiga::GetPath(taiga::kPathFeedHistory);
  return XmlWriteDocumentToFile(document, path);
}

bool Aggregator::CompareFeedItems(const GenericFeedItem& item1,
                                  const GenericFeedItem& item2) {
  // Check for guid element first
  if (item1.permalink && item2.permalink)
    if (!item1.guid.empty() || !item2.guid.empty())
      if (item1.guid == item2.guid)
        return true;

  // Fallback to link element
  if (!item1.link.empty() || !item2.link.empty())
    if (item1.link == item2.link)
      return true;

  // Fallback to title element
  if (!item1.title.empty() || !item2.title.empty())
    if (item1.title == item2.title)
      return true;

  // Items are different
  return false;
}