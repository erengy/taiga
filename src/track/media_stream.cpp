/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include <anisthesia/src/win/strategy/ui_automation.h>

#include "base/process.h"
#include "base/string.h"
#include "library/anime_episode.h"
#include "taiga/settings.h"
#include "track/media.h"

enum class Stream {
  Unknown,
  Animelab,
  Ann,
  Crunchyroll,
  Daisuki,
  Hidive,
  Plex,
  Veoh,
  Viz,
  Vrv,
  Wakanim,
  Youtube,
};

////////////////////////////////////////////////////////////////////////////////

std::wstring MediaPlayers::GetTitleFromBrowser(
    HWND hwnd, const MediaPlayer& media_player) {
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

  anisthesia::win::WebBrowser browser;
  if (!anisthesia::win::GetWebBrowserInformation(hwnd, browser))
    return std::wstring();

  if (CurrentEpisode.anime_id > 0) {
    for (const auto& tab : browser.tabs) {
      if (IntersectsWith(tab, current_title()))
        return current_title();  // Tab is still open, just not active
    }
    return std::wstring();
  }

  return GetTitleFromStreamingMediaProvider(browser.address, title);
}

bool IsStreamSettingEnabled(Stream stream) {
  switch (stream) {
    case Stream::Animelab:
      return Settings.GetBool(taiga::kStream_Animelab);
    case Stream::Ann:
      return Settings.GetBool(taiga::kStream_Ann);
    case Stream::Crunchyroll:
      return Settings.GetBool(taiga::kStream_Crunchyroll);
    case Stream::Daisuki:
      return Settings.GetBool(taiga::kStream_Daisuki);
    case Stream::Hidive:
      return Settings.GetBool(taiga::kStream_Hidive);
    case Stream::Plex:
      return Settings.GetBool(taiga::kStream_Plex);
    case Stream::Veoh:
      return Settings.GetBool(taiga::kStream_Veoh);
    case Stream::Viz:
      return Settings.GetBool(taiga::kStream_Viz);
    case Stream::Vrv:
      return Settings.GetBool(taiga::kStream_Vrv);
    case Stream::Wakanim:
      return Settings.GetBool(taiga::kStream_Wakanim);
    case Stream::Youtube:
      return Settings.GetBool(taiga::kStream_Youtube);
  }

  return false;
}

Stream FindStreamFromUrl(const std::wstring& url) {
  if (url.empty())
    return Stream::Unknown;

  if (InStr(url, L"animelab.com/player/") > -1)
    return Stream::Animelab;

  if (SearchRegex(url, L"animenewsnetwork.com/video/[0-9]+"))
    return Stream::Ann;

  if (SearchRegex(url, L"crunchyroll\\.[a-z.]+/[^/]+/episode-[0-9]+.*-[0-9]+") ||
      SearchRegex(url, L"crunchyroll\\.[a-z.]+/[^/]+/.*-movie-[0-9]+"))
    return Stream::Crunchyroll;

  if (SearchRegex(url, L"daisuki\\.net/[a-z]+/[a-z]+/anime/watch"))
    return Stream::Daisuki;

  if (InStr(url, L"hidive.com/stream/") > -1)
    return Stream::Hidive;

  if (InStr(url, L"plex.tv/web/") > -1 ||
      InStr(url, L"localhost:32400/web/") > -1 ||
      SearchRegex(url, L"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}:32400/web/") ||
      SearchRegex(url, L"plex\\.[a-z0-9-]+\\.[a-z0-9-]+") ||
      SearchRegex(url, L"[a-z0-9-]+\\.[a-z0-9-]+/plex"))
    return Stream::Plex;

  if (InStr(url, L"veoh.com/watch/") > -1)
    return Stream::Veoh;

  if (SearchRegex(url, L"viz.com/watch/streaming/[^/]+-episode-[0-9]+/") ||
      SearchRegex(url, L"viz.com/watch/streaming/[^/]+-movie/"))
    return Stream::Viz;

  if (InStr(url, L"vrv.co/watch") > -1)
    return Stream::Vrv;

  if (SearchRegex(url, L"wakanim\\.tv/video(-premium)?/[^/]+/"))
    return Stream::Wakanim;

  if (InStr(url, L"youtube.com/watch") > -1)
    return Stream::Youtube;

  return Stream::Unknown;
}

void CleanStreamTitle(Stream stream, std::wstring& title) {
  switch (stream) {
    // AnimeLab
    case Stream::Animelab:
      EraseLeft(title, L"AnimeLab - ");
      break;
    // Anime News Network
    case Stream::Ann:
      EraseRight(title, L" - Anime News Network");
      Erase(title, L" (s)");
      Erase(title, L" (d)");
      break;
    // Crunchyroll
    case Stream::Crunchyroll:
      EraseLeft(title, L"Crunchyroll - Watch ");
      EraseRight(title, L" - Movie - Movie");
      break;
    // DAISUKI
    case Stream::Daisuki: {
      EraseRight(title, L" - DAISUKI");
      auto pos = title.rfind(L" - ");
      if (pos != title.npos) {
        title = title.substr(pos + 3) + L" - " + title.substr(1, pos);
      } else {
        title.clear();
      }
      break;
    }
    // HIDIVE
    case Stream::Hidive: {
      break;
    }
    // Plex
    case Stream::Plex: {
      EraseLeft(title, L"Plex");
      EraseLeft(title, L"\u25B6 ");  // black right pointing triangle
      ReplaceString(title, L" \u00B7 ", L"");
      break;
    }
    // Veoh
    case Stream::Veoh:
      EraseLeft(title, L"Watch Videos Online | ");
      EraseRight(title, L" | Veoh.com");
      break;
    // Viz Anime
    case Stream::Viz:
      EraseRight(title, L" // VIZ");
      break;
    // VRV
    case Stream::Vrv:
      EraseLeft(title, L"VRV - Watch ");
      ReplaceString(title, 0, L": EP ", L" - EP ", false, false);
      break;
    // Wakanim
    case Stream::Wakanim:
      EraseRight(title, L" - Wakanim.TV");
      EraseRight(title, L" / Streaming");
      ReplaceString(title, 0, L" de ", L" ", false, false);
      ReplaceString(title, 0, L" en VOSTFR", L" VOSTFR", false, false);
      break;
    // YouTube
    case Stream::Youtube:
      EraseLeft(title, L"\u25B6 ");  // black right pointing triangle
      EraseRight(title, L" - YouTube");
      break;
    // Some other website, or URL is not found
    default:
    case Stream::Unknown:
      title.clear();
      break;
  }
}

std::wstring MediaPlayers::GetTitleFromStreamingMediaProvider(
    const std::wstring& url,
    std::wstring& title) {
  const auto stream = FindStreamFromUrl(url);
  if (IsStreamSettingEnabled(stream)) {
    EraseLeft(title, L"New Tab");
    CleanStreamTitle(stream, title);
  } else {
    title.clear();
  }

  return title;
}
