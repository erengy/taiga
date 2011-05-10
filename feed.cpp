/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include "myanimelist.h"
#include "recognition.h"
#include "resource.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "win32/win_taskbar.h"
#include "xml.h"

CAggregator Aggregator;

// =============================================================================

bool CFeed::Check(const wstring& source) {
  // Reset ticker before checking the source so we don't fall into a loop
  Ticker = 0;
  if (source.empty()) return false;
  Link = source;
  
  switch (Category) {
    case FEED_CATEGORY_LINK:
      // Disable toolbar buttons on torrent dialog
      TorrentWindow.m_Toolbar.EnableButton(0, false);
      TorrentWindow.m_Toolbar.EnableButton(1, false);
      break;
  }
  
  CUrl url(Link);
  return Client.Get(url, GetDataPath() + L"feed.xml", 
    HTTP_Feed_Check, reinterpret_cast<LPARAM>(this));
}

bool CFeed::Download(int index) {
  if (Category != FEED_CATEGORY_LINK) {
    DEBUG_PRINT(L"CFeed::Download - How did we end up here?\n");
    return false;
  }
  
  DWORD dwMode = HTTP_Feed_Download;
  if (index == -1) {
    for (size_t i = 0; i < Item.size(); i++) {
      if (Item[i].Download) {
        dwMode = HTTP_Feed_DownloadAll;
        index = i;
        break;
      }
    }
  }
  if (index < 0 || index > static_cast<int>(Item.size())) return false;
  DownloadIndex = index;

  TorrentWindow.ChangeStatus(L"Downloading \"" + Item[index].Title + L"\"...");
  TorrentWindow.m_Toolbar.EnableButton(0, false);
  TorrentWindow.m_Toolbar.EnableButton(1, false);
  
  wstring file = Item[index].Title + L".torrent";
  ValidateFileName(file);
  file = GetDataPath() + file;
  
  CUrl url(Item[index].Link);
  return Client.Get(url, file, dwMode, reinterpret_cast<LPARAM>(this));
}

int CFeed::ExamineData() {
  // Examine title and compare with anime list items
  for (size_t i = 0; i < Item.size(); i++) {
    Meow.ExamineTitle(Item[i].Title, Item[i].EpisodeData, true, true, true, true, false);
    for (int j = AnimeList.Count; j > 0; j--) {
      if (Meow.CompareEpisode(Item[i].EpisodeData, AnimeList.Item[j])) {
        Item[i].EpisodeData.Index = j;
        break;
      }
    }
  }

  // Filter
  int count = Aggregator.FilterManager.Filter(*this);
  return count;
}

wstring CFeed::GetDataPath() {
  wstring path = Taiga.GetDataPath() + L"Feed\\";
  if (!Link.empty()) {
    CUrl url(Link);
    path += Base64Encode(url.Host, true) + L"\\";
  }
  return path;
}

HICON CFeed::GetIcon() {
  if (m_hIcon) return m_hIcon;
  if (Link.empty()) return NULL;

  wstring path = GetDataPath() + L"favicon.ico";

  if (FileExists(path)) {
    m_hIcon = GdiPlus.LoadIcon(path);
    return m_hIcon;
  } else {
    CUrl url(Link + L"/favicon.ico");
    Client.Get(url, path, HTTP_Feed_DownloadIcon, reinterpret_cast<LPARAM>(this));
    return NULL;
  }
}

bool CFeed::Read() {
  // Initialize
  wstring file = GetDataPath() + L"feed.xml";
  Item.clear();

  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok) {
    return false;
  }

  // Read channel information
  xml_node channel = doc.child(L"rss").child(L"channel");
  Title = XML_ReadStrValue(channel, L"title");
  Link = XML_ReadStrValue(channel, L"link");
  Description = XML_ReadStrValue(channel, L"description");

  // Read items
  for (xml_node item = channel.child(L"item"); item; item = item.next_sibling(L"item")) {
    // Read data
    Item.resize(Item.size() + 1);
    Item.back().Index = Item.size() - 1;
    Item.back().Category = XML_ReadStrValue(item, L"category");
    Item.back().Title = XML_ReadStrValue(item, L"title");
    Item.back().Link = XML_ReadStrValue(item, L"link");
    Item.back().Description = XML_ReadStrValue(item, L"description");
    
    // Remove if title or link is empty
    if (Category == FEED_CATEGORY_LINK) {
      if (Item.back().Title.empty() || Item.back().Link.empty()) {
        Item.pop_back();
        continue;
      }
    }
    
    // Clean up title
    DecodeHTML(Item.back().Title);
    Replace(Item.back().Title, L"\\'", L"'");
    // Clean up description
    DecodeHTML(Item.back().Description);
    Replace(Item.back().Description, L"<br/>", L"\n");
    Replace(Item.back().Description, L"<br />", L"\n");
    StripHTML(Item.back().Description);
    Trim(Item.back().Description, L" \n");
    Aggregator.ParseDescription(Item.back(), Link);
    Replace(Item.back().Description, L"\n", L" | ");
    // Get download link
    if (InStr(Item.back().Link, L"nyaatorrents", 0, true) > -1) {
      Replace(Item.back().Link, L"torrentinfo", L"download");
    }
  }

  return true;
}

// =============================================================================

