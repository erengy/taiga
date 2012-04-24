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

#include "../std.h"

#include "dlg_anime_info.h"
#include "dlg_anime_info_page.h"

#include "../anime.h"
#include "../anime_db.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"

// =============================================================================

AnimeInfoPage::AnimeInfoPage()
    : anime_id_(anime::ID_UNKNOWN), 
      header_font_(nullptr) {
}

AnimeInfoPage::~AnimeInfoPage() {
  if (header_font_) {
    ::DeleteObject(header_font_);
    header_font_ = nullptr;
  }
}

// =============================================================================

BOOL AnimeInfoPage::OnInitDialog() {
  // Create bold header font
  if (!header_font_) {
    LOGFONT lFont;
    ::GetObject(GetFont(), sizeof(LOGFONT), &lFont);
    lFont.lfCharSet = DEFAULT_CHARSET;
    lFont.lfWeight = FW_BOLD;
    header_font_ = ::CreateFontIndirect(&lFont);
  }
  // Set new font for headers
  for (int i = 0; i < 3; i++) {
    SendDlgItemMessage(IDC_STATIC_HEADER1 + i, WM_SETFONT, 
      reinterpret_cast<WPARAM>(header_font_), FALSE);
  }
  
  return TRUE;
}

BOOL AnimeInfoPage::OnCommand(WPARAM wParam, LPARAM lParam) {
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  switch (LOWORD(wParam)) {
    // User changed rewatching checkbox
    case IDC_CHECK_ANIME_REWATCH:
      if (HIWORD(wParam) == BN_CLICKED) {
        win32::ComboBox m_Combo = GetDlgItem(IDC_COMBO_ANIME_STATUS);
        win32::Spin m_Spin = GetDlgItem(IDC_SPIN_PROGRESS);
        int episode_value; m_Spin.GetPos32(episode_value);
        if (IsDlgButtonChecked(IDC_CHECK_ANIME_REWATCH)) {
          if (anime_item->GetMyStatus() == mal::MYSTATUS_COMPLETED && episode_value == anime_item->GetEpisodeCount()) {
            m_Spin.SetPos32(0);
          }
          m_Combo.Enable(FALSE);
          m_Combo.SetCurSel(mal::MYSTATUS_COMPLETED - 1);
        } else {
          if (episode_value == 0) {
            m_Spin.SetPos32(anime_item->GetMyLastWatchedEpisode());
          }
          m_Combo.Enable();
          m_Combo.SetCurSel(anime_item->GetMyStatus() - 1);
        }
        m_Spin.SetWindowHandle(nullptr);
        m_Combo.SetWindowHandle(nullptr);
        return TRUE;
      }
      break;

    // User changed status dropdown
    case IDC_COMBO_ANIME_STATUS:
      if (HIWORD(wParam) == CBN_SELENDOK) {
        // Selected "Completed"
        win32::ComboBox m_Combo = GetDlgItem(IDC_COMBO_ANIME_STATUS);
        if (m_Combo.GetItemData(m_Combo.GetCurSel()) == mal::MYSTATUS_COMPLETED) {
          if (anime_item->GetMyStatus() != mal::MYSTATUS_COMPLETED && anime_item->GetEpisodeCount() > 0) {
            SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETPOS32, 0, anime_item->GetEpisodeCount());
          }
        }
        m_Combo.SetWindowHandle(nullptr);
        return TRUE;
      }
      break;
  }
  
  return FALSE;
}

LRESULT AnimeInfoPage::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (pnmh->idFrom) {
    case IDC_LINK_ANIME_FANSUB:
      switch (pnmh->code) {
        case NM_CLICK: {
          // Set/change fansub group preference
          if (anime::SetFansubFilter(anime_id_, L"TaigaSubs (change this)")) {
            RefreshFansubPreference();
          }
          return TRUE;
        }
      }
  }

  return 0;
}

// =============================================================================

