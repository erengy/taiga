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

#include "taiga/settings.h"

#include "base/log.h"
#include "base/string.h"
#include "taiga/version.h"

namespace taiga {

// @TODO
bool AppSettings::HandleCompatibility() {
  return false;

  /*
  const semaver::Version version(WstrToStr(GetWstr(kMeta_Version)));

  if (version == taiga::version())
    return false;

  LOGW(L"Upgraded from v{} to v{}", StrToWstr(version.to_string()),
       StrToWstr(taiga::version().to_string()));

  if (version <= semaver::Version(1, 3, 0)) {
    // Set title language preference
    if (GetWstr(kApp_List_TitleLanguagePreference).empty()) {
      if (GetBool(kApp_List_DisplayEnglishTitles)) {
        Set(kApp_List_TitleLanguagePreference, L"english");
      } else {
        Set(kApp_List_TitleLanguagePreference, L"romaji");
      }
    }
  }
  */

  return true;
}

}  // namespace taiga
