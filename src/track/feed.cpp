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

#include "feed.h"

#include "library/anime_db.h"
#include "base/common.h"
#include "base/file.h"
#include "base/encoding.h"
#include "base/encryption.h"
#include "base/foreach.h"
#include "base/gfx.h"
#include "recognition.h"
#include "taiga/path.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "base/types.h"
#include "taiga/script.h"
#include "taiga/taiga.h"
#include "base/xml.h"

#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_torrent.h"

#include "win/win_taskbar.h"

class Aggregator Aggregator;

// =============================================================================

Feed::Feed()
    : category(0), 
      download_index(-1), 
      ticker(0) {
}

bool Feed::Check(const wstring& source, bool automatic) {
  // Reset ticker before checking the source so we don't fall into a loop
  ticker = 0;
  if (source.empty()) return false;
  link = source;
  
  switch (category) {
    case FEED_CATEGORY_LINK:
      // Disable torrent dialog input
      TorrentDialog.EnableInput(false);
      break;
  }

  win::http::Url url(link);
  
  HttpRequest http_request;
  http_request.host = url.host;
  http_request.path = url.path;
  http_request.parameter = reinterpret_cast<LPARAM>(this);
  
  auto client_mode = automatic ? taiga::kHttpFeedCheckAuto : taiga::kHttpFeedCheck;
  auto& client = ConnectionManager.GetNewClient(http_request.uuid);
  client.set_download_path(GetDataPath() + L"feed.xml");
  ConnectionManager.MakeRequest(client, http_request, client_mode);
  return true;
}

bool Feed::Download(int index) {
  if (category != FEED_CATEGORY_LINK)
    return false;
  
  auto client_mode = taiga::kHttpFeedDownload;
  if (index == -1) {
    for (size_t i = 0; i < items.size(); i++) {
      if (items[i].state == FEEDITEM_SELECTED) {
        client_mode = taiga::kHttpFeedDownloadAll;
        index = i;
        break;
      }
    }
  }
  if (index < 0 || index > static_cast<int>(items.size())) return false;
  download_index = index;

  MainDialog.ChangeStatus(L"Downloading \"" + items[index].title + L"\"...");
  TorrentDialog.EnableInput(false);
  
  wstring file = items[index].title + L".torrent";
  ValidateFileName(file);
  file = GetDataPath() + file;

  win::http::Url url(items[index].link);
  
  HttpRequest http_request;
  http_request.host = url.host;
  http_request.path = url.path;
  http_request.parameter = reinterpret_cast<LPARAM>(this);

  auto& client = ConnectionManager.GetNewClient(http_request.uuid);
  client.set_download_path(file);
  ConnectionManager.MakeRequest(client, http_request, client_mode);
  return true;
}

bool Feed::ExamineData() {
  for (size_t i = 0; i < items.size(); i++) {
    // Examine title and compare with anime list items
    Meow.ExamineTitle(items[i].title, items[i].episode_data, true, true, true, true, false);
    Meow.MatchDatabase(items[i].episode_data, true, true);
    
    // Update last aired episode number
    if (items[i].episode_data.anime_id > anime::ID_UNKNOWN) {
      auto anime_item = AnimeDatabase.FindItem(items[i].episode_data.anime_id);
      int episode_number = GetEpisodeHigh(items[i].episode_data.number);
      anime_item->SetLastAiredEpisodeNumber(episode_number);
    }
  }

  // Sort items by their anime ID, giving priority to identified items, while
  // preserving the order for unidentified ones.
  std::stable_sort(items.begin(), items.end(),
    [](const FeedItem& a, const FeedItem& b) {
      return a.episode_data.anime_id > b.episode_data.anime_id;
    });
  // Re-assign item indexes
  for (size_t i = 0; i < items.size(); i++)
    items.at(i).index = i;

  // Filter
  Aggregator.filter_manager.MarkNewEpisodes(*this);
  // Preferences have lower priority, so we need to handle other filters
  // first in order to avoid discarding items that we actually want.
  Aggregator.filter_manager.Filter(*this, false);
  Aggregator.filter_manager.Filter(*this, true);
  // Archived items must be discarded after other filters are processed.
  Aggregator.filter_manager.FilterArchived(*this);
  
  return Aggregator.filter_manager.IsItemDownloadAvailable(*this);
}

wstring Feed::GetDataPath() {
  wstring path = taiga::GetPath(taiga::kPathFeed);

  if (!link.empty()) {
    win::http::Url url(link);
    path += Base64Encode(url.host, true) + L"\\";
  }

  return path;
}

