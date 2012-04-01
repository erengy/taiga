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

#include "std.h"
#include "animelist.h"
#include "common.h"
#include "dlg/dlg_torrent.h"
#include "feed.h"
#include "gfx.h"
#include "recognition.h"
#include "resource.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "win32/win_taskbar.h"
#include "xml.h"

class Aggregator Aggregator;

// =============================================================================

Feed::Feed() : 
  category(0), download_index(-1), ticker(0), icon_(nullptr)
{
}

Feed::~Feed() {
  if (icon_) {
    DestroyIcon(icon_);
    icon_ = nullptr;
  }
}

bool Feed::Check(const wstring& source) {
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
  
  CUrl url(link);
  return client.Get(url, GetDataPath() + L"feed.xml", 
    HTTP_Feed_Check, reinterpret_cast<LPARAM>(this));
}

bool Feed::Download(int index) {
  if (category != FEED_CATEGORY_LINK) {
    DEBUG_PRINT(L"Feed::Download - How did we end up here?\n");
    return false;
  }
  
  DWORD dwMode = HTTP_Feed_Download;
  if (index == -1) {
    for (size_t i = 0; i < items.size(); i++) {
      if (items[i].download) {
        dwMode = HTTP_Feed_DownloadAll;
        index = i;
        break;
      }
    }
  }
  if (index < 0 || index > static_cast<int>(items.size())) return false;
  download_index = index;

  TorrentDialog.ChangeStatus(L"Downloading \"" + items[index].title + L"\"...");
  TorrentDialog.EnableInput(false);
  
  wstring file = items[index].title + L".torrent";
  ValidateFileName(file);
  file = GetDataPath() + file;
  
  CUrl url(items[index].link);
  return client.Get(url, file, dwMode, reinterpret_cast<LPARAM>(this));
}

int Feed::ExamineData() {
  // Examine title and compare with anime list items
  for (size_t i = 0; i < items.size(); i++) {
    Meow.ExamineTitle(items[i].title, items[i].episode_data, true, true, true, true, false);
    for (int j = AnimeList.count; j > 0; j--) {
      if (Meow.CompareEpisode(items[i].episode_data, AnimeList.items[j])) {
        items[i].episode_data.anime_id = AnimeList.items[j].series_id;
        break;
      }
    }
  }

  // Filter
  int count = Aggregator.filter_manager.Filter(*this);
  return count;
}

wstring Feed::GetDataPath() {
  wstring path = Taiga.GetDataPath() + L"Feed\\";
  if (!link.empty()) {
    CUrl url(link);
    path += Base64Encode(url.Host, true) + L"\\";
  }
  return path;
}

HICON Feed::GetIcon() {
  if (link.empty()) return NULL;
  if (icon_) {
    DestroyIcon(icon_);
    icon_ = nullptr;
  }

  wstring path = GetDataPath() + L"favicon.ico";

  if (FileExists(path)) {
    icon_ = GdiPlus.LoadIcon(path);
    return icon_;
  } else {
    CUrl url(link + L"/favicon.ico");
    client.Get(url, path, HTTP_Feed_DownloadIcon, reinterpret_cast<LPARAM>(this));
    return NULL;
  }
}

bool Feed::Load() {
  // Initialize
  wstring file = GetDataPath() + L"feed.xml";
  items.clear();

  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok) {
    return false;
  }

  // Read channel information
  xml_node channel = doc.child(L"rss").child(L"channel");
  title = XML_ReadStrValue(channel, L"title");
  link = XML_ReadStrValue(channel, L"link");
  description = XML_ReadStrValue(channel, L"description");

  // Read items
  for (xml_node item = channel.child(L"item"); item; item = item.next_sibling(L"item")) {
    // Read data
    items.resize(items.size() + 1);
    items.back().index = items.size() - 1;
    items.back().category = XML_ReadStrValue(item, L"category");
    items.back().title = XML_ReadStrValue(item, L"title");
    items.back().link = XML_ReadStrValue(item, L"link");
    items.back().description = XML_ReadStrValue(item, L"description");
    
    // Remove if title or link is empty
    if (category == FEED_CATEGORY_LINK) {
      if (items.back().title.empty() || items.back().link.empty()) {
        items.pop_back();
        continue;
      }
    }
    
    // Clean up title
    DecodeHTML(items.back().title);
    Replace(items.back().title, L"\\'", L"'");
    // Clean up description
    DecodeHTML(items.back().description);
    Replace(items.back().description, L"<br/>", L"\n");
    Replace(items.back().description, L"<br />", L"\n");
    StripHTML(items.back().description);
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
  for (unsigned int i = 0; i < feeds.size(); i++) {
    if (feeds[i].category == category) return &feeds[i];
  }
  return nullptr;
}

bool Aggregator::Notify(const Feed& feed) {
  wstring tip_text, tip_title;

  for (size_t i = 0; i < feed.items.size(); i++) {
    if (feed.items[i].download) {
      tip_text += L"\u00BB " + ReplaceVariables(L"%title%$if(%episode%, #%episode%)\n", 
        feed.items[i].episode_data);
    }
  }

  if (tip_text.empty()) {
    return false;
  } else {
    tip_text += L"Click to see all.";
    tip_title = L"New torrents available";
    LimitText(tip_text, 255);
    Taiga.current_tip_type = TIPTYPE_TORRENT;
    Taskbar.Tip(L"", L"", 0);
    Taskbar.Tip(tip_text.c_str(), tip_title.c_str(), NIIF_INFO);
    return true;
  }
}

bool Aggregator::SearchArchive(const wstring& file) {
  for (size_t i = 0; i < file_archive.size(); i++) {
    if (file_archive[i] == file) return true;
  }
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
  } else if (InStr(source, L"nyaatorrents", 0, true) > -1) {
    Erase(feed_item.description, L"None\n");
    Replace(feed_item.description, L"\n\n", L"\n", true);

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
      }
    }

  // Yahoo! Pipes
  } else if (InStr(source, L"pipes.yahoo.com", 0, true) > -1) {
    Erase(feed_item.title, L"<span class=\"s\"> </span>");
  }
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