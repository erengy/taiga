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

#include "base/std.h"

#include "dlg_anime_info.h"
#include "dlg_anime_info_page.h"

#include "dlg_input.h"

#include "library/anime.h"
#include "library/anime_db.h"
#include "base/common.h"
#include "library/history.h"
#include "taiga/http.h"
#include "sync/myanimelist.h"
#include "track/recognition.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "ui/theme.h"

// =============================================================================

PageBaseInfo::PageBaseInfo()
    : anime_id_(anime::ID_UNKNOWN) {
}

BOOL PageBaseInfo::OnInitDialog() {
  // Set new font for headers
  for (int i = 0; i < 3; i++) {
    SendDlgItemMessage(IDC_STATIC_HEADER1 + i, WM_SETFONT, 
      reinterpret_cast<WPARAM>(UI.font_bold.Get()), FALSE);
  }

  return TRUE;
}

INT_PTR PageBaseInfo::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC:
      if (!parent->IsTabVisible())
        return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

void PageBaseInfo::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  win::Dc dc = hdc;
  win::Rect rect;

  // Paint background
  rect.Copy(lpps->rcPaint);
  if (!parent->IsTabVisible())
    dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));

  // Paint header lines
  for (int i = 0; i < 3; i++) {
    win::Rect rect_header;
    win::Window header = GetDlgItem(IDC_STATIC_HEADER1 + i);
    header.GetWindowRect(m_hWindow, &rect_header);
    rect_header.top = rect_header.bottom + 3;
    rect_header.bottom =  rect_header.top + 1;
    dc.FillRect(rect_header, ::GetSysColor(COLOR_ACTIVEBORDER));
    rect_header.Offset(0, 1);
    dc.FillRect(rect_header, ::GetSysColor(COLOR_WINDOW));
    header.SetWindowHandle(nullptr);
  }
}

void PageBaseInfo::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      win::Rect rect;
      rect.Set(0, 0, size.cx, size.cy);
      rect.Inflate(-ScaleX(WIN_CONTROL_MARGIN), -ScaleY(WIN_CONTROL_MARGIN));

      // Headers
      for (int i = 0; i < 3; i++) {
        win::Rect rect_header;
        win::Window header = GetDlgItem(IDC_STATIC_HEADER1 + i);
        header.GetWindowRect(m_hWindow, &rect_header);
        rect_header.right = rect.right;
        header.SetPosition(nullptr, rect_header);
        header.SetWindowHandle(nullptr);
      }

      // Redraw
      InvalidateRect();
    }
  }
}

// =============================================================================

void PageSeriesInfo::OnSize(UINT uMsg, UINT nType, SIZE size) {
  PageBaseInfo::OnSize(uMsg, nType, size);

  switch (uMsg) {
    case WM_SIZE: {
      win::Rect rect;
      rect.Set(0, 0, size.cx, size.cy);
      rect.Inflate(-ScaleX(WIN_CONTROL_MARGIN), -ScaleY(WIN_CONTROL_MARGIN));

      // Synonyms
      win::Rect rect_child;
      win::Window window = GetDlgItem(IDC_EDIT_ANIME_ALT);
      window.GetWindowRect(m_hWindow, &rect_child);
      rect_child.right = rect.right - ScaleX(WIN_CONTROL_MARGIN);
      window.SetPosition(nullptr, rect_child);

      // Details
      window.SetWindowHandle(GetDlgItem(IDC_STATIC_ANIME_DETAILS));
      window.GetWindowRect(m_hWindow, &rect_child);
      rect_child.right = rect.right - ScaleX(WIN_CONTROL_MARGIN);
      window.SetPosition(nullptr, rect_child);

      // Synopsis
      window.SetWindowHandle(GetDlgItem(IDC_EDIT_ANIME_SYNOPSIS));
      window.GetWindowRect(m_hWindow, &rect_child);
      rect_child.right = rect.right - ScaleX(WIN_CONTROL_MARGIN);
      rect_child.bottom = rect.bottom;
      window.SetPosition(nullptr, rect_child);
      window.SetWindowHandle(nullptr);
    }
  }
}

