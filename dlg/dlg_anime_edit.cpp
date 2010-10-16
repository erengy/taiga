/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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
#include "dlg_anime_edit.h"
#include "../event.h"
#include "../http.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"
#include "../ui/ui_control.h"

CAnimeEditWindow AnimeEditWindow;

// =============================================================================

CAnimeEditWindow::CAnimeEditWindow() {
  m_Index = 0;
  RegisterDlgClass(L"TaigaAnimeEditW");
}

CAnimeEditWindow::~CAnimeEditWindow() {}

BOOL CAnimeEditWindow::OnInitDialog() {
  // Set index
  m_Index = AnimeList.Index;
  // Set caption
  SetText(AnimeList.Item[m_Index].Series_Title.c_str());
  
  // Progress
  SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETRANGE32, 0, 
    AnimeList.Item[m_Index].Series_Episodes > 0 ? AnimeList.Item[m_Index].Series_Episodes : 999);
  SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETPOS32, 0, AnimeList.Item[m_Index].My_WatchedEpisodes);

  // Status
  CComboBox m_Combo = GetDlgItem(IDC_COMBO_ANIME_STATUS);
  for (int i = MAL_WATCHING; i <= MAL_PLANTOWATCH; i++) {
    if (i != MAL_UNKNOWN) {
      m_Combo.AddString(MAL.TranslateMyStatus(i, false).c_str());
    }
  }
  int status = AnimeList.Item[m_Index].My_Status;
  if (status == MAL_PLANTOWATCH) status--;
  m_Combo.SetCurSel(status - 1);
  m_Combo.SetWindowHandle(NULL);

  // Score
  m_Combo.SetWindowHandle(GetDlgItem(IDC_COMBO_ANIME_SCORE));
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
  m_Combo.SetCurSel(10 - AnimeList.Item[m_Index].My_Score);
  m_Combo.SetWindowHandle(NULL);

  // Tags
  SetDlgItemText(IDC_EDIT_ANIME_TAGS, AnimeList.Item[m_Index].My_Tags.c_str());

  // Start date
  if (AnimeList.Item[m_Index].My_StartDate == L"0000-00-00" ||
    AnimeList.Item[m_Index].My_StartDate.empty()) {
      SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_NONE, NULL);
  } else {
    SYSTEMTIME st;
    st.wYear  = ToINT(AnimeList.Item[m_Index].My_StartDate.substr(0, 4));
    st.wMonth = ToINT(AnimeList.Item[m_Index].My_StartDate.substr(5, 2));
    st.wDay   = ToINT(AnimeList.Item[m_Index].My_StartDate.substr(8, 2));
    SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
  }
  // Finish date
  if (AnimeList.Item[m_Index].My_FinishDate == L"0000-00-00" ||
    AnimeList.Item[m_Index].My_FinishDate.empty()) {
      SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_NONE, NULL);
  } else {
    SYSTEMTIME st;
    st.wYear  = ToINT(AnimeList.Item[m_Index].My_FinishDate.substr(0, 4));
    st.wMonth = ToINT(AnimeList.Item[m_Index].My_FinishDate.substr(5, 2));
    st.wDay   = ToINT(AnimeList.Item[m_Index].My_FinishDate.substr(8, 2));
    SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
  }

  return TRUE;
}

void CAnimeEditWindow::OnOK() {
  // Progress
  int episode = GetDlgItemInt(IDC_EDIT_ANIME_PROGRESS);

  // Score
  int score = 10 - GetComboSelection(IDC_COMBO_ANIME_SCORE);

  // Status
  int status = GetComboSelection(IDC_COMBO_ANIME_STATUS) + 1;
  if (status == MAL_UNKNOWN) status++;
  
  // Tags
  wstring tags;
  GetDlgItemText(IDC_EDIT_ANIME_TAGS, tags);

  // Start date
  SYSTEMTIME sts;
  if (SendDlgItemMessage(IDC_DATETIME_START, DTM_GETSYSTEMTIME, 0, reinterpret_cast<LPARAM>(&sts)) == GDT_NONE) {
    AnimeList.Item[m_Index].My_StartDate = L"0000-00-00";
  } else {
    wstring year  = WSTR(sts.wYear);
    wstring month = (sts.wMonth < 10 ? L"0" : L"") + WSTR(sts.wMonth);
    wstring day   = (sts.wDay < 10 ? L"0" : L"") + WSTR(sts.wDay);
    AnimeList.Item[m_Index].SetStartDate(year + L"-" + month + L"-" + day, true);
  }
  // Finish date
  SYSTEMTIME stf;
  if (SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_GETSYSTEMTIME, 0, reinterpret_cast<LPARAM>(&stf)) == GDT_NONE) {
    AnimeList.Item[m_Index].My_FinishDate = L"0000-00-00";
  } else {
    wstring year  = WSTR(stf.wYear);
    wstring month = (stf.wMonth < 10 ? L"0" : L"") + WSTR(stf.wMonth);
    wstring day   = (stf.wDay < 10 ? L"0" : L"") + WSTR(stf.wDay);
    AnimeList.Item[m_Index].SetFinishDate(year + L"-" + month + L"-" + day, true);
  }
  
  // Add to buffer
  EventBuffer.Add(L"", m_Index, AnimeList.Item[m_Index].Series_ID, episode, score, status, tags, L"", HTTP_MAL_AnimeEdit);

  // Exit
  EndDialog(IDOK);
}