bool Feed::Load() {
  // Initialize
  wstring file = GetDataPath() + L"feed.xml";
  items.clear();

  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != pugi::status_ok) {
    return false;
  }

  // Read channel information
  xml_node channel = doc.child(L"rss").child(L"channel");
  title = XmlReadStrValue(channel, L"title");
  link = XmlReadStrValue(channel, L"link");
  description = XmlReadStrValue(channel, L"description");

  // Read items
  for (xml_node item = channel.child(L"item"); item; item = item.next_sibling(L"item")) {
    // Read data
    items.resize(items.size() + 1);
    items.back().index = items.size() - 1;
    items.back().category = XmlReadStrValue(item, L"category");
    items.back().title = XmlReadStrValue(item, L"title");
    items.back().link = XmlReadStrValue(item, L"link");
    items.back().description = XmlReadStrValue(item, L"description");
    
    // Remove if title or link is empty
    if (category == FEED_CATEGORY_LINK) {
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
    // Get download link
    if (InStr(items.back().link, L"nyaatorrents", 0, true) > -1) {
      Replace(items.back().link, L"torrentinfo", L"download");
    }
  }

  return true;
}

// =============================================================================

Aggregator::Aggregator() {
  // Add torrent feed
  feeds.resize(feeds.size() + 1);
  feeds.back().category = FEED_CATEGORY_LINK;
}

Feed* Aggregator::Get(int category) {
  foreach_(it, feeds)
    if (it->category == category)
      return &(*it);

  return nullptr;
}

bool Aggregator::Notify(const Feed& feed) {
  wstring tip_text;
  wstring tip_title = L"New torrents available";
  wstring tip_format = L"%title%$if(%episode%, #%episode%)\n";

  foreach_(it, feed.items)
    if (it->state == FEEDITEM_SELECTED)
      tip_text += L"\u00BB " + ReplaceVariables(tip_format, it->episode_data);

  if (tip_text.empty())
    return false;

  tip_text += L"Click to see all.";
  tip_text = LimitText(tip_text, 255);
  Taiga.current_tip_type = taiga::kTipTypeTorrent;
  Taskbar.Tip(L"", L"", 0);
  Taskbar.Tip(tip_text.c_str(), tip_title.c_str(), NIIF_INFO);
  
  return true;
}

bool Aggregator::SearchArchive(const wstring& file) {
  for (size_t i = 0; i < file_archive.size(); i++)
    if (file_archive[i] == file)
      return true;

  return false;
}

void Aggregator::ParseDescription(FeedItem& feed_item, const wstring& source) {
  // AnimeSuki
  if (InStr(source, L"animesuki", 0, true) > -1) {
    wstring size_str = L"Filesize: ";
    vector<wstring> description_vector;
    Split(feed_item.description, L"\n", description_vector);
    if (description_vector.size() > 2) {
      feed_item.episode_data.file_size = description_vector[2].substr(size_str.length());
    }
    if (description_vector.size() > 1) {
      feed_item.description = description_vector[0] + L" " + description_vector[1];
      return;
    }
    feed_item.description.clear();

  // Baka-Updates
  } else if (InStr(source, L"baka-updates", 0, true) > -1) {
    int index_begin = 0, index_end = feed_item.description.length();
    index_begin = InStr(feed_item.description, L"Released on");
    if (index_begin > -1) index_end -= index_begin;
    if (index_begin == -1) index_begin = 0;
    feed_item.description = feed_item.description.substr(index_begin, index_end);

  // NyaaTorrents
  } else if (InStr(source, L"nyaa", 0, true) > -1) {
    feed_item.episode_data.file_size = InStr(feed_item.description, L" - ", L" - ");
    Erase(feed_item.description, feed_item.episode_data.file_size);
    Replace(feed_item.description, L"-  -", L"-");

  // TokyoTosho
  } else if (InStr(source, L"tokyotosho", 0, true) > -1) {
    wstring size_str = L"Size: ", comment_str = L"Comment: ";
    vector<wstring> description_vector;
    Split(feed_item.description, L"\n", description_vector);
    feed_item.description.clear();
    for (auto it = description_vector.begin(); it != description_vector.end(); ++it) {
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
  wstring path = taiga::GetPath(taiga::kPathFeedHistory);
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

  int max_count = Settings.GetInt(taiga::kTorrent_Filter_ArchiveMaxCount);

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

  wstring path = taiga::GetPath(taiga::kPathFeedHistory);
  return XmlWriteDocumentToFile(document, path);
}

// =============================================================================

bool Aggregator::CompareFeedItems(const GenericFeedItem& item1, const GenericFeedItem& item2) {
  // Check for guid element first
  if (item1.is_permalink && item2.is_permalink) {
    if (!item1.guid.empty() || !item2.guid.empty()) {
      if (item1.guid == item2.guid) return true;
    }
  }

  // Fallback to link element
  if (!item1.link.empty() || !item2.link.empty()) {
    if (item1.link == item2.link) return true;
  }

  // Fallback to title element
  if (!item1.title.empty() || !item2.title.empty()) {
    if (item1.title == item2.title) return true;
  }

  // items are different
  return false;
}