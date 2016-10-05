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
#include "win/win_automation.h"

enum StreamingVideoProvider {
  kStreamUnknown = -1,
  kStreamFirst,
  kStreamAnimelab = kStreamFirst,
  kStreamAnn,
  kStreamCrunchyroll,
  kStreamDaisuki,
  kStreamPlex,
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

win::AccessibleChild* FindAccessibleChild(
    std::vector<win::AccessibleChild>& children,
    const std::wstring& name,
    DWORD role) {
  win::AccessibleChild* child = nullptr;

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
    win::AccessibleChild& child,
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

std::wstring MediaPlayers::FromActiveAccessibility(HWND hwnd, int web_engine,
                                                   std::wstring& title) {
  // Build accessibility data
  acc_obj.children.clear();
  if (acc_obj.FromWindow(hwnd) == S_OK) {
    acc_obj.BuildChildren(acc_obj.children, nullptr, web_engine);
    acc_obj.Release();
  }

  // Check other tabs
  if (CurrentEpisode.anime_id > 0) {
    win::AccessibleChild* child = nullptr;
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

  // Find URL
  } else {
    win::AccessibleChild* child = nullptr;
    switch (web_engine) {
      case kWebEngineWebkit:
      case kWebEngineGecko:
      case kWebEngineTrident: {
        InitializeBrowserData();
        auto& child_data = browser_data[static_cast<WebBrowserEngine>(web_engine)];
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
  }

  return title;
}

std::wstring MediaPlayers::FromAutomationApi(HWND hwnd, int web_engine,
                                             std::wstring& title) {
  win::UIAutomation ui_automation;
  if (!ui_automation.Initialize())
    return std::wstring();

  // Find browser
  auto element_browser = ui_automation.ElementFromHandle(hwnd);
  if (!element_browser)
    return std::wstring();

  // Find tabs
  if (CurrentEpisode.anime_id > 0) {
    bool found_tab = false;
    auto element_tab = ui_automation.FindFirstControl(
        element_browser, UIA_TabControlTypeId, true);
    if (element_tab) {
      auto element_array_tab_items = ui_automation.FindAllControls(
          element_tab, UIA_TabItemControlTypeId, true);
      if (element_array_tab_items) {
        int tab_count = ui_automation.GetElementArrayLength(element_array_tab_items);
        for (int tab_index = 0; tab_index < tab_count; ++tab_index) {
          auto element_tab_item = ui_automation.GetElementFromArrayIndex(
              element_array_tab_items, tab_index);
          auto name = ui_automation.GetElementName(element_tab_item);
          if (name.empty()) {
            auto element_text = ui_automation.FindFirstControl(
                element_tab_item, UIA_TextControlTypeId, true);
            if (element_text) {
              name = ui_automation.GetElementName(element_text);
              element_text->Release();
            }
          }
          if (IntersectsWith(name, current_title())) {
            // Tab is still open, just not active
            title = current_title();
            found_tab = true;
            break;
          }
        }
        element_array_tab_items->Release();
      }
      element_tab->Release();
    }
    if (!found_tab)
      title.clear();
  
  // Find URL
  } else {
    auto element_url = ui_automation.FindFirstControl(
        element_browser, UIA_EditControlTypeId, false);
    if (element_url) {
      title = GetTitleFromStreamingMediaProvider(
          ui_automation.GetElementValue(element_url), title);
      element_url->Release();
    } else {
      title.clear();
    }
  }

  element_browser->Release();

  return title;
}

std::wstring MediaPlayers::GetTitleFromBrowser(
    HWND hwnd, const MediaPlayer& media_player) {
  WebBrowserEngine web_engine = kWebEngineUnknown;

  // Get window title
  std::wstring title = GetWindowTitle(hwnd);
  EditTitle(title, media_player);

  if (media_player.name == current_player_name()) {
    // Return current title if the same web page is still open
    if (CurrentEpisode.anime_id > 0)
      if (IntersectsWith(title, current_title()))
        return current_title();

    // Delay operation to save some CPU cycles
    static int counter = 0;
    if (++counter < 5) {
      return current_title();
    } else {
      counter = 0;
    }
  }

  // Select web browser engine
  if (media_player.engine == L"WebKit") {
    web_engine = kWebEngineWebkit;
  } else if (media_player.engine == L"Gecko") {
    web_engine = kWebEngineGecko;
  } else if (media_player.engine == L"Trident") {
    web_engine = kWebEngineTrident;
  } else if (media_player.engine == L"Presto") {
    web_engine = kWebEnginePresto;
  } else {
    return std::wstring();
  }

  if (Settings[taiga::kRecognition_BrowserDetectionMethod] == L"ui_automation") {
    return FromAutomationApi(hwnd, web_engine, title);
  } else {
    return FromActiveAccessibility(hwnd, web_engine, title);
  }
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
    case kStreamPlex:
      return Settings.GetBool(taiga::kStream_Plex);
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
      return SearchRegex(url, L"daisuki\\.net/[a-z]+/[a-z]+/anime/watch");
    case kStreamPlex:
      return SearchRegex(url,
          L"(?:(?:localhost|\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):32400|plex.tv)/web/");
    case kStreamVeoh:
      return InStr(url, L"veoh.com/watch/") > -1;
    case kStreamViz:
      return SearchRegex(url, L"viz.com/watch/streaming/[^/]+-episode-[0-9]+/") ||
             SearchRegex(url, L"viz.com/watch/streaming/[^/]+-movie/");
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
    // Plex
    case kStreamPlex: {
      EraseLeft(title, L"Plex");
      EraseLeft(title, L"\u25B6 ");  // black right pointing triangle
      ReplaceString(title, L" \u00B7 ", L"");
      break;
    }
    // Veoh
    case kStreamVeoh:
      EraseLeft(title, L"Watch Videos Online | ");
      EraseRight(title, L" | Veoh.com");
      break;
    // Viz Anime
    case kStreamViz:
      EraseRight(title, L" // VIZ");
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
  EraseLeft(title, L"New Tab");
  CleanStreamTitle(stream_provider, title);

  return title;
}