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
#include "http.h"
#include "recognition.h"
#include "resource.h"
#include "rss.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "win32/win_taskbar.h"
#include "xml.h"

CTorrents Torrents;
CHTTPClient TorrentClient;

// =============================================================================

CRSSFeedItem::CRSSFeedItem() {
  NewItem = false;
  Download = false;
}

// =============================================================================

BOOL CTorrents::Check(const wstring& source) {
  Taiga.TickerTorrent = 0;
  if (source.empty()) return FALSE;
  wstring server, object;
  CrackURL(source, server, object);
  TorrentWindow.m_Toolbar.EnableButton(0, false);
  TorrentWindow.m_Toolbar.EnableButton(1, false);
  return TorrentClient.Get(server, object, Taiga.GetDataPath() + L"rss.xml", HTTP_TorrentCheck);
}

BOOL CTorrents::Compare() {
  int torrent_count = 0;
  
  // Examine title and compare with anime list items
  for (size_t i = 0; i < Feed.Item.size(); i++) {
    Meow.ExamineTitle(Feed.Item[i].Title, Feed.Item[i].EpisodeData, true, true, true, true, false);
    for (int j = AnimeList.Count; j > 0; j--) {
      if (Meow.CompareEpisode(Feed.Item[i].EpisodeData, AnimeList.Item[j])) {
        Feed.Item[i].EpisodeData.Index = j;
        break;
      }
    }
  }
  
  // Filter
  for (size_t i = 0; i < Feed.Item.size(); i++) {
    int anime_index = Feed.Item[i].EpisodeData.Index;
    if (anime_index > 0) {
      if (GetLastEpisode(Feed.Item[i].EpisodeData.Number) > AnimeList.Item[anime_index].GetLastWatchedEpisode()) {
        Feed.Item[i].NewItem = true;
        if (Filter(i, anime_index)) {
          wstring file = Feed.Item[i].Title + L".torrent";
          ValidateFileName(file);
          if (!SearchArchive(file)) {
            Feed.Item[i].Download = true;
            torrent_count++;
          }
        }
      }
    }
  }

  return torrent_count;
}

BOOL CTorrents::Download(int index) {
  DWORD dwMode = HTTP_TorrentDownload;
  
  if (index == -1) {
    for (size_t i = 0; i < Feed.Item.size(); i++) {
      if (Feed.Item[i].Download) {
        dwMode = HTTP_TorrentDownloadAll;
        index = i;
        break;
      }
    }
  }
  
  if (index < 0 || index > static_cast<int>(Feed.Item.size()))
    return FALSE;

  TorrentWindow.ChangeStatus(L"Downloading \"" + Feed.Item[index].Title + L"\"...");
  TorrentWindow.m_Toolbar.EnableButton(0, false);
  TorrentWindow.m_Toolbar.EnableButton(1, false);
  
  wstring server, object;
  CrackURL(Feed.Item[index].Link, server, object);
  
  wstring folder = Taiga.GetDataPath() + L"Torrents\\";
  CreateDirectory(folder.c_str(), NULL);
  wstring file = Feed.Item[index].Title + L".torrent";
  ValidateFileName(file);
  
  return TorrentClient.Get(server, object, folder + file, dwMode, 
    reinterpret_cast<LPARAM>(&Feed.Item[index]));
}

BOOL CTorrents::Read() {
  // Initialize
  wstring file = Taiga.GetDataPath() + L"rss.xml";
  Feed.Item.clear();

  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok) {
    return FALSE;
  }

  // Read information
  xml_node channel = doc.child(L"rss").child(L"channel");
  Feed.Title = XML_ReadStrValue(channel, L"title");
  Feed.Link = XML_ReadStrValue(channel, L"link");
  Feed.Description = XML_ReadStrValue(channel, L"description");

  // Read items
  for (xml_node item = channel.child(L"item"); item; item = item.next_sibling(L"item")) {
    // Read data
    Feed.Item.resize(Feed.Item.size() + 1);
    Feed.Item.back().Index = Feed.Item.size() - 1;
    Feed.Item.back().Category = XML_ReadStrValue(item, L"category");
    Feed.Item.back().Title = XML_ReadStrValue(item, L"title");
    Feed.Item.back().Link = XML_ReadStrValue(item, L"link");
    Feed.Item.back().Description = XML_ReadStrValue(item, L"description");
    // Remove if title or link is empty
    if (Feed.Item.back().Title.empty() || Feed.Item.back().Link.empty()) {
      Feed.Item.pop_back();
      continue;
    }
    // Clean up title
    DecodeHTML(Feed.Item.back().Title);
    Replace(Feed.Item.back().Title, L"\\'", L"'");
    // Clean up description
    DecodeHTML(Feed.Item.back().Description);
    Replace(Feed.Item.back().Description, L"<br/>", L"\n");
    Replace(Feed.Item.back().Description, L"<br />", L"\n");
    StripHTML(Feed.Item.back().Description);
    Trim(Feed.Item.back().Description, L" \n");
    ParseDescription(Feed.Item.back(), Feed.Link);
    Replace(Feed.Item.back().Description, L"\n", L" | ");
    // Get real link
    if (InStr(Feed.Item.back().Link, L"nyaatorrents", 0, true) > -1) {
      Replace(Feed.Item.back().Link, L"torrentinfo", L"download");
    }
  }
  
  return TRUE;
}

