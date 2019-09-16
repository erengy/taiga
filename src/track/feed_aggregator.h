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

#pragma once

#include <string>
#include <vector>

#include "base/types.h"
#include "track/feed.h"
#include "track/feed_filter.h"

namespace track {

class Aggregator {
public:
  Feed& GetFeed();

  bool CheckFeed(const std::wstring& source, bool automatic = false);
  bool Download(const FeedItem* feed_item);

  void HandleFeedCheck(Feed& feed, const std::string& data, bool automatic);
  void HandleFeedDownload(Feed& feed, const std::string& data);
  void HandleFeedDownloadError(Feed& feed);
  bool ValidateFeedDownload(const HttpRequest& http_request, HttpResponse& http_response);

  void FindFeedSource(Feed& feed) const;
  void ExamineData(Feed& feed);
  void ParseFeedItem(FeedSource source, FeedItem& feed_item);
  void CleanupDescription(std::wstring& description);

  size_t GetArchiveSize() const;
  bool LoadArchive();
  bool SaveArchive() const;
  void AddToArchive(const std::wstring& file);
  void ClearArchive();
  bool SearchArchive(const std::wstring& file) const;

  FeedFilterManager filter_manager;

private:
  FeedItem* FindFeedItemByLink(Feed& feed, const std::wstring& link);
  void HandleFeedDownloadOpen(FeedItem& feed_item, const std::wstring& file);
  bool IsMagnetLink(const FeedItem& feed_item) const;

  std::vector<std::wstring> download_queue_;
  Feed feed_;
  std::vector<std::wstring> file_archive_;
};

inline track::Aggregator aggregator;

}  // namespace track
