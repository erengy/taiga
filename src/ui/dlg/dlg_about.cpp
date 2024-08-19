/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <pugixml.hpp>
#include <rapidjson/rapidjson.h>
#include <utf8proc/utf8proc.h>
#include <zlib/zlib.h>

#include "base/file.h"
#include "base/gfx.h"
#include "base/string.h"
#include "taiga/config.h"
#include "taiga/orange.h"
#include "taiga/resource.h"
#include "taiga/stats.h"
#include "taiga/version.h"
#include "ui/dlg/dlg_about.h"

namespace ui {

enum ThirdPartyLibrary {
  kDate,
  kDiscordRpc,
  kFmt,
  kJson,
  kLibcurl,
  kPugixml,
  kRandom,
  kRapidJson,
  kUtf8proc,
  kZlib,
};

static std::wstring GetLibraryVersion(ThirdPartyLibrary library) {
  switch (library) {
    case kDate:
      return L"3.0.1";
    case kDiscordRpc:
      return L"3.4.0";
    case kFmt:
      return StrToWstr(semaver::Version(
          (FMT_VERSION / 10000),
          (FMT_VERSION % 10000) / 100,
          (FMT_VERSION % 10000) % 100).to_string());
    case kJson:
      return StrToWstr(semaver::Version(
          NLOHMANN_JSON_VERSION_MAJOR,
          NLOHMANN_JSON_VERSION_MINOR,
          NLOHMANN_JSON_VERSION_PATCH).to_string());
    case kLibcurl:
      return StrToWstr(semaver::Version(
          LIBCURL_VERSION_MAJOR,
          LIBCURL_VERSION_MINOR,
          LIBCURL_VERSION_PATCH).to_string());
    case kPugixml:
      return StrToWstr(semaver::Version(
          (PUGIXML_VERSION / 1000),
          (PUGIXML_VERSION % 1000) / 10,
          (PUGIXML_VERSION % 1000) % 10).to_string());
    case kRandom:
      return L"1.4.0";
    case kRapidJson:
      return StrToWstr(semaver::Version(
          RAPIDJSON_MAJOR_VERSION,
          RAPIDJSON_MINOR_VERSION,
          RAPIDJSON_PATCH_VERSION).to_string());
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
      L"\\fs24\\b " TAIGA_APP_NAME L"\\b0  " + StrToWstr(taiga::version().to_string()) + L"\\line\\fs18\\par "
      L"\\b Author:\\b0\\line "
      L"erengy (Eren Okka)\\line\\par "
      L"\\b Contributors:\\b0\\line "
      L"saka, Diablofan, slevir, LordGravewish, rr-, sunjayc, ConnorKrammer, Soinou, Jiyuu, ryban, tollyx,\\line "
      L"pavelxdd, gunt3001, synthtech, cnguy, CeruleanSky, Xabis, rzumer, Juplay, SacredZenpie\\line\\par "
      L"\\b Donators:\\b0\\line "
      L"Farfie, snickler, Nydaleclya, WizardTim, Kinzer, MeGaNeKo, WhatsCPS, Jerico64 and other anonymous supporters\\line\\par "
      L"\\b Third-party components:\\b0\\line "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/HowardHinnant/date\"}}{\\fldrslt{date " + GetLibraryVersion(kDate) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/discordapp/discord-rpc\"}}{\\fldrslt{Discord RPC " + GetLibraryVersion(kDiscordRpc) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/fmtlib/fmt\"}}{\\fldrslt{fmt " + GetLibraryVersion(kFmt) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://p.yusukekamiyamane.com/icons/search/fugue/\"}}{\\fldrslt{Fugue Icons 3.4.5}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/nlohmann/json\"}}{\\fldrslt{JSON for Modern C++ " + GetLibraryVersion(kJson) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/curl/curl\"}}{\\fldrslt{libcurl " + GetLibraryVersion(kLibcurl) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/zeux/pugixml\"}}{\\fldrslt{pugixml " + GetLibraryVersion(kPugixml) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/effolkronium/random\"}}{\\fldrslt{Random " + GetLibraryVersion(kRandom) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/Tencent/rapidjson\"}}{\\fldrslt{RapidJSON " + GetLibraryVersion(kRapidJson) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/JuliaLang/utf8proc\"}}{\\fldrslt{utf8proc " + GetLibraryVersion(kUtf8proc) + L"}}}, "
      L"{\\field{\\*\\fldinst{HYPERLINK \"https://github.com/madler/zlib\"}}{\\fldrslt{zlib " + GetLibraryVersion(kZlib) + L"}}}\\line\\par "
      L"\\b Links:\\b0\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://taiga.moe\"}}{\\fldrslt{Home page}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://github.com/erengy/taiga\"}}{\\fldrslt{GitHub repository}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://myanimelist.net/clubs.php?cid=21400\"}}{\\fldrslt{MyAnimeList club}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://kitsu.app/groups/taiga\"}}{\\fldrslt{Kitsu group}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://anilist.co/forum/thread/2846/1\"}}{\\fldrslt{AniList thread}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://discord.gg/yeGNktZ\"}}{\\fldrslt{Discord server}}}\\line "
      L"\u2022 {\\field{\\*\\fldinst{HYPERLINK \"https://twitter.com/taigaapp\"}}{\\fldrslt{Twitter account}}}"
      L"}";
  rich_edit_.SetTextEx(WstrToStr(text));

  return TRUE;
}

INT_PTR AboutDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_COMMAND: {
      // Icon click
      if (HIWORD(wParam) == STN_DBLCLK) {
        SetText(L"Orange");
        taiga::stats.tigers_harmed++;
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
