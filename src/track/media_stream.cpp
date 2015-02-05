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

#include "base/foreach.h"
#include "base/process.h"
#include "base/string.h"
#include "library/anime_episode.h"
#include "taiga/settings.h"
#include "track/media.h"

enum StreamingVideoProvider {
  kStreamUnknown = -1,
  kStreamFirst,
  kStreamAnimelab = kStreamFirst,
  kStreamAnn,
  kStreamCrunchyroll,
  kStreamDaisuki,
  kStreamVeoh,
  kStreamViz,
  kStreamWakanim,
  kStreamYoutube,
  kStreamLast
};

enum WebBrowserEngine {
  kWebEngineUnknown = -1,
  kWebEngineWebkit,   // Google Chrome (and other browsers based on Chromium)
  kWebEngineGecko,    // Mozilla Firefox
  kWebEngineTrident,  // Internet Explorer
  kWebEnginePresto    // Opera (older versions)
};

class BrowserAccessibilityData {
public:
  BrowserAccessibilityData(const std::wstring& name, DWORD role);

  std::wstring name;
  DWORD role;
};

std::map<WebBrowserEngine, std::vector<BrowserAccessibilityData>> browser_data;

////////////////////////////////////////////////////////////////////////////////

BrowserAccessibilityData::BrowserAccessibilityData(const std::wstring& name,
                                                   DWORD role)
    : name(name), role(role) {
}

void AddBrowserData(WebBrowserEngine browser,
                    const std::wstring& name, DWORD role) {
  browser_data[browser].push_back(BrowserAccessibilityData(name, role));
}

void InitializeBrowserData() {
  if (!browser_data.empty())
    return;

  AddBrowserData(kWebEngineWebkit,
      L"Address and search bar", ROLE_SYSTEM_TEXT);
  AddBrowserData(kWebEngineWebkit,
      L"Address and search bar", ROLE_SYSTEM_GROUPING);
  AddBrowserData(kWebEngineWebkit,
      L"Address", ROLE_SYSTEM_GROUPING);
  AddBrowserData(kWebEngineWebkit,
      L"Location", ROLE_SYSTEM_GROUPING);
  AddBrowserData(kWebEngineWebkit,
      L"Address field", ROLE_SYSTEM_TEXT);

  AddBrowserData(kWebEngineGecko,
      L"Search or enter address", ROLE_SYSTEM_TEXT);
  AddBrowserData(kWebEngineGecko,
      L"Go to a Website", ROLE_SYSTEM_TEXT);
  AddBrowserData(kWebEngineGecko,
      L"Go to a Web Site", ROLE_SYSTEM_TEXT);

  AddBrowserData(kWebEngineTrident,
      L"Address and search using Bing", ROLE_SYSTEM_TEXT);
  AddBrowserData(kWebEngineTrident,
      L"Address and search using Google", ROLE_SYSTEM_TEXT);
}

////////////////////////////////////////////////////////////////////////////////

base::AccessibleChild* FindAccessibleChild(
    std::vector<base::AccessibleChild>& children,
    const std::wstring& name,
    DWORD role) {
  base::AccessibleChild* child = nullptr;

  foreach_(it, children) {
    if (name.empty() || IsEqual(name, it->name))
      if (!role || role == it->role)
        child = &(*it);
    if (child == nullptr && !it->children.empty())
      child = FindAccessibleChild(it->children, name, role);
    if (child)
      break;
  }

  return child;
}

