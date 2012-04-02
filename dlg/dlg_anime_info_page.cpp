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
#include "../animedb.h"
#include "../animelist.h"
#include "dlg_anime_info.h"
#include "dlg_anime_info_page.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"

// =============================================================================

AnimeInfoPage::AnimeInfoPage() :
  anime_(nullptr), header_font_(nullptr)
{
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
  switch (LOWORD(wParam)) {
    // User changed rewatching checkbox
    case IDC_CHECK_ANIME_REWATCH:
      if (HIWORD(wParam) == BN_CLICKED) {
        if (!anime_) break;
        win32::ComboBox m_Combo = GetDlgItem(IDC_COMBO_ANIME_STATUS);
        win32::Spin m_Spin = GetDlgItem(IDC_SPIN_PROGRESS);
        int episode_value; m_Spin.GetPos32(episode_value);
        if (IsDlgButtonChecked(IDC_CHECK_ANIME_REWATCH)) {
          if (anime_->GetStatus() == MAL_COMPLETED && episode_value == anime_->series_episodes) {
            m_Spin.SetPos32(0);
          }
          m_Combo.Enable(FALSE);
          m_Combo.SetCurSel(MAL_COMPLETED - 1);
        } else {
          if (episode_value == 0) {
            m_Spin.SetPos32(anime_->GetLastWatchedEpisode());
          }
          m_Combo.Enable();
          m_Combo.SetCurSel(anime_->GetStatus() - 1);
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
        if (m_Combo.GetItemData(m_Combo.GetCurSel()) == MAL_COMPLETED) {
          if (anime_ && anime_->GetStatus() != MAL_COMPLETED && anime_->series_episodes > 0) {
            SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETPOS32, 0, anime_->series_episodes);
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
          if (anime_) {
            // Set/change fansub group preference
            if (anime_->SetFansubFilter()) {
              RefreshFansubPreference();
            }
          }
          return TRUE;
        }
      }
  }

  return 0;
}

// =============================================================================

void AnimeInfoPage::Refresh(Anime* anime) {
  if (!anime) return;

  // Store pointer to current anime item
  anime_ = anime;
  
  switch (index) {
    // Series information
    case TAB_SERIESINFO: {
      // Set synonyms
      wstring text;
      if (!anime->series_synonyms.empty()) {
        text = anime->series_synonyms;
        Replace(text, L"; ", L", ", true);
        TrimLeft(text, L", ");
      }
      if (text.empty()) text = L"-";
      SetDlgItemText(IDC_EDIT_ANIME_ALT, text.c_str());
      
      // Set synopsis
      bool must_update = false;
      Anime* db_anime = AnimeDatabase.FindItem(anime->series_id);
      if (db_anime != nullptr) {
        must_update = db_anime->IsDataOld();
      } else {
        must_update = anime->synopsis.empty();
      }
      if (must_update) {
        if (MAL.SearchAnime(anime->series_title, anime)) {
          text = L"Retrieving...";
        } else {
          text = L"-";
        }
      } else {
        text = anime->synopsis;
        if (anime->genres.empty() || anime->score.empty()) {
          MAL.GetAnimeDetails(anime);
        }
      }
      SetDlgItemText(IDC_EDIT_ANIME_INFO, text.c_str());
      
      // Set information
      text = MAL.TranslateType(anime->series_type) + L"\n" + 
             MAL.TranslateNumber(anime->series_episodes, L"Unknown") + L"\n" + 
             MAL.TranslateStatus(anime->GetAiringStatus()) + L"\n" + 
             MAL.TranslateDateToSeason(anime->series_start);
      SetDlgItemText(IDC_STATIC_ANIME_INFO1, text.c_str());
      #define ADD_INFOLINE(x, y) (anime->x.empty() ? y : anime->x)
      wstring genres = anime->genres; LimitText(anime->genres, 50); // TEMP
      text = ADD_INFOLINE(genres, L"-") + L"\n" +
             ADD_INFOLINE(score, L"0.00") + L"\n" + 
             ADD_INFOLINE(rank, L"#0") + L"\n" + 
             ADD_INFOLINE(popularity, L"#0");
      #undef ADD_INFOLINE
      anime->genres = genres;
      SetDlgItemText(IDC_STATIC_ANIME_INFO2, text.c_str());

      break;
    }

    // =========================================================================

    // My information
    case TAB_MYINFO: {
      // Episodes watched
      SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETRANGE32, 0, 
        anime->series_episodes > 0 ? anime->series_episodes : 9999);
      SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETPOS32, 0, anime->GetLastWatchedEpisode());

      // Re-watching
      CheckDlgButton(IDC_CHECK_ANIME_REWATCH, anime->GetRewatching());
      EnableDlgItem(IDC_CHECK_ANIME_REWATCH, anime->GetStatus() == MAL_COMPLETED);

      // Status
      win32::ComboBox m_Combo = GetDlgItem(IDC_COMBO_ANIME_STATUS);
      if (m_Combo.GetCount() == 0) {
        for (int i = MAL_WATCHING; i <= MAL_PLANTOWATCH; i++) {
          if (i != MAL_UNKNOWN) {
            m_Combo.AddItem(MAL.TranslateMyStatus(i, false).c_str(), i);
          }
        }
      }
      int status = anime->GetStatus();
      if (status == MAL_PLANTOWATCH) status--;
      m_Combo.SetCurSel(status - 1);
      m_Combo.Enable(!anime->GetRewatching());
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
      m_Combo.SetCurSel(10 - anime->GetScore());
      m_Combo.SetWindowHandle(nullptr);

      // Tags
      win32::Edit m_Edit = GetDlgItem(IDC_EDIT_ANIME_TAGS);
      m_Edit.SetCueBannerText(L"Enter tags here, separated by a comma (e.g. tag1, tag2)");
      m_Edit.SetText(anime->GetTags());
      m_Edit.SetWindowHandle(nullptr);
      
      // Date limits and defaults
      if (MAL.IsValidDate(anime->series_start)) {
        SYSTEMTIME stSeriesStart;
        MAL.ParseDateString(anime->series_start, stSeriesStart.wYear, stSeriesStart.wMonth, stSeriesStart.wDay);
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETRANGE, GDTR_MIN, (LPARAM)&stSeriesStart);
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stSeriesStart);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETRANGE, GDTR_MIN, (LPARAM)&stSeriesStart);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stSeriesStart);
      }
      if (MAL.IsValidDate(anime->series_end)) {
        SYSTEMTIME stSeriesEnd;
        MAL.ParseDateString(anime->series_end, stSeriesEnd.wYear, stSeriesEnd.wMonth, stSeriesEnd.wDay);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETRANGE, GDTR_MIN, (LPARAM)&stSeriesEnd);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stSeriesEnd);
      }
      // Start date
      if (MAL.IsValidDate(anime->my_start_date)) {
        SYSTEMTIME stMyStart;
        MAL.ParseDateString(anime->my_start_date, stMyStart.wYear, stMyStart.wMonth, stMyStart.wDay);
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stMyStart);
      } else {
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_NONE, 0);
      }
      // Finish date
      if (MAL.IsValidDate(anime->my_finish_date)) {
        SYSTEMTIME stMyFinish;
        MAL.ParseDateString(anime->my_finish_date, stMyFinish.wYear, stMyFinish.wMonth, stMyFinish.wDay);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stMyFinish);
      } else {
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_NONE, 0);
      }

      // Alternative titles
      m_Edit.SetWindowHandle(GetDlgItem(IDC_EDIT_ANIME_ALT));
      m_Edit.SetCueBannerText(L"Enter alternative titles here, separated by a semicolon (e.g. Title 1; Title 2)");
      m_Edit.SetText(anime->synonyms);
      m_Edit.SetWindowHandle(nullptr);

      // Fansub group
      RefreshFansubPreference();
      
      break;
    }
  }
}

void AnimeInfoPage::RefreshFansubPreference() {
  if (!anime_ || index != TAB_MYINFO) return;

  wstring text;
  vector<wstring> groups;
  
  if (anime_->GetFansubFilter(groups)) {
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