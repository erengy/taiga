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

#include <cmath>

#include "base/file.h"
#include "base/gfx.h"
#include "base/string.h"
#include "taiga/orange.h"
#include "taiga/resource.h"
#include "taiga/stats.h"
#include "taiga/taiga.h"
#include "ui/dlg/dlg_about.h"

namespace ui {

class AboutDialog DlgAbout;

AboutDialog::AboutDialog() {
  RegisterDlgClass(L"TaigaAboutW");
}

BOOL AboutDialog::OnDestroy() {
  taiga::orange.Stop();

  return TRUE;
}

BOOL AboutDialog::OnInitDialog() {
  rich_edit_.Attach(GetDlgItem(IDC_RICHEDIT_ABOUT));
  auto schemes = L"http:https:irc:";
  rich_edit_.SendMessage(EM_AUTOURLDETECT, TRUE /*= AURL_ENABLEURL*/,
                         reinterpret_cast<LPARAM>(schemes));
  rich_edit_.SetEventMask(ENM_LINK);

  std::wstring text =
      L"{\\rtf1\\ansi\\deff0\\deflang1024"
      L"{\\fonttbl"
      L"{\\f0\\fnil\\fcharset0 Segoe UI;}"
      L"}"
      L"\\fs24\\b " TAIGA_APP_NAME L"\\b0  " + std::wstring(Taiga.version) + L"\\line\\fs18\\par "
      L"\\b Author:\\b0\\line "
      L"Eren 'erengy' Okka\\line\\par "
      L"\\b Contributors:\\b0\\line "
      L"saka, Diablofan, slevir, LordGravewish, cassist, rr-, sunjayc\\line\\par "
      L"\\b Third-party components:\\b0\\line "
      L"\u2022 Fugue Icons 3.4.5, Copyright (c) 2012, Yusuke Kamiyamane\\line "
      L"\u2022 JsonCpp 1.1.0, Copyright (c) 2007-2010, Baptiste Lepilleur\\line "
      L"\u2022 libcurl 7.40.0, Copyright (c) 1996-2015, Daniel Stenberg\\line "
      L"\u2022 libmojibake 1.1.6, Copyright (c) 2009 Public Software Group e. V.\\line "
      L"\u2022 pugixml 1.5, Copyright (c) 2006-2014, Arseny Kapoulkine\\line "
      L"\u2022 zlib 1.2.8, Copyright (c) 1995-2013, Jean-loup Gailly and Mark Adler\\line\\par "
      L"\\b Links:\\b0\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"http://taiga.erengy.com\"}}{\\fldrslt{Home page}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://github.com/erengy/taiga\"}}{\\fldrslt{GitHub repository}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://forums.hummingbird.me/t/taiga/10565\"}}{\\fldrslt{Hummingbird thread}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"http://myanimelist.net/clubs.php?cid=21400\"}}{\\fldrslt{MyAnimeList club}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://twitter.com/taigaapp\"}}{\\fldrslt{Twitter account}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"irc://irc.rizon.net/taiga\"}}{\\fldrslt{IRC channel}}}"
      L"}";
  rich_edit_.SetTextEx(WstrToStr(text));

  return TRUE;
}

BOOL AboutDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_COMMAND: {
      // Icon click
      if (HIWORD(wParam) == STN_DBLCLK) {
        SetText(L"Orange");
        Stats.tigers_harmed++;
        taiga::orange.Start();
        return TRUE;
      }
      break;
    }

    case WM_NOTIFY: {
      switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
        // Execute link
        case EN_LINK: {
          auto en_link = reinterpret_cast<ENLINK*>(lParam);
          if (en_link->msg == WM_LBUTTONUP) {
            ExecuteLink(rich_edit_.GetTextRange(&en_link->chrg));
            return TRUE;
          }
          break;
        }
      }
      break;
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void AboutDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  win::Dc dc = hdc;
  win::Rect rect;

  // Paint background
  GetClientRect(&rect);
  rect.left = ScaleX(static_cast<int>(48 * 1.5f));
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));
}

}  // namespace ui