BOOL CTorrents::SearchArchive(const wstring& file) {
  for (size_t i = 0; i < Archive.size(); i++) {
    if (Archive[i] == file) return TRUE;
  }
  return FALSE;
}

BOOL CTorrents::Tip() {
  wstring tip_text, tip_title;

  for (size_t i = 0; i < Feed.Item.size(); i++) {
    if (Feed.Item[i].Download) {
      tip_text += L"\u00BB " + ReplaceVariables(L"%title%$if(%episode%, #%episode%)\n", 
        Feed.Item[i].EpisodeData);
    }
  }

  if (tip_text.empty()) {
    return FALSE;
  } else {
    tip_text += L"Click to see all.";
    tip_title = L"New torrents available";
    LimitText(tip_text, 255);
    Taiga.CurrentTipType = TIPTYPE_TORRENT;
    Taskbar.Tip(L"", L"", 0);
    Taskbar.Tip(tip_text.c_str(), tip_title.c_str(), NIIF_INFO);
    return TRUE;
  }
}

// =============================================================================

void CTorrents::AddFilter(int option, int type, wstring value) {
  #define FILTERS Settings.RSS.Torrent.Filters.Global
  FILTERS.resize(FILTERS.size() + 1);
  FILTERS.back().Option = option;
  FILTERS.back().Type = type;
  FILTERS.back().Value = value;
  #undef FILTERS
}

BOOL CTorrents::Filter(int feed_index, int anime_index) {
  // Filter fansub group preference
  if (!AnimeList.Item[anime_index].FansubGroup.empty() && 
    !IsEqual(AnimeList.Item[anime_index].FansubGroup, Feed.Item[feed_index].EpisodeData.Group)) {
      return FALSE;
  }
  
  // Don't bother if global filters are disabled
  if (Settings.RSS.Torrent.Filters.GlobalEnabled == FALSE) {
    return TRUE;
  }
  
  for (size_t i = 0; i < Settings.RSS.Torrent.Filters.Global.size(); i++) {
    #define FEED Feed.Item[feed_index]
    #define FILTER Settings.RSS.Torrent.Filters.Global[i]
    switch (FILTER.Type) {
      // Filter keyword
      case RSS_FILTER_KEYWORD: {
        switch (FILTER.Option) {
          // Exclude
          case RSS_FILTER_EXCLUDE: {
            if (InStr(FEED.EpisodeData.Title, FILTER.Value, 0, true) > -1 || 
              InStr(FEED.Title, FILTER.Value, 0, true) > -1 || 
              InStr(FEED.Link, FILTER.Value, 0, true) > -1) {
                return FALSE;
            }
            break;
          }
          // Include
          case RSS_FILTER_INCLUDE: {
            if (InStr(FEED.EpisodeData.Title, FILTER.Value, 0, true) == -1 && 
              InStr(FEED.Title, FILTER.Value, 0, true) == -1 && 
              InStr(FEED.Link, FILTER.Value, 0, true) == -1) {
                return FALSE;
            }
            break;
          }
          // Preference
          // TODO: Fix this
          case RSS_FILTER_PREFERENCE: {
            if (InStr(FEED.EpisodeData.Title, FILTER.Value, 0, true) > -1 || 
              InStr(FEED.Title, FILTER.Value, 0, true) > -1 || 
              InStr(FEED.Link, FILTER.Value, 0, true) > -1) {
                break;
            }
            for (size_t j = 0; j < Feed.Item.size(); j++) {
              if (j == feed_index || 
                Feed.Item[j].EpisodeData.Index  != FEED.EpisodeData.Index || 
                Feed.Item[j].EpisodeData.Number != FEED.EpisodeData.Number) {
                  continue;
              }
              if (InStr(FEED.EpisodeData.Title, FILTER.Value, 0, true) > -1 || 
                InStr(Feed.Item[j].Title, FILTER.Value, 0, true) > -1 || 
                InStr(Feed.Item[j].Link, FILTER.Value, 0, true) > -1) {
                  return FALSE;
              }
            }
            break;
          }
        }
        break;
      }
      // Filter airing status
      case RSS_FILTER_AIRINGSTATUS: {
        if (anime_index > 0 && 
          AnimeList.Item[anime_index].Series_Status == ToINT(FILTER.Value)) {
            return FALSE;
        }
        break;
      }
      // Filter watching status
      case RSS_FILTER_WATCHINGSTATUS: {
        if (anime_index > 0 && 
          AnimeList.Item[anime_index].GetStatus() == ToINT(FILTER.Value)) {
            return FALSE;
        }
        break;
      }
    }
    #undef FEED
    #undef FILTER
  }

  // Passed
  return TRUE;
}

// =============================================================================

void CTorrents::ParseDescription(CRSSFeedItem& feed_item, const wstring& source) {
  // AnimeSuki
  if (InStr(source, L"animesuki", 0, true) > -1) {
    wstring size_str = L"Filesize: ";
    vector<wstring> description_vector;
    Split(feed_item.Description, L"\n", description_vector);
    if (description_vector.size() > 2) {
      feed_item.Size = description_vector[2].substr(size_str.length());
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
      feed_item.Size = description_vector[1].substr(size_str.length());
    }
    if (description_vector.size() > 2) {
      feed_item.Description = description_vector[2].substr(comment_str.length());
      return;
    }
    feed_item.Description.clear();
  }
}