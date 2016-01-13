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

#include <curl/curlver.h>
#include <jsoncpp/json/json.h>
#include <pugixml/pugixml.hpp>
#include <utf8proc/utf8proc.h>
#include <zlib/zlib.h>

#include "base/file.h"
#include "base/gfx.h"
#include "base/string.h"
#include "taiga/orange.h"
#include "taiga/resource.h"
#include "taiga/stats.h"
#include "taiga/taiga.h"
#include "ui/dlg/dlg_about.h"

namespace ui {

enum ThirdPartyLibrary {
  kJsoncpp,
  kLibcurl,
  kPugixml,
  kUtf8proc,
  kZlib,
};

static std::wstring GetLibraryVersion(ThirdPartyLibrary library) {
  switch (library) {
    case kJsoncpp:
      return StrToWstr(JSONCPP_VERSION_STRING);
    case kLibcurl:
      return StrToWstr(LIBCURL_VERSION);
    case kPugixml: {
      base::SemanticVersion version((PUGIXML_VERSION / 100),
                                    (PUGIXML_VERSION % 100) / 10,
                                    (PUGIXML_VERSION % 100) % 10);
      return version;
    }
    case kUtf8proc:
      return StrToWstr(utf8proc_version());
    case kZlib:
      return StrToWstr(ZLIB_VERSION);
      break;
  }

  return std::wstring();
}

////////////////////////////////////////////////////////////////////////////////

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
      L"saka, Diablofan, slevir, LordGravewish, cassist, rr-, sunjayc, LordHaruto, Keelhauled,\\line "
      L"thesethwalker, Soinou, menma1234, KazukiMutou, vipirius, Kokoro-chan, snowfag,\\line "
      L"such-doge\\line\\par "
      L"\\b Third-party components:\\b0\\line "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/yusukekamiyamane/fugue-icons\"}}{\\fldrslt{Fugue Icons 3.4.5}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/open-source-parsers/jsoncpp\"}}{\\fldrslt{JsonCpp " + GetLibraryVersion(kJsoncpp) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/bagder/curl\"}}{\\fldrslt{libcurl " + GetLibraryVersion(kLibcurl) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/zeux/pugixml\"}}{\\fldrslt{pugixml " + GetLibraryVersion(kPugixml) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/JuliaLang/utf8proc\"}}{\\fldrslt{utf8proc " + GetLibraryVersion(kUtf8proc) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/madler/zlib\"}}{\\fldrslt{zlib " + GetLibraryVersion(kZlib) + L"}}}\\line\\par "
      L"\\b Links:\\b0\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"http://taiga.moe\"}}{\\fldrslt{Home page}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://github.com/erengy/taiga\"}}{\\fldrslt{GitHub repository}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://hummingbird.me/groups/taiga\"}}{\\fldrslt{Hummingbird group}}}\\line "
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

  win::Rect rect_edit;
  rich_edit_.GetWindowRect(GetWindowHandle(), &rect_edit);

  const int margin = rect_edit.top;
  const int sidebar_width = rect_edit.left - margin;

  // Paint background
  GetClientRect(&rect);
  rect.left = sidebar_width;
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));

  // Paint application icon
  rect.Set(margin / 2, margin, sidebar_width - (margin / 2), rect.bottom);
  DrawIconResource(IDI_MAIN, dc.Get(), rect, true, false);
  win::Window label = GetDlgItem(IDC_STATIC_APP_ICON);
  label.SetPosition(nullptr, rect,
      SWP_NOACTIVATE | SWP_NOREDRAW | SWP_NOOWNERZORDER | SWP_NOZORDER);
  label.SetWindowHandle(nullptr);
}

}  // namespace ui