void AnimeInfoPage::Refresh(int anime_id) {
  if (anime_id <= anime::ID_UNKNOWN) return;

  anime_id_ = anime_id;
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  switch (index) {
    // Series information
    case INFOPAGE_SERIESINFO: {
      // Set synonyms
      wstring text = Join(anime_item->GetSynonyms(), L", ");
      if (text.empty()) text = L"-";
      SetDlgItemText(IDC_EDIT_ANIME_ALT, text.c_str());
      
      // Set synopsis
      if (anime_item->IsOldEnough() || anime_item->GetSynopsis().empty()) {
        if (mal::SearchAnime(anime_id_, anime_item->GetTitle())) {
          text = L"Retrieving...";
        } else {
          text = L"-";
        }
      } else {
        text = anime_item->GetSynopsis();
        if (anime_item->GetGenres().empty() || anime_item->GetScore().empty()) {
          mal::GetAnimeDetails(anime_id_);
        }
      }
      SetDlgItemText(IDC_EDIT_ANIME_INFO, text.c_str());
      
      // Set information
      text = mal::TranslateType(anime_item->GetType()) + L"\n" + 
             mal::TranslateNumber(anime_item->GetEpisodeCount(), L"Unknown") + L"\n" + 
             mal::TranslateStatus(anime_item->GetAiringStatus()) + L"\n" + 
             mal::TranslateDateToSeason(anime_item->GetDate(anime::DATE_START));
      SetDlgItemText(IDC_STATIC_ANIME_INFO1, text.c_str());
      #define ADD_INFOLINE(x, y) (x.empty() ? y : x)
      wstring genres = anime_item->GetGenres();
      LimitText(genres, 50); // TODO: Get rid of magic number
      text = ADD_INFOLINE(genres, L"-") + L"\n" +
             ADD_INFOLINE(anime_item->GetScore(), L"0.00") + L"\n" + 
             ADD_INFOLINE(anime_item->GetRank(), L"#0") + L"\n" + 
             ADD_INFOLINE(anime_item->GetPopularity(), L"#0");
      #undef ADD_INFOLINE
      SetDlgItemText(IDC_STATIC_ANIME_INFO2, text.c_str());
      break;
    }

    // =========================================================================

    // My information
    case INFOPAGE_MYINFO: {
      if (!anime_item->IsInList()) break;

      // Episodes watched
      SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETRANGE32, 0, 
        anime_item->GetEpisodeCount() > 0 ? anime_item->GetEpisodeCount() : 9999);
      SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETPOS32, 0, anime_item->GetMyLastWatchedEpisode());

      // Re-watching
      CheckDlgButton(IDC_CHECK_ANIME_REWATCH, anime_item->GetMyRewatching());
      EnableDlgItem(IDC_CHECK_ANIME_REWATCH, anime_item->GetMyStatus() == mal::MYSTATUS_COMPLETED);

      // Status
      win32::ComboBox m_Combo = GetDlgItem(IDC_COMBO_ANIME_STATUS);
      if (m_Combo.GetCount() == 0) {
        for (int i = mal::MYSTATUS_WATCHING; i <= mal::MYSTATUS_PLANTOWATCH; i++) {
          if (i != mal::MYSTATUS_UNKNOWN) {
            m_Combo.AddItem(mal::TranslateMyStatus(i, false).c_str(), i);
          }
        }
      }
      int status = anime_item->GetMyStatus();
      if (status == mal::MYSTATUS_PLANTOWATCH) status--;
      m_Combo.SetCurSel(status - 1);
      m_Combo.Enable(!anime_item->GetMyRewatching());
      m_Combo.SetWindowHandle(nullptr);

      // Score
      m_Combo.SetWindowHandle(GetDlgItem(IDC_COMBO_ANIME_SCORE));
      if (m_Combo.GetCount() == 0) {
        m_Combo.AddString(L"(10) Masterpiece");
        m_Combo.AddString(L"(9) Great");
        m_Combo.AddString(L"(8) Very Good");
        m_Combo.AddString(L"(7) Good");
        m_Combo.AddString(L"(6) Fine");
        m_Combo.AddString(L"(5) Average");
        m_Combo.AddString(L"(4) Bad");
        m_Combo.AddString(L"(3) Very Bad");
        m_Combo.AddString(L"(2) Horrible");
        m_Combo.AddString(L"(1) Unwatchable");
        m_Combo.AddString(L"(0) No Score");
      }
      m_Combo.SetCurSel(10 - anime_item->GetMyScore());
      m_Combo.SetWindowHandle(nullptr);

      // Tags
      win32::Edit m_Edit = GetDlgItem(IDC_EDIT_ANIME_TAGS);
      m_Edit.SetCueBannerText(L"Enter tags here, separated by a comma (e.g. tag1, tag2)");
      m_Edit.SetText(anime_item->GetMyTags());
      m_Edit.SetWindowHandle(nullptr);
      
      // Date limits and defaults
      if (mal::IsValidDate(anime_item->GetDate(anime::DATE_START))) {
        SYSTEMTIME stSeriesStart = anime_item->GetDate(anime::DATE_START);
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETRANGE, GDTR_MIN, (LPARAM)&stSeriesStart);
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stSeriesStart);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETRANGE, GDTR_MIN, (LPARAM)&stSeriesStart);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stSeriesStart);
      }
      if (mal::IsValidDate(anime_item->GetDate(anime::DATE_END))) {
        SYSTEMTIME stSeriesEnd = anime_item->GetDate(anime::DATE_END);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETRANGE, GDTR_MIN, (LPARAM)&stSeriesEnd);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stSeriesEnd);
      }
      // Start date
      if (mal::IsValidDate(anime_item->GetMyDate(anime::DATE_START))) {
        SYSTEMTIME stMyStart = anime_item->GetMyDate(anime::DATE_START);
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stMyStart);
      } else {
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_NONE, 0);
      }
      // Finish date
      if (mal::IsValidDate(anime_item->GetMyDate(anime::DATE_END))) {
        SYSTEMTIME stMyFinish = anime_item->GetMyDate(anime::DATE_END);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stMyFinish);
      } else {
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_NONE, 0);
      }

      // Alternative titles
      m_Edit.SetWindowHandle(GetDlgItem(IDC_EDIT_ANIME_ALT));
      m_Edit.SetCueBannerText(L"Enter alternative titles here, separated by a semicolon (e.g. Title 1; Title 2)");
      m_Edit.SetText(Join(anime_item->GetUserSynonyms(), L"; "));
      m_Edit.SetWindowHandle(nullptr);

      // Fansub group
      RefreshFansubPreference();
      
      break;
    }
  }
}

void AnimeInfoPage::RefreshFansubPreference() {
  if (anime_id_ <= anime::ID_UNKNOWN || index != INFOPAGE_MYINFO) return;

  wstring text;
  vector<wstring> groups;
  
  if (anime::GetFansubFilter(anime_id_, groups)) {
    for (auto it = groups.begin(); it != groups.end(); ++it) {
      if (!text.empty()) text += L" or ";
      text += L"\"" + *it + L"\"";
    }
  } else {
    text = L"None";
  }

  text = L"Fansub group preference: " + text + L" <a href=\"#\">(Change)</a>";
  SetDlgItemText(IDC_LINK_ANIME_FANSUB, text.c_str());
}