void PageSeriesInfo::Refresh(int anime_id, bool connect) {
  if (anime_id <= anime::ID_UNKNOWN) return;

  anime_id_ = anime_id;
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  // Set synonyms
  wstring text = Join(anime_item->GetSynonyms(), L", ");
  if (text.empty()) text = L"-";
  SetDlgItemText(IDC_EDIT_ANIME_ALT, text.c_str());

  // Set information
  #define ADD_INFOLINE(x, y) (x.empty() ? y : x)
  text = mal::TranslateType(anime_item->GetType()) + L"\n" +
         mal::TranslateNumber(anime_item->GetEpisodeCount(), L"Unknown") + L"\n" +
         mal::TranslateStatus(anime_item->GetAiringStatus()) + L"\n" +
         mal::TranslateDateToSeason(anime_item->GetDate(anime::DATE_START)) + L"\n" +
         ADD_INFOLINE(anime_item->GetGenres(), L"Unknown") + L"\n" +
         ADD_INFOLINE(anime_item->GetProducers(), L"Unknown") + L"\n" +
         ADD_INFOLINE(anime_item->GetScore(), L"0.00") + L"\n" +
         ADD_INFOLINE(anime_item->GetPopularity(), L"#0");
  #undef ADD_INFOLINE
  SetDlgItemText(IDC_STATIC_ANIME_DETAILS, text.c_str());

  // Set synopsis
  text = anime_item->GetSynopsis();
  SetDlgItemText(IDC_EDIT_ANIME_SYNOPSIS, text.c_str());
  if (connect) {
    if (anime_item->IsOldEnough() || anime_item->GetSynopsis().empty()) {
      mal::SearchAnime(anime_id_, anime_item->GetTitle());
    } else if (anime_item->GetGenres().empty() || anime_item->GetScore().empty()) {
      mal::GetAnimeDetails(anime_id_);
    }
  }
}

// =============================================================================

BOOL PageMyInfo::OnCommand(WPARAM wParam, LPARAM lParam) {
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  switch (LOWORD(wParam)) {
    // Browse anime folder
    case IDC_BUTTON_BROWSE: {
      wstring default_path, path;
      if (!anime_item->GetFolder().empty()) {
        default_path = anime_item->GetFolder();
      } else if (!Settings.Folders.root.empty()) {
        default_path = Settings.Folders.root.front();
      }
      if (BrowseForFolder(m_hWindow, L"Choose an anime folder", default_path, path)) {
        SetDlgItemText(IDC_EDIT_ANIME_FOLDER, path.c_str());
      }
      return TRUE;
    }

    // User changed rewatching checkbox
    case IDC_CHECK_ANIME_REWATCH:
      if (HIWORD(wParam) == BN_CLICKED) {
        win::ComboBox m_Combo = GetDlgItem(IDC_COMBO_ANIME_STATUS);
        win::Spin m_Spin = GetDlgItem(IDC_SPIN_PROGRESS);
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
        win::ComboBox m_Combo = GetDlgItem(IDC_COMBO_ANIME_STATUS);
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

LRESULT PageMyInfo::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (pnmh->idFrom) {
    case IDC_LINK_ANIME_FANSUB:
      switch (pnmh->code) {
        case NM_CLICK: {
          // Set/change fansub group preference
          vector<wstring> groups;
          anime::GetFansubFilter(anime_id_, groups);
          wstring text = Join(groups, L", ");
          InputDialog dlg;
          dlg.title = AnimeDatabase.FindItem(anime_id_)->GetTitle();
          dlg.info = L"Please enter your fansub group preference for this title:";
          dlg.text = text;
          dlg.Show(AnimeDialog.GetWindowHandle());
          if (dlg.result == IDOK)
            if (anime::SetFansubFilter(anime_id_, dlg.text))
              RefreshFansubPreference();
          return TRUE;
        }
      }
  }

  return 0;
}

void PageMyInfo::Refresh(int anime_id) {
  if (anime_id <= anime::ID_UNKNOWN) return;

  anime_id_ = anime_id;
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  if (!anime_item->IsInList()) return;

  // Episodes watched
  SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETRANGE32, 0, 
    anime_item->GetEpisodeCount() > 0 ? anime_item->GetEpisodeCount() : 9999);
  SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETPOS32, 0, anime_item->GetMyLastWatchedEpisode());

  // Re-watching
  CheckDlgButton(IDC_CHECK_ANIME_REWATCH, anime_item->GetMyRewatching());
  EnableDlgItem(IDC_CHECK_ANIME_REWATCH, anime_item->GetMyStatus() == mal::MYSTATUS_COMPLETED);

  // Status
  win::ComboBox m_Combo = GetDlgItem(IDC_COMBO_ANIME_STATUS);
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
  win::Edit m_Edit = GetDlgItem(IDC_EDIT_ANIME_TAGS);
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
  CheckDlgButton(IDC_CHECK_ANIME_ALT, anime_item->GetUseAlternative());

  // Folder
  m_Edit.SetWindowHandle(GetDlgItem(IDC_EDIT_ANIME_FOLDER));
  m_Edit.SetText(anime_item->GetFolder());
  m_Edit.SetWindowHandle(nullptr);

  // Fansub group
  RefreshFansubPreference();
}

