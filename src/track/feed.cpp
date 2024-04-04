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

#include "track/feed.h"

#include "base/base64.h"
#include "base/html.h"
#include "base/string.h"
#include "base/url.h"
#include "base/xml.h"
#include "taiga/path.h"
#include "track/episode_util.h"
#include "track/feed_filter.h"
#include "track/recognition.h"

namespace track {

static void TidyFeedItemDescription(std::wstring& description) {
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

void FeedItem::Discard(int option) {
  switch (option) {
    default:
    case kFeedFilterOptionDefault:
      state = FeedItemState::DiscardedNormal;
      break;
    case kFeedFilterOptionDeactivate:
      state = FeedItemState::DiscardedInactive;
      break;
    case kFeedFilterOptionHide:
      state = FeedItemState::DiscardedHidden;
      break;
  }
}

bool FeedItem::IsDiscarded() const {
  switch (state) {
    case FeedItemState::DiscardedNormal:
    case FeedItemState::DiscardedInactive:
    case FeedItemState::DiscardedHidden:
      return true;
    default:
      return false;
  }
}

bool FeedItem::operator<(const FeedItem& item) const {
  constexpr int state_priorities[] = {1, 2, 3, 4, 0};

  // Sort items by the priority of their state
  return state_priorities[static_cast<size_t>(this->state)] <
         state_priorities[static_cast<size_t>(item.state)];
}

bool FeedItem::operator==(const FeedItem& item) const {
  // Check for guid element first
  if (guid.is_permalink && item.guid.is_permalink)
    if (!guid.value.empty() || !item.guid.value.empty())
      if (guid.value == item.guid.value)
        return true;

  // Fall back to link element
  if (!link.empty() || !item.link.empty())
    if (link == item.link)
      return true;

  // Fall back to title element
  if (!title.empty() || !item.title.empty())
    if (title == item.title)
      return true;

  // Items are different
  return false;
}

TorrentCategory GetTorrentCategory(const FeedItem& item) {
  // Respect our previous categorization
  if (item.torrent_category != TorrentCategory::Anime)
    return item.torrent_category;

  // Respect feed's own categorization
  if (InStr(item.category.value, L"Batch") > -1)
    return TorrentCategory::Batch;

  if (Meow.IsBatchRelease(item.episode_data))
    return TorrentCategory::Batch;

  if (!Meow.IsValidAnimeType(item.episode_data))
    return TorrentCategory::Other;

  if (anime::IsEpisodeRange(item.episode_data))
    return TorrentCategory::Batch;

  if (!item.episode_data.file_extension().empty())
    if (!Meow.IsValidFileExtension(item.episode_data.file_extension()))
      return TorrentCategory::Other;

  return TorrentCategory::Anime;
}

std::wstring TranslateTorrentCategory(TorrentCategory category) {
  switch (category) {
    case TorrentCategory::Anime: return L"Anime";
    case TorrentCategory::Batch: return L"Batch";
    case TorrentCategory::Other: return L"Other";
    default: return L"Anime";
  }
}

TorrentCategory TranslateTorrentCategory(const std::wstring& str) {
  static const std::map<std::wstring, TorrentCategory> categories{
    {L"Anime", TorrentCategory::Anime},
    {L"Batch", TorrentCategory::Batch},
    {L"Other", TorrentCategory::Other},
  };

  const auto it = categories.find(str);
  return it != categories.end() ? it->second : TorrentCategory::Anime;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring Feed::GetDataPath() const {
  std::wstring path = taiga::GetPath(taiga::Path::Feed);

  if (!channel.link.empty()) {
    Url url(channel.link);
    path += Base64Encode(url.host(), true) + L"\\";
  }

  return path;
}

bool Feed::Load() {
  items.clear();

  XmlDocument document;
  const auto path = GetDataPath() + L"feed.xml";
  constexpr auto options = pugi::parse_default | pugi::parse_trim_pcdata;
  const auto parse_result = XmlLoadFileToDocument(document, path, options);

  if (!parse_result)
    return false;

  Load(document);
  return true;
}

bool Feed::Load(const std::wstring& data) {
  items.clear();

  XmlDocument document;
  const auto parse_result = document.load_string(data.c_str());

  if (!parse_result)
    return false;

  Load(document);
  return true;
}

void Feed::Load(const XmlDocument& document) {
  auto rss_feed = rss::ParseDocument(document);
  channel = rss_feed.channel;

  for (auto& item : rss_feed.items) {
    items.push_back(FeedItem{std::move(item)});
  }

  source = GetFeedSource(channel.link);

  for (auto& item : items) {
    ParseFeedItemFromSource(source, item);
    TidyFeedItemDescription(item.description);
  }
}

}  // namespace track
