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

#include "debug.h"

#include "anime_db.h"
#include "common.h"
#include "myanimelist.h"
#include "string.h"

#include "dlg/dlg_main.h"

namespace debug {

// =============================================================================

Tester::Tester()
    : frequency_(0.0), value_(0) {
}

void Tester::Start() {
  LARGE_INTEGER li;
  
  if (frequency_ == 0.0) {
    ::QueryPerformanceFrequency(&li);
    frequency_ = double(li.QuadPart) / 1000.0;
  }
  
  ::QueryPerformanceCounter(&li);
  value_ = li.QuadPart;
}

void Tester::End(wstring str, bool display_result) {
  LARGE_INTEGER li;

  ::QueryPerformanceCounter(&li);
  double value = double(li.QuadPart - value_) / frequency_;

  if (display_result) {
    str = ToWstr(value, 2) + L"ms | Text: [" + str + L"]";
    MainDialog.SetText(str);
  }
}

// =============================================================================

void PrintBbcodeUrl(const anime::Item& anime_item) {
  Print(L"[url=http://myanimelist.net/anime/" + ToWstr(anime_item.GetId()) + L"/]" + 
    anime_item.GetTitle() + L"[/url]\n");
}

void CheckDoubleSpace() {
  for (auto item = AnimeDatabase.items.begin(); item != AnimeDatabase.items.end(); ++item) {
    if (InStr(item->second.GetTitle(), L"  ") > -1 ||
        InStr(Join(item->second.GetSynonyms(), L"; "), L"  ") > -1) {
      PrintBbcodeUrl(item->second);
    }
  }
}

void CheckInvalidDates() {
  for (auto item = AnimeDatabase.items.begin(); item != AnimeDatabase.items.end(); ++item) {
    if (item->second.GetAiringStatus(true) != item->second.GetAiringStatus(false)) {
      PrintBbcodeUrl(item->second);
    }
  }
}

void CheckInvalidEpisodes() {
  for (auto item = AnimeDatabase.items.begin(); item != AnimeDatabase.items.end(); ++item) {
    if (item->second.GetEpisodeCount() < 0 ||
        item->second.GetEpisodeCount() > 500 ||
        (item->second.IsInList() && item->second.GetMyLastWatchedEpisode() > item->second.GetEpisodeCount())) {
      PrintBbcodeUrl(item->second);
    }
  }
}

void CheckSynonyms() {
  for (auto item = AnimeDatabase.items.begin(); item != AnimeDatabase.items.end(); ++item) {
    for (auto synonym = item->second.GetSynonyms().begin(); synonym != item->second.GetSynonyms().end(); ++synonym) {
      if (InStr(*synonym, L";", 0, true) > -1) {
        PrintBbcodeUrl(item->second);
        break;
      }
    }
  }
}

// =============================================================================

void Print(wstring text) {
#ifdef _DEBUG
  ::OutputDebugString(text.c_str());
#else
  UNREFERENCED_PARAMETER(text);
#endif
}

void Test() {
  // Define variables
  wstring str;

  // Start ticking
  Tester test;
  test.Start();

  for (int i = 0; i < 10000; i++) {
    // Do some tests here
    //       ___
    //      {o,o}
    //      |)__)
    //     --"-"--
    //      O RLY?
  }

  // Debugging MAL database
  //CheckDoubleSpace();
  //CheckInvalidDates();
  //CheckInvalidEpisodes();
  //CheckSynonyms();

  // Debugging recognition engine
  //ExecuteAction(L"RecognitionTest");

  // Show result
  test.End(str, 0);
}

} // namespace debug