bool MediaPlayers::BrowserAccessibleObject::AllowChildTraverse(
    base::AccessibleChild& child,
    LPARAM param) {
  switch (param) {
    case kWebEngineUnknown:
      return false;

    case kWebEngineWebkit:
      switch (child.role) {
        case ROLE_SYSTEM_CLIENT:
        case ROLE_SYSTEM_GROUPING:
        case ROLE_SYSTEM_PAGETABLIST:
        case ROLE_SYSTEM_TEXT:
        case ROLE_SYSTEM_TOOLBAR:
        case ROLE_SYSTEM_WINDOW:
          return true;
        default:
          return false;
      }
      break;

    case kWebEngineGecko:
      switch (child.role) {
        case ROLE_SYSTEM_APPLICATION:
        case ROLE_SYSTEM_COMBOBOX:
        case ROLE_SYSTEM_PAGETABLIST:
        case ROLE_SYSTEM_TOOLBAR:
          return true;
        case ROLE_SYSTEM_DOCUMENT:
        default:
          return false;
      }
      break;

    case kWebEngineTrident:
      switch (child.role) {
        case ROLE_SYSTEM_PANE:
        case ROLE_SYSTEM_SCROLLBAR:
          return false;
      }
      break;

    case kWebEnginePresto:
      switch (child.role) {
        case ROLE_SYSTEM_DOCUMENT:
        case ROLE_SYSTEM_PANE:
          return false;
      }
      break;
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

std::wstring MediaPlayers::GetTitleFromBrowser(HWND hwnd) {
  WebBrowserEngine web_engine = kWebEngineUnknown;

  auto media_player = FindPlayer(current_player());

  // Get window title
  std::wstring title = GetWindowTitle(hwnd);
  EditTitle(title, media_player);

  // Return current title if the same web page is still open
  if (CurrentEpisode.anime_id > 0)
    if (IntersectsWith(title, current_title()))
      return current_title();

  // Delay operation to save some CPU
  static int counter = 0;
  if (counter < 5) {
    counter++;
    return current_title();
  } else {
    counter = 0;
  }

  // Select web browser engine
  if (media_player->engine == L"WebKit") {
    web_engine = kWebEngineWebkit;
  } else if (media_player->engine == L"Gecko") {
    web_engine = kWebEngineGecko;
  } else if (media_player->engine == L"Trident") {
    web_engine = kWebEngineTrident;
  } else if (media_player->engine == L"Presto") {
    web_engine = kWebEnginePresto;
  } else {
    return std::wstring();
  }

  // Build accessibility data
  acc_obj.children.clear();
  if (acc_obj.FromWindow(hwnd) == S_OK) {
    acc_obj.BuildChildren(acc_obj.children, nullptr, web_engine);
    acc_obj.Release();
  }

  // Check other tabs
  if (CurrentEpisode.anime_id > 0) {
    base::AccessibleChild* child = nullptr;
    switch (web_engine) {
      case kWebEngineWebkit:
      case kWebEngineGecko:
        child = FindAccessibleChild(acc_obj.children,
                                    L"", ROLE_SYSTEM_PAGETABLIST);
        break;
      case kWebEngineTrident:
        child = FindAccessibleChild(acc_obj.children,
                                    L"Tab Row", 0);
        break;
      case kWebEnginePresto:
        child = FindAccessibleChild(acc_obj.children,
                                    L"", ROLE_SYSTEM_CLIENT);
        break;
    }
    if (child) {
      foreach_(it, child->children) {
        if (IntersectsWith(it->name, current_title())) {
          // Tab is still open, just not active
          return current_title();
        }
      }
    }
    // Tab is closed
    return std::wstring();
  }

  // Find URL
  base::AccessibleChild* child = nullptr;
  switch (web_engine) {
    case kWebEngineWebkit:
    case kWebEngineGecko:
    case kWebEngineTrident: {
      InitializeBrowserData();
      auto& child_data = browser_data[web_engine];
      foreach_(it, child_data) {
        child = FindAccessibleChild(acc_obj.children, it->name, it->role);
        if (child)
          break;
      }
      break;
    }
    case kWebEnginePresto:
      child = FindAccessibleChild(acc_obj.children,
                                  L"", ROLE_SYSTEM_CLIENT);
      if (child && !child->children.empty()) {
        child = FindAccessibleChild(child->children.at(0).children,
                                    L"", ROLE_SYSTEM_TOOLBAR);
        if (child && !child->children.empty()) {
          child = FindAccessibleChild(child->children,
                                      L"", ROLE_SYSTEM_COMBOBOX);
          if (child && !child->children.empty()) {
            child = FindAccessibleChild(child->children,
                                        L"", ROLE_SYSTEM_TEXT);
          }
        }
      }
      break;
  }

  if (child) {
    title = GetTitleFromStreamingMediaProvider(child->value, title);
  } else {
    title.clear();
  }

  return title;
}

bool IsStreamSettingEnabled(StreamingVideoProvider stream_provider) {
  switch (stream_provider) {
    case kStreamAnimelab:
      return Settings.GetBool(taiga::kStream_Animelab);
    case kStreamAnn:
      return Settings.GetBool(taiga::kStream_Ann);
    case kStreamCrunchyroll:
      return Settings.GetBool(taiga::kStream_Crunchyroll);
    case kStreamDaisuki:
      return Settings.GetBool(taiga::kStream_Daisuki);
    case kStreamVeoh:
      return Settings.GetBool(taiga::kStream_Veoh);
    case kStreamViz:
      return Settings.GetBool(taiga::kStream_Viz);
    case kStreamWakanim:
      return Settings.GetBool(taiga::kStream_Wakanim);
    case kStreamYoutube:
      return Settings.GetBool(taiga::kStream_Youtube);
  }

  return false;
}

bool MatchStreamUrl(StreamingVideoProvider stream_provider,
                    const std::wstring& url) {
  switch (stream_provider) {
    case kStreamAnimelab:
      return InStr(url, L"animelab.com/player/") > -1;
    case kStreamAnn:
      return SearchRegex(url, L"animenewsnetwork.com/video/[0-9]+");
    case kStreamCrunchyroll:
      return SearchRegex(url, L"crunchyroll\\.[a-z.]+/[^/]+/episode-[0-9]+.*-[0-9]+") ||
             SearchRegex(url, L"crunchyroll\\.[a-z.]+/[^/]+/.*-movie-[0-9]+");
    case kStreamDaisuki:
      return InStr(url, L"daisuki.net/anime/watch/") > -1;
    case kStreamVeoh:
      return InStr(url, L"veoh.com/watch/") > -1;
    case kStreamViz:
      return SearchRegex(url, L"viz.com/anime/streaming/[^/]+-episode-[0-9]+/") ||
             SearchRegex(url, L"viz.com/anime/streaming/[^/]+-movie/");
    case kStreamWakanim:
      return SearchRegex(url, L"wakanim\\.tv/video(-premium)?/[^/]+/");
    case kStreamYoutube:
      return InStr(url, L"youtube.com/watch") > -1;
  }

  return false;
}

void CleanStreamTitle(StreamingVideoProvider stream_provider,
                      std::wstring& title) {
  switch (stream_provider) {
    // AnimeLab
    case kStreamAnimelab:
      EraseLeft(title, L"AnimeLab - ");
      break;
    // Anime News Network
    case kStreamAnn:
      EraseRight(title, L" - Anime News Network");
      Erase(title, L" (s)");
      Erase(title, L" (d)");
      break;
    // Crunchyroll
    case kStreamCrunchyroll:
      EraseLeft(title, L"Crunchyroll - Watch ");
      EraseRight(title, L" - Movie - Movie");
      break;
    // DAISUKI
    case kStreamDaisuki: {
      EraseRight(title, L" - DAISUKI");
      auto pos = title.rfind(L" - ");
      if (pos != title.npos) {
        title = title.substr(pos + 3) + L" - " + title.substr(1, pos);
      } else {
        title.clear();
      }
      break;
    }
    // Veoh
    case kStreamVeoh:
      EraseLeft(title, L"Watch Videos Online | ");
      EraseRight(title, L" | Veoh.com");
      break;
    // Viz Anime
    case kStreamViz:
      EraseLeft(title, L"VIZ.com - NEON ALLEY - ");
      break;
    // Wakanim
    case kStreamWakanim:
      EraseRight(title, L" - Wakanim.TV");
      EraseRight(title, L" / Streaming");
      ReplaceString(title, 0, L" de ", L" ", false, false);
      ReplaceString(title, 0, L" en VOSTFR", L" VOSTFR", false, false);
      break;
    // YouTube
    case kStreamYoutube:
      EraseLeft(title, L"\u25B6 ");  // black right pointing triangle
      EraseRight(title, L" - YouTube");
      break;
    // Some other website, or URL is not found
    default:
    case kStreamUnknown:
      title.clear();
      break;
  }
}

std::wstring MediaPlayers::GetTitleFromStreamingMediaProvider(
    const std::wstring& url,
    std::wstring& title) {
  StreamingVideoProvider stream_provider = kStreamUnknown;

  // Check URL for known streaming video providers
  if (!url.empty()) {
    for (int i = kStreamFirst; i < kStreamLast; i++) {
      auto stream = static_cast<StreamingVideoProvider>(i);
      if (IsStreamSettingEnabled(stream)) {
        if (MatchStreamUrl(stream, url)) {
          stream_provider = stream;
          break;
        }
      }
    }
  }

  // Clean-up title
  CleanStreamTitle(stream_provider, title);

  return title;
}