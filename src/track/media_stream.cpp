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

#include <regex>
#include <set>

#include "track/media_stream.h"

#include "base/format.h"
#include "base/process.h"
#include "base/string.h"
#include "base/url.h"
#include "taiga/settings.h"
#include "track/episode.h"

namespace track::recognition {

static const std::vector<StreamData> stream_data{
  // AnimeLab
  {
    Stream::Animelab,
    L"AnimeLab",
    L"https://www.animelab.com",
    std::regex("animelab\\.com/player/"),
    std::regex("AnimeLab - (.+)"),
  },
  // Anime Digital Network
  {
    Stream::Adn,
    L"Anime Digital Network",
    L"https://animedigitalnetwork.fr/video/",
    std::regex("animedigitalnetwork.fr/video/[^/]+/[0-9]+"),
    std::regex("(.+) - streaming -.* ADN"),
  },
  // Anime News Network
  {
    Stream::Ann,
    L"Anime News Network",
    L"https://www.animenewsnetwork.com/video/",
    std::regex("animenewsnetwork\\.(?:com|cc)/video/[0-9]+"),
    std::regex("(.+) - Anime News Network"),
  },
  // Bilibili
  {
    Stream::Bilibili,
    L"Bilibili",
    L"https://www.bilibili.tv/en/anime",
    std::regex("bilibili.tv/[^/]+/play/[0-9]+"),
    std::regex("(.+) - Bilibili"),
  },
  // Jellyfin Web App
  {
    Stream::Jellyfin,
    L"Jellyfin Web App",
    L"https://jellyfin.org",
    std::regex("^.+/web/(?:index\\.html)?#!/video"),
    std::regex("Jellyfin|(.+)"),
  },
  // Plex Web App
  {
    Stream::Plex,
    L"Plex Web App",
    L"https://www.plex.tv",
    std::regex(
      "^app\\.plex\\.tv/desktop|"
      "^[^/]*?plex\\.tv/web/|"
      "^localhost:32400/web/|"
      "^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}:32400/web/|"
      "^plex\\.[a-z0-9-]+\\.[a-z0-9-]+|"
      "^[^/]*[a-z0-9-]+\\.[a-z0-9-]+/plex"
    ),
    std::regex(u8"Plex|(?:\u25B6 )?(.+)"),
  },
  // Roku Channel
  {
    Stream::RokuChannel,
    L"Roku Channel",
    L"https://therokuchannel.roku.com",
    std::regex("therokuchannel\\.roku\\.com/watch/.+"),
    std::regex("Watch (.+) Online for Free \\| The Roku Channel \\| Roku")
  },
  // Tubi
  {
    Stream::Tubi,
    L"Tubi",
    L"https://tubitv.com",
    std::regex("tubitv\\.com/tv-shows/.+"),
    std::regex("Watch (.+) - Free TV Shows \\| Tubi")
  },
  // Veoh
  {
    Stream::Veoh,
    L"Veoh",
    L"http://www.veoh.com",
    std::regex("veoh\\.com/watch/"),
    std::regex("Watch Videos Online \\| (.+) \\| Veoh\\.com"),
  },
  // VIZ
  {
    Stream::Viz,
    L"VIZ",
    L"https://www.viz.com/watch",
    std::regex("viz\\.com/watch/streaming/[^/]+-(?:episode-[0-9]+|movie)/"),
    std::regex("(.+) // VIZ"),
  },
  // VRV
  {
    Stream::Vrv,
    L"VRV",
    L"https://vrv.co",
    std::regex("vrv\\.co/watch/"),
    std::regex("(.+) - Watch on VRV"),
  },
  // Wakanim
  {
    Stream::Wakanim,
    L"Wakanim",
    L"https://www.wakanim.tv",
    std::regex("wakanim\\.tv/[^/]+/v2/catalogue/episode/[^/]+/"),
    std::regex("(.+) (?:auf|on|sur) Wakanim\\.TV.*"),
  },
  // Yahoo View
  {
    Stream::Yahoo,
    L"Yahoo View",
    L"https://view.yahoo.com",
    std::regex("view.yahoo.com/show/[^/]+/episode/[^/]+/"),
    std::regex("Watch .+ Free Online - (.+) \\| Yahoo View"),
  },
  // YouTube
  {
    Stream::Youtube,
    L"YouTube",
    L"https://www.youtube.com",
    std::regex("youtube\\.com/watch"),
    std::regex(u8"YouTube|(?:\u25B6 )?(.+) - YouTube"),
  },
};

const std::vector<StreamData>& GetStreamData() {
  return stream_data;
}

bool IsStreamEnabled(const Stream stream) {
  switch (stream) {
    case Stream::Animelab:
      return taiga::settings.GetStreamAnimelab();
    case Stream::Adn:
      return taiga::settings.GetStreamAdn();
    case Stream::Ann:
      return taiga::settings.GetStreamAnn();
    case Stream::Bilibili:
      return taiga::settings.GetStreamBilibili();
    case Stream::Jellyfin:
      return taiga::settings.GetStreamJellyfin();
    case Stream::Plex:
      return taiga::settings.GetStreamPlex();
    case Stream::RokuChannel:
      return taiga::settings.GetStreamRokuChannel();
    case Stream::Tubi:
      return taiga::settings.GetStreamTubi();
    case Stream::Veoh:
      return taiga::settings.GetStreamVeoh();
    case Stream::Viz:
      return taiga::settings.GetStreamViz();
    case Stream::Vrv:
      return taiga::settings.GetStreamVrv();
    case Stream::Wakanim:
      return taiga::settings.GetStreamWakanim();
    case Stream::Yahoo:
      return taiga::settings.GetStreamYahoo();
    case Stream::Youtube:
      return taiga::settings.GetStreamYoutube();
  }
  return false;
}

void EnableStream(const Stream stream, const bool enabled) {
  switch (stream) {
    case Stream::Animelab:
      taiga::settings.SetStreamAnimelab(enabled);
      break;
    case Stream::Adn:
      taiga::settings.SetStreamAdn(enabled);
      break;
    case Stream::Ann:
      taiga::settings.SetStreamAnn(enabled);
      break;
    case Stream::Bilibili:
      taiga::settings.SetStreamBilibili(enabled);
      break;
    case Stream::Jellyfin:
      taiga::settings.SetStreamJellyfin(enabled);
      break;
    case Stream::Plex:
      taiga::settings.SetStreamPlex(enabled);
      break;
    case Stream::RokuChannel:
      taiga::settings.SetStreamRokuChannel(enabled);
      break;
    case Stream::Tubi:
      taiga::settings.SetStreamTubi(enabled);
      break;
    case Stream::Veoh:
      taiga::settings.SetStreamVeoh(enabled);
      break;
    case Stream::Viz:
      taiga::settings.SetStreamViz(enabled);
      break;
    case Stream::Vrv:
      taiga::settings.SetStreamVrv(enabled);
      break;
    case Stream::Wakanim:
      taiga::settings.SetStreamWakanim(enabled);
      break;
    case Stream::Yahoo:
      taiga::settings.SetStreamYahoo(enabled);
      break;
    case Stream::Youtube:
      taiga::settings.SetStreamYoutube(enabled);
      break;
  }
}

const StreamData* FindStreamFromUrl(std::wstring url) {
  EraseLeft(url, L"http://");
  EraseLeft(url, L"https://");

  if (url.empty())
    return nullptr;

  const std::string str = WstrToStr(url);

  for (const auto& item : stream_data) {
    if (std::regex_search(str, item.url_pattern)) {;
      return IsStreamEnabled(item.id) ? &item : nullptr;
    }
  }

  return nullptr;
}

bool ApplyStreamTitleFormat(const StreamData& stream_data, std::string& title) {
  std::smatch match;
  std::regex_match(title, match, stream_data.title_pattern);

  // Use the first non-empty match result
  for (size_t i = 1; i < match.size(); ++i) {
    if (!match.str(i).empty()) {
      title = match.str(i);
      return true;
    }
  }

  // Results are empty, but the match was successful
  if (!match.empty()) {
    title.clear();
    return true;
  }

  return false;
}

void CleanStreamTitle(const StreamData& stream_data, std::string& title) {
  if (!ApplyStreamTitleFormat(stream_data, title))
    return;

  switch (stream_data.id) {
    case Stream::Adn: {
      auto str = StrToWstr(title);
      ReplaceString(str, L" : ", L" - ");
      title = WstrToStr(str);
      break;
    }
    case Stream::Ann: {
      static const std::regex pattern{" \\((?:s|d)(?:, uncut)?\\)"};
      title = std::regex_replace(title, pattern, "");
      break;
    }
    case Stream::Plex: {
      auto str = StrToWstr(title);
      ReplaceString(str, L" \u00B7 ", L"");
      title = WstrToStr(str);
      break;
    }
    case Stream::RokuChannel:
    case Stream::Tubi: {
      static const std::regex pattern{" S(\\d+):E(\\d+) "};
      title = std::regex_replace(title, pattern, " S$1E$2 ");
      break;
    }
    case Stream::Vrv: {
      auto str = StrToWstr(title);
      ReplaceString(str, 0, L": EP ", L" - EP ", false, false);
      title = WstrToStr(str);
      break;
    }
    case Stream::Wakanim: {
      static const std::regex pattern{
          "(?:Episode (\\d+)|Film|Movie) - (?:ENGDUB - )?(.+)"};
      std::smatch match;
      if (std::regex_match(title, match, pattern))
        title = match.str(2) +
                (match.length(1) ? " - Episode " + match.str(1) : "");
      break;
    }
  }
}

bool GetTitleFromStreamingMediaProvider(const std::wstring& url,
                                        std::wstring& title) {
  const auto stream = FindStreamFromUrl(url);

  if (stream) {
    std::string str = WstrToStr(title);
    CleanStreamTitle(*stream, str);
    title = StrToWstr(str);
  } else {
    title.clear();
  }

  return !title.empty();
}

////////////////////////////////////////////////////////////////////////////////

void IgnoreCommonWebBrowserTitles(const std::wstring& address,
                                  std::wstring& title) {
  const auto has_http_scheme = [](const std::wstring& str) {
    return StartsWith(str, L"http://") || StartsWith(str, L"https://");
  };
  if (!address.empty()) {
    const Url url = Url::Parse(
        // Chrome's address bar hides the scheme, so what we have might be an
        // invalid HTTP URI ("www." is hidden too, which does not affect us).
        has_http_scheme(address) ? address : L"http://{}"_format(address),
        false);
    if (!url.host().empty() && StartsWith(title, url.host()))  // Chrome
      title.clear();
  }
  if (has_http_scheme(title))
    title.clear();

  static const std::set<std::wstring> common_titles{
    L"Blank Page",            // Internet Explorer
    L"InPrivate",             // Internet Explorer
    L"New Tab",               // Chrome, Firefox
    L"Private Browsing",      // Firefox
    L"Private browsing",      // Opera
    L"Problem loading page",  // Firefox
    L"Speed Dial",            // Opera
    L"Untitled",              // Chrome
  };
  if (common_titles.count(title))
    title.clear();

  static const std::vector<std::wstring> common_suffixes{
    L" - Crashed",        // Chrome
    L" - Network error",  // Chrome
  };
  for (const auto& suffix : common_suffixes) {
    if (EndsWith(title, suffix)) {
      title.clear();
      return;
    }
  }
}

void RemoveCommonWebBrowserAffixes(std::wstring& title) {
  // Ignoring all localized strings is not feasible, because Chrome is available
  // in 50+ languages and translations can change over time.
  static const std::vector<std::wstring> common_suffixes{
    L" - Audio muted",    // Chrome
    L" - Audio playing",  // Chrome
  };
  for (const auto& suffix : common_suffixes) {
    EraseRight(title, suffix);
  }
}

void NormalizeWebBrowserTitle(const std::wstring& url, std::wstring& title) {
  IgnoreCommonWebBrowserTitles(url, title);
  RemoveCommonWebBrowserAffixes(title);
}

}  // namespace track::recognition