CAggregator::CAggregator() {
  // Add torrent feed
  Feeds.resize(Feeds.size() + 1);
  Feeds.back().Category = FEED_CATEGORY_LINK;

  // TEMP: Add temporary filters for testing purposes
  #ifdef _DEBUG
  #define FM Aggregator.FilterManager
  // Discard unknown titles
  FM.AddFilter(FEED_FILTER_ACTION_EXCLUDE);
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_ID, FEED_FILTER_OPERATOR_IS, L"");
  
  // Discard items for completed titles
  FM.AddFilter(FEED_FILTER_ACTION_EXCLUDE);
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_MYSTATUS, FEED_FILTER_OPERATOR_IS, ToWSTR(MAL_COMPLETED));
  
  // Discard items for dropped titles
  FM.AddFilter(FEED_FILTER_ACTION_EXCLUDE);
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_MYSTATUS, FEED_FILTER_OPERATOR_IS, ToWSTR(MAL_DROPPED));
  
  // Discard already watched episodes
  FM.AddFilter(FEED_FILTER_ACTION_EXCLUDE, FEED_FILTER_MATCH_ANY);
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_EPISODE, FEED_FILTER_OPERATOR_IS, L"%watched%");
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_EPISODE, FEED_FILTER_OPERATOR_ISLESSTHAN, L"%watched%");
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_EPISODEAVAILABLE, FEED_FILTER_OPERATOR_IS, L"1");
  
  // Discard low resolution files
  FM.AddFilter(FEED_FILTER_ACTION_EXCLUDE, FEED_FILTER_MATCH_ANY);
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_RESOLUTION, FEED_FILTER_OPERATOR_CONTAINS, L"360p");
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_RESOLUTION, FEED_FILTER_OPERATOR_CONTAINS, L"480p");
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_RESOLUTION, FEED_FILTER_OPERATOR_CONTAINS, L"704x400");
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_RESOLUTION, FEED_FILTER_OPERATOR_CONTAINS, L"XviD");
  
  // Discard fansub groups other than "Ahodomo" for "Nichijou"
  FM.AddFilter(FEED_FILTER_ACTION_EXCLUDE);
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_ID, FEED_FILTER_OPERATOR_IS, L"10165");
  FM.Filters.back().AddCondition(FEED_FILTER_ELEMENT_ANIME_GROUP, FEED_FILTER_OPERATOR_ISNOT, L"Ahodomo");
  #undef FM
  #endif
}

CFeed* CAggregator::Get(int category) {
  for (unsigned int i = 0; i < Feeds.size(); i++) {
    if (Feeds[i].Category == category) return &Feeds[i];
  }
  return NULL;
}

bool CAggregator::Notify(const CFeed& feed) {
  wstring tip_text, tip_title;

  for (size_t i = 0; i < feed.Item.size(); i++) {
    if (feed.Item[i].Download) {
      tip_text += L"\u00BB " + ReplaceVariables(L"%title%$if(%episode%, #%episode%)\n", 
        feed.Item[i].EpisodeData);
    }
  }

  if (tip_text.empty()) {
    return false;
  } else {
    tip_text += L"Click to see all.";
    tip_title = L"New torrents available";
    LimitText(tip_text, 255);
    Taiga.CurrentTipType = TIPTYPE_TORRENT;
    Taskbar.Tip(L"", L"", 0);
    Taskbar.Tip(tip_text.c_str(), tip_title.c_str(), NIIF_INFO);
    return true;
  }
}

bool CAggregator::SearchArchive(const wstring& file) {
  for (size_t i = 0; i < FileArchive.size(); i++) {
    if (FileArchive[i] == file) return TRUE;
  }
  return FALSE;
}

void CAggregator::ParseDescription(CFeedItem& feed_item, const wstring& source) {
  // AnimeSuki
  if (InStr(source, L"animesuki", 0, true) > -1) {
    wstring size_str = L"Filesize: ";
    vector<wstring> description_vector;
    Split(feed_item.Description, L"\n", description_vector);
    if (description_vector.size() > 2) {
      feed_item.EpisodeData.FileSize = description_vector[2].substr(size_str.length());
    }
    if (description_vector.size() > 1) {
      feed_item.Description = description_vector[0] + L" " + description_vector[1];
      return;
    }
    feed_item.Description.clear();

  // Baka-Updates
  } else if (InStr(source, L"baka-updates", 0, true) > -1) {
    int index_begin = 0, index_end = feed_item.Description.length();
    index_begin = InStr(feed_item.Description, L"Released on");
    if (index_begin > -1) index_end -= index_begin;
    if (index_begin == -1) index_begin = 0;
    feed_item.Description = feed_item.Description.substr(index_begin, index_end);

  // NyaaTorrents
  } else if (InStr(source, L"nyaatorrents", 0, true) > -1) {
    Erase(feed_item.Description, L"None\n");
    Replace(feed_item.Description, L"\n\n", L"\n", true);

  // TokyoTosho
  } else if (InStr(source, L"tokyotosho", 0, true) > -1) {
    wstring size_str = L"Size: ", comment_str = L"Comment: ";
    vector<wstring> description_vector;
    Split(feed_item.Description, L"\n", description_vector);
    if (description_vector.size() > 1) {
      feed_item.EpisodeData.FileSize = description_vector[1].substr(size_str.length());
    }
    if (description_vector.size() > 2) {
      feed_item.Description = description_vector[2].substr(comment_str.length());
      return;
    }
    feed_item.Description.clear();
  }
}

// =============================================================================

bool CAggregator::CompareFeedItems(const CGenericFeedItem& item1, const CGenericFeedItem& item2) {
  // Check for guid element first
  if (item1.IsPermaLink && item2.IsPermaLink) {
    if (!item1.GUID.empty() || !item2.GUID.empty()) {
      if (item1.GUID == item2.GUID) return true;
    }
  }

  // Fallback to link element
  if (!item1.Link.empty() || !item2.Link.empty()) {
    if (item1.Link == item2.Link) return true;
  }

  // Fallback to title element
  if (!item1.Title.empty() || !item2.Title.empty()) {
    if (item1.Title == item2.Title) return true;
  }

  // Items are different
  return false;
}