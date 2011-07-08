/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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
#include "../animelist.h"
#include "dlg_anime_info.h"
#include "dlg_anime_info_page.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"

// =============================================================================

CAnimeInfoPage::CAnimeInfoPage() :
  m_hfHeader(NULL)
{
}

CAnimeInfoPage::~CAnimeInfoPage() {
  if (m_hfHeader) ::DeleteObject(m_hfHeader);
}

// =============================================================================

BOOL CAnimeInfoPage::OnInitDialog() {
  // Create bold header font
  if (!m_hfHeader) {
    LOGFONT lFont;
    ::GetObject(GetFont(), sizeof(LOGFONT), &lFont);
    lFont.lfCharSet = DEFAULT_CHARSET;
    lFont.lfWeight = FW_BOLD;
    m_hfHeader = ::CreateFontIndirect(&lFont);
  }
  // Set new font for headers
  for (int i = 0; i < 3; i++) {
    SendDlgItemMessage(IDC_STATIC_HEADER1 + i, WM_SETFONT, 
      reinterpret_cast<WPARAM>(m_hfHeader), FALSE);
  }
  
  return TRUE;
}

// =============================================================================

void CAnimeInfoPage::Refresh(CAnime* pAnimeItem) {
  if (!pAnimeItem) return;
  
  switch (Index) {
    // Series information
    case TAB_SERIESINFO: {
      // Set synonyms
      wstring text;
      if (!pAnimeItem->Series_Synonyms.empty()) {
        text = pAnimeItem->Series_Synonyms;
        Replace(text, L"; ", L", ", true);
        TrimLeft(text, L", ");
      }
      if (text.empty()) text = L"-";
      SetDlgItemText(IDC_EDIT_ANIME_ALT, text.c_str());
      
      // Set synopsis
      if (pAnimeItem->Synopsis.empty()) {
        if (MAL.SearchAnime(pAnimeItem->Series_Title, pAnimeItem)) {
          text = L"Retrieving...";
        } else {
          text = L"-";
        }
      } else {
        text = pAnimeItem->Synopsis;
        if (pAnimeItem->Genres.empty()) {
          MAL.GetAnimeDetails(pAnimeItem);
        }
      }
      SetDlgItemText(IDC_EDIT_ANIME_INFO, text.c_str());
      
      // Set information
      text = MAL.TranslateType(pAnimeItem->Series_Type) + L"\n" + 
             MAL.TranslateNumber(pAnimeItem->Series_Episodes, L"Unknown") + L"\n" + 
             MAL.TranslateStatus(pAnimeItem->GetAiringStatus()) + L"\n" + 
             MAL.TranslateDate(pAnimeItem->Series_Start);
      SetDlgItemText(IDC_STATIC_ANIME_INFO1, text.c_str());
      #define ADD_INFOLINE(x, y) (pAnimeItem->x.empty() ? y : pAnimeItem->x)
      wstring genres = pAnimeItem->Genres; LimitText(pAnimeItem->Genres, 50); // TEMP
      text = ADD_INFOLINE(Genres, L"-") + L"\n" +
             ADD_INFOLINE(Score, L"0.00") + L"\n" + 
             ADD_INFOLINE(Rank, L"#0") + L"\n" + 
             ADD_INFOLINE(Popularity, L"#0");
      #undef ADD_INFOLINE
      pAnimeItem->Genres = genres;
      SetDlgItemText(IDC_STATIC_ANIME_INFO2, text.c_str());

      break;
    }

    // =========================================================================

    // My information
    case TAB_MYINFO: {
      // Episodes watched
      SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETRANGE32, 0, 
        pAnimeItem->Series_Episodes > 0 ? pAnimeItem->Series_Episodes : 999);
      SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETPOS32, 0, pAnimeItem->GetLastWatchedEpisode());

      // Re-watching
      CheckDlgButton(IDC_CHECK_ANIME_REWATCH, pAnimeItem->GetRewatching());
      if (pAnimeItem->GetStatus() == MAL_COMPLETED) {
        EnableDlgItem(IDC_CHECK_ANIME_REWATCH, TRUE);
      } else {
        EnableDlgItem(IDC_CHECK_ANIME_REWATCH, FALSE);
      }

      // Status
      CComboBox m_Combo = GetDlgItem(IDC_COMBO_ANIME_STATUS);
      if (m_Combo.GetCount() == 0) {
        for (int i = MAL_WATCHING; i <= MAL_PLANTOWATCH; i++) {
          if (i != MAL_UNKNOWN) {
            m_Combo.AddString(MAL.TranslateMyStatus(i, false).c_str());
          }
        }
      }
      int status = pAnimeItem->GetStatus();
      if (status == MAL_PLANTOWATCH) status--;
      m_Combo.SetCurSel(status - 1);
      m_Combo.SetWindowHandle(NULL);

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
      m_Combo.SetCurSel(10 - pAnimeItem->GetScore());
      m_Combo.SetWindowHandle(NULL);

      // Tags
      CEdit m_Edit = GetDlgItem(IDC_EDIT_ANIME_TAGS);
      m_Edit.SetCueBannerText(L"Enter tags here, separated by a comma (e.g. tag1, tag2)");
      m_Edit.SetText(pAnimeItem->GetTags());
      m_Edit.SetWindowHandle(NULL);
      
      // Date limits and defaults
      if (MAL.IsValidDate(pAnimeItem->Series_Start)) {
        SYSTEMTIME stSeriesStart;
        MAL.ParseDateString(pAnimeItem->Series_Start, stSeriesStart.wYear, stSeriesStart.wMonth, stSeriesStart.wDay);
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETRANGE, GDTR_MIN, (LPARAM)&stSeriesStart);
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stSeriesStart);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETRANGE, GDTR_MIN, (LPARAM)&stSeriesStart);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stSeriesStart);
      }
      if (MAL.IsValidDate(pAnimeItem->Series_End)) {
        SYSTEMTIME stSeriesEnd;
        MAL.ParseDateString(pAnimeItem->Series_End, stSeriesEnd.wYear, stSeriesEnd.wMonth, stSeriesEnd.wDay);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETRANGE, GDTR_MIN, (LPARAM)&stSeriesEnd);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stSeriesEnd);
      }
      // Start date
      if (MAL.IsValidDate(pAnimeItem->My_StartDate)) {
        SYSTEMTIME stMyStart;
        MAL.ParseDateString(pAnimeItem->My_StartDate, stMyStart.wYear, stMyStart.wMonth, stMyStart.wDay);
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stMyStart);
      } else {
        SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_NONE, NULL);
      }
      // Finish date
      if (MAL.IsValidDate(pAnimeItem->My_FinishDate)) {
        SYSTEMTIME stMyFinish;
        MAL.ParseDateString(pAnimeItem->My_FinishDate, stMyFinish.wYear, stMyFinish.wMonth, stMyFinish.wDay);
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stMyFinish);
      } else {
        SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_NONE, NULL);
      }

      // Alternative titles
      m_Edit.SetWindowHandle(GetDlgItem(IDC_EDIT_ANIME_ALT));
      m_Edit.SetCueBannerText(L"Enter alternative titles here, separated by a semicolon (e.g. Title 1; Title 2)");
      m_Edit.SetText(pAnimeItem->Synonyms);
      m_Edit.SetWindowHandle(NULL);
      
      break;
    }
  }
}