void PageMyInfo::RefreshFansubPreference() {
  if (anime_id_ <= anime::ID_UNKNOWN) return;

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

bool PageMyInfo::Save() {
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  // Create item
  EventItem event_item;
  event_item.anime_id = anime_id_;
  event_item.mode = HTTP_MAL_AnimeUpdate;

  // Episodes watched
  event_item.episode = GetDlgItemInt(IDC_EDIT_ANIME_PROGRESS);
  if (!mal::IsValidEpisode(*event_item.episode, -1, anime_item->GetEpisodeCount())) {
    wstring msg = L"Please enter a valid episode number between 0-" + 
                  ToWstr(anime_item->GetEpisodeCount()) + L".";
    MessageBox(msg.c_str(), L"Episodes watched", MB_OK | MB_ICONERROR);
    return false;
  }

  // Re-watching
  event_item.enable_rewatching = IsDlgButtonChecked(IDC_CHECK_ANIME_REWATCH);
  
  // Score
  event_item.score = 10 - GetComboSelection(IDC_COMBO_ANIME_SCORE);
  
  // Status
  event_item.status = GetComboSelection(IDC_COMBO_ANIME_STATUS) + 1;
  if (*event_item.status == mal::MYSTATUS_UNKNOWN)
    event_item.status = *event_item.status + 1;
  
  // Tags
  wstring tags;
  GetDlgItemText(IDC_EDIT_ANIME_TAGS, tags);
  event_item.tags = tags;

  // Start date
  SYSTEMTIME stMyStart;
  if (SendDlgItemMessage(IDC_DATETIME_START, DTM_GETSYSTEMTIME, 0, 
                         reinterpret_cast<LPARAM>(&stMyStart)) == GDT_NONE) {
    event_item.date_start = mal::TranslateDateForApi(Date());
  } else {
    event_item.date_start = mal::TranslateDateForApi(
      Date(stMyStart.wYear, stMyStart.wMonth, stMyStart.wDay));
  }
  // Finish date
  SYSTEMTIME stMyFinish;
  if (SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_GETSYSTEMTIME, 0, 
                         reinterpret_cast<LPARAM>(&stMyFinish)) == GDT_NONE) {
    event_item.date_finish = mal::TranslateDateForApi(Date());
  } else {
    event_item.date_finish = mal::TranslateDateForApi(
      Date(stMyFinish.wYear, stMyFinish.wMonth, stMyFinish.wDay));
  }

  // Alternative titles
  wstring titles;
  GetDlgItemText(IDC_EDIT_ANIME_ALT, titles);
  anime_item->SetUserSynonyms(titles);
  anime_item->SetUseAlternative(IsDlgButtonChecked(IDC_CHECK_ANIME_ALT) == TRUE);
  Meow.UpdateCleanTitles(anime_id_);

  // Folder
  wstring folder;
  GetDlgItemText(IDC_EDIT_ANIME_FOLDER, folder);
  anime_item->SetFolder(folder);

  // Save settings
  Settings.Save();

  // Add item to event queue
  History.queue.Add(event_item);
  return true;
}