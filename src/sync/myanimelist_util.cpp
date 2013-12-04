/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#include "myanimelist_util.h"

#include "base/file.h"
#include "base/string.h"
#include "base/time.h"
#include "taiga/settings.h"

namespace sync {
namespace myanimelist {

wstring DecodeText(wstring text) {
  Replace(text, L"<br />", L"\r", true);
  Replace(text, L"\n\n", L"\r\n\r\n", true);
  
  StripHtmlTags(text);
  DecodeHtmlEntities(text);
  
  return text;
}

// =============================================================================

void ViewAnimePage(int anime_id) {
  ExecuteLink(L"http://myanimelist.net/anime/" + ToWstr(anime_id) + L"/");
}

void ViewAnimeSearch(const wstring& title) {
  ExecuteLink(L"http://myanimelist.net/anime.php?q=" + title);
}

void ViewHistory() {
  ExecuteLink(L"http://myanimelist.net/history/" + Settings.Account.MAL.user);
}

void ViewMessages() {
  ExecuteLink(L"http://myanimelist.net/mymessages.php");
}

void ViewPanel() {
  ExecuteLink(L"http://myanimelist.net/panel.php");
}

void ViewProfile() {
  ExecuteLink(L"http://myanimelist.net/profile/" + Settings.Account.MAL.user);
}

void ViewSeasonGroup() {
  ExecuteLink(L"http://myanimelist.net/clubs.php?cid=743");
}

void ViewUpcomingAnime() {
  Date date = GetDate();

  ExecuteLink(L"http://myanimelist.net/anime.php"
              L"?sd=" + ToWstr(date.day) + 
              L"&sm=" + ToWstr(date.month) + 
              L"&sy=" + ToWstr(date.year) + 
              L"&em=0&ed=0&ey=0&o=2&w=&c[]=a&c[]=d&cv=1");
}

}  // namespace myanimelist
}  // namespace sync