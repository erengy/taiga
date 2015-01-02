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

#include "base/foreach.h"
#include "base/string.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "sync/sync.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "track/recognition.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_anime_info_page.h"
#include "ui/dlg/dlg_input.h"
#include "ui/theme.h"
#include "win/win_commondialog.h"

namespace ui {

PageBaseInfo::PageBaseInfo()
    : anime_id_(anime::ID_UNKNOWN), parent(nullptr) {
}

BOOL PageBaseInfo::OnInitDialog() {
  // Set new font for headers
  for (int i = 0; i < 3; i++) {
    SendDlgItemMessage(IDC_STATIC_HEADER1 + i, WM_SETFONT,
                       reinterpret_cast<WPARAM>(ui::Theme.GetBoldFont()), FALSE);
  }

  return TRUE;
}

INT_PTR PageBaseInfo::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC: {
      if (!parent->IsTabVisible()) {
        win::Dc dc = reinterpret_cast<HDC>(wParam);
        HWND hwnd_control = reinterpret_cast<HWND>(lParam);
        dc.SetBkMode(TRANSPARENT);
        dc.DetachDc();
        if (hwnd_control == GetDlgItem(IDC_EDIT_ANIME_ALT))
          return reinterpret_cast<INT_PTR>(Theme.GetBackgroundBrush());
        return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
      }
      break;
    }
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
    header.GetWindowRect(GetWindowHandle(), &rect_header);
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
      rect.Inflate(-ScaleX(win::kControlMargin), -ScaleY(win::kControlMargin));

      // Headers
      for (int i = 0; i < 3; i++) {
        win::Rect rect_header;
        win::Window header = GetDlgItem(IDC_STATIC_HEADER1 + i);
        header.GetWindowRect(GetWindowHandle(), &rect_header);
        rect_header.right = rect.right;
        header.SetPosition(nullptr, rect_header);
        header.SetWindowHandle(nullptr);
      }

      // Redraw
      InvalidateRect();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void PageSeriesInfo::OnSize(UINT uMsg, UINT nType, SIZE size) {
  PageBaseInfo::OnSize(uMsg, nType, size);

  switch (uMsg) {
    case WM_SIZE: {
      win::Rect rect;
      rect.Set(0, 0, size.cx, size.cy);
      rect.Inflate(-ScaleX(win::kControlMargin), -ScaleY(win::kControlMargin));

      // Synonyms
      win::Rect rect_child;
      win::Window window = GetDlgItem(IDC_EDIT_ANIME_ALT);
      window.GetWindowRect(GetWindowHandle(), &rect_child);
      rect_child.right = rect.right - ScaleX(win::kControlMargin);
      window.SetPosition(nullptr, rect_child);

      // Details
      window.SetWindowHandle(GetDlgItem(IDC_STATIC_ANIME_DETAILS));
      window.GetWindowRect(GetWindowHandle(), &rect_child);
      rect_child.right = rect.right - ScaleX(win::kControlMargin);
      window.SetPosition(nullptr, rect_child);

      // Synopsis
      window.SetWindowHandle(GetDlgItem(IDC_EDIT_ANIME_SYNOPSIS));
      window.GetWindowRect(GetWindowHandle(), &rect_child);
      rect_child.right = rect.right - ScaleX(win::kControlMargin);
      rect_child.bottom = rect.bottom;
      window.SetPosition(nullptr, rect_child);
      window.SetWindowHandle(nullptr);
    }
  }
}

void PageSeriesInfo::Refresh(int anime_id, bool connect) {
  if (!anime::IsValidId(anime_id))
    return;

  anime_id_ = anime_id;
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  // Update window title
  parent->UpdateTitle(false);

  // Set synonyms
  std::wstring text = Join(anime_item->GetSynonyms(), L", ");
  if (text.empty())
    text = L"-";
  SetDlgItemText(IDC_EDIT_ANIME_ALT, text.c_str());

  // Set information
  text = anime::TranslateType(anime_item->GetType()) + L"\n" +
         anime::TranslateNumber(anime_item->GetEpisodeCount(), L"Unknown") + L"\n" +
         anime::TranslateStatus(anime_item->GetAiringStatus()) + L"\n" +
         anime::TranslateDateToSeasonString(anime_item->GetDateStart()) + L"\n" +
         (anime_item->GetGenres().empty() ? L"Unknown" : Join(anime_item->GetGenres(), L", ")) + L"\n" +
         (anime_item->GetProducers().empty() ? L"Unknown" : Join(anime_item->GetProducers(), L", ")) + L"\n" +
         anime::TranslateScore(anime_item->GetScore());
  SetDlgItemText(IDC_STATIC_ANIME_DETAILS, text.c_str());

  // Set synopsis
  text = anime_item->GetSynopsis();
  SetDlgItemText(IDC_EDIT_ANIME_SYNOPSIS, text.c_str());

  // Get new data if necessary
  if (connect && anime::MetadataNeedsRefresh(*anime_item)) {
    parent->UpdateTitle(true);
    sync::GetMetadataById(anime_id_);
  }
}

////////////////////////////////////////////////////////////////////////////////

BOOL PageMyInfo::OnCommand(WPARAM wParam, LPARAM lParam) {
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  switch (LOWORD(wParam)) {
    // Browse anime folder
    case IDC_BUTTON_BROWSE: {
      std::wstring default_path, path;
      if (!anime_item->GetFolder().empty()) {
        default_path = anime_item->GetFolder();
      } else if (!Settings.root_folders.empty()) {
        default_path = Settings.root_folders.front();
      }
      if (win::BrowseForFolder(GetWindowHandle(), L"Choose an anime folder", default_path, path)) {
        SetDlgItemText(IDC_EDIT_ANIME_FOLDER, path.c_str());
      }
      return TRUE;
    }

    // User changed rewatching checkbox
    case IDC_CHECK_ANIME_REWATCH:
      if (HIWORD(wParam) == BN_CLICKED) {
        win::Spin spin = GetDlgItem(IDC_SPIN_PROGRESS);
        int episode_value = 0;
        spin.GetPos32(episode_value);
        if (IsDlgButtonChecked(IDC_CHECK_ANIME_REWATCH)) {
          if (anime_item->GetMyStatus() == anime::kCompleted &&
              episode_value == anime_item->GetEpisodeCount())
            spin.SetPos32(0);
        } else {
          if (episode_value == 0)
            spin.SetPos32(anime_item->GetMyLastWatchedEpisode());
        }
        spin.SetWindowHandle(nullptr);
        return TRUE;
      }
      break;

    // User changed date checkbox
    case IDC_CHECK_ANIME_DATE_START: {
      auto enabled = IsDlgButtonChecked(IDC_CHECK_ANIME_DATE_START);
      EnableDlgItem(IDC_COMBO_ANIME_DATE_START_YEAR, enabled);
      EnableDlgItem(IDC_COMBO_ANIME_DATE_START_MONTH, enabled);
      EnableDlgItem(IDC_COMBO_ANIME_DATE_START_DAY, enabled);
      break;
    }
    case IDC_CHECK_ANIME_DATE_FINISH: {
      auto enabled = IsDlgButtonChecked(IDC_CHECK_ANIME_DATE_FINISH);
      EnableDlgItem(IDC_COMBO_ANIME_DATE_FINISH_YEAR, enabled);
      EnableDlgItem(IDC_COMBO_ANIME_DATE_FINISH_MONTH, enabled);
      EnableDlgItem(IDC_COMBO_ANIME_DATE_FINISH_DAY, enabled);
      break;
    }

    // User changed status dropdown
    case IDC_COMBO_ANIME_STATUS:
      if (HIWORD(wParam) == CBN_SELENDOK) {
        // Selected "Completed"
        win::ComboBox combobox = GetDlgItem(IDC_COMBO_ANIME_STATUS);
        if (combobox.GetItemData(combobox.GetCurSel()) == anime::kCompleted)
          if (anime_item->GetMyStatus() != anime::kCompleted &&
              anime_item->GetEpisodeCount() > 0)
            SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETPOS32, 0, anime_item->GetEpisodeCount());
        combobox.SetWindowHandle(nullptr);
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
        case NM_CLICK:
        case NM_RETURN: {
          // Set/change fansub group preference
          std::vector<std::wstring> groups;
          anime::GetFansubFilter(anime_id_, groups);
          std::wstring text = Join(groups, L", ");
          InputDialog dlg;
          dlg.title = AnimeDatabase.FindItem(anime_id_)->GetTitle();
          dlg.info = L"Please enter your fansub group preference for this title:";
          dlg.text = text;
          dlg.Show(parent->GetWindowHandle());
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
  if (!anime::IsValidId(anime_id))
    return;

  anime_id_ = anime_id;
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  if (!anime_item->IsInList())
    return;

  // Episodes watched
  SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETRANGE32, 0,
                     anime_item->GetEpisodeCount() > 0 ? anime_item->GetEpisodeCount() : 9999);
  SendDlgItemMessage(IDC_SPIN_PROGRESS, UDM_SETPOS32, 0, anime_item->GetMyLastWatchedEpisode());

  // Rewatching
  CheckDlgButton(IDC_CHECK_ANIME_REWATCH, anime_item->GetMyRewatching());

  // Status
  win::ComboBox combobox = GetDlgItem(IDC_COMBO_ANIME_STATUS);
  if (combobox.GetCount() == 0)
    for (int i = anime::kMyStatusFirst; i < anime::kMyStatusLast; i++)
      combobox.AddItem(anime::TranslateMyStatus(i, false).c_str(), i);
  combobox.SetCurSel(anime_item->GetMyStatus() - 1);
  combobox.SetWindowHandle(nullptr);

  // Score
  combobox.SetWindowHandle(GetDlgItem(IDC_COMBO_ANIME_SCORE));
  if (combobox.GetCount() == 0) {
    for (int i = 10; i >= 0; i--) {
      combobox.AddString(anime::TranslateMyScoreFull(i).c_str());
    }
  }
  combobox.SetCurSel(10 - anime_item->GetMyScore());
  combobox.SetWindowHandle(nullptr);

  // Tags
  win::Edit edit = GetDlgItem(IDC_EDIT_ANIME_TAGS);
  edit.SetCueBannerText(L"Enter tags here, separated by a comma (e.g. tag1, tag2)");
  edit.SetText(anime_item->GetMyTags());
  edit.SetWindowHandle(nullptr);

  // Dates
  bool date_start_enabled = anime::IsValidDate(anime_item->GetMyDateStart());
  bool date_finish_enabled = anime::IsValidDate(anime_item->GetMyDateEnd());
  CheckDlgButton(IDC_CHECK_ANIME_DATE_START, date_start_enabled);
  CheckDlgButton(IDC_CHECK_ANIME_DATE_FINISH, date_finish_enabled);
  EnableDlgItem(IDC_COMBO_ANIME_DATE_START_YEAR, date_start_enabled);
  EnableDlgItem(IDC_COMBO_ANIME_DATE_START_MONTH, date_start_enabled);
  EnableDlgItem(IDC_COMBO_ANIME_DATE_START_DAY, date_start_enabled);
  EnableDlgItem(IDC_COMBO_ANIME_DATE_FINISH_YEAR, date_finish_enabled);
  EnableDlgItem(IDC_COMBO_ANIME_DATE_FINISH_MONTH, date_finish_enabled);
  EnableDlgItem(IDC_COMBO_ANIME_DATE_FINISH_DAY, date_finish_enabled);

  win::ComboBox combo_start_year = GetDlgItem(IDC_COMBO_ANIME_DATE_START_YEAR);
  win::ComboBox combo_start_month = GetDlgItem(IDC_COMBO_ANIME_DATE_START_MONTH);
  win::ComboBox combo_start_day = GetDlgItem(IDC_COMBO_ANIME_DATE_START_DAY);
  win::ComboBox combo_finish_year = GetDlgItem(IDC_COMBO_ANIME_DATE_FINISH_YEAR);
  win::ComboBox combo_finish_month = GetDlgItem(IDC_COMBO_ANIME_DATE_FINISH_MONTH);
  win::ComboBox combo_finish_day = GetDlgItem(IDC_COMBO_ANIME_DATE_FINISH_DAY);

  Date date_now = GetDate();
  Date date_start = anime_item->GetDateStart();
  Date date_finish = anime_item->GetDateEnd();

  combo_start_year.ResetContent();
  combo_finish_year.ResetContent();
  combo_start_year.AddItem(L"(yyyy)", 0);
  combo_finish_year.AddItem(L"(yyyy)", 0);
  for (int i = date_now.year; i >= max(date_start.year, 1985); --i)
    combo_start_year.AddItem(ToWstr(i).c_str(), i);
  for (int i = date_now.year; i >= max(date_finish.year, 1985); --i)
    combo_finish_year.AddItem(ToWstr(i).c_str(), i);
  combo_start_year.SetCurSel(anime_item->GetMyDateStart().year == 0 ? 0 :
                             1 + date_now.year - anime_item->GetMyDateStart().year);
  combo_finish_year.SetCurSel(anime_item->GetMyDateEnd().year == 0 ? 0 :
                              1 + date_now.year - anime_item->GetMyDateEnd().year);

  combo_start_month.ResetContent();
  combo_finish_month.ResetContent();
  combo_start_month.AddItem(L"(mm)", 0);
  combo_finish_month.AddItem(L"(mm)", 0);
  for (int i = 1; i <= 12; ++i) {
    combo_start_month.AddItem(ToWstr(i).c_str(), i);
    combo_finish_month.AddItem(ToWstr(i).c_str(), i);
  }
  combo_start_month.SetCurSel(anime_item->GetMyDateStart().month);
  combo_finish_month.SetCurSel(anime_item->GetMyDateEnd().month);

  combo_start_day.ResetContent();
  combo_finish_day.ResetContent();
  combo_start_day.AddItem(L"(dd)", 0);
  combo_finish_day.AddItem(L"(dd)", 0);
  for (int i = 1; i <= 31; ++i) {
    combo_start_day.AddItem(ToWstr(i).c_str(), i);
    combo_finish_day.AddItem(ToWstr(i).c_str(), i);
  }
  combo_start_day.SetCurSel(anime_item->GetMyDateStart().day);
  combo_finish_day.SetCurSel(anime_item->GetMyDateEnd().day);

  combo_finish_day.SetWindowHandle(nullptr);
  combo_finish_month.SetWindowHandle(nullptr);
  combo_finish_year.SetWindowHandle(nullptr);
  combo_start_day.SetWindowHandle(nullptr);
  combo_start_month.SetWindowHandle(nullptr);
  combo_start_year.SetWindowHandle(nullptr);

  // Alternative titles
  edit.SetWindowHandle(GetDlgItem(IDC_EDIT_ANIME_ALT));
  edit.SetCueBannerText(L"Enter alternative titles here, separated by a semicolon (e.g. Title 1; Title 2)");
  edit.SetText(Join(anime_item->GetUserSynonyms(), L"; "));
  edit.SetWindowHandle(nullptr);
  CheckDlgButton(IDC_CHECK_ANIME_ALT, anime_item->GetUseAlternative());

  // Folder
  edit.SetWindowHandle(GetDlgItem(IDC_EDIT_ANIME_FOLDER));
  edit.SetText(anime_item->GetFolder());
  edit.SetWindowHandle(nullptr);

  // Fansub group
  RefreshFansubPreference();
}

void PageMyInfo::RefreshFansubPreference() {
  if (!anime::IsValidId(anime_id_))
    return;

  std::wstring text;
  std::vector<std::wstring> groups;

  if (anime::GetFansubFilter(anime_id_, groups)) {
    foreach_(it, groups) {
      if (!text.empty())
        text += L" or ";
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
  HistoryItem history_item;
  history_item.anime_id = anime_id_;
  history_item.mode = taiga::kHttpServiceUpdateLibraryEntry;

  // Episodes watched
  history_item.episode = GetDlgItemInt(IDC_EDIT_ANIME_PROGRESS);
  if (!anime::IsValidEpisode(*history_item.episode, anime_item->GetEpisodeCount())) {
    std::wstring msg = L"Please enter a valid episode number between 0-" +
                  ToWstr(anime_item->GetEpisodeCount()) + L".";
    MessageBox(msg.c_str(), L"Episodes watched", MB_OK | MB_ICONERROR);
    return false;
  }

  // Rewatching
  history_item.enable_rewatching = IsDlgButtonChecked(IDC_CHECK_ANIME_REWATCH);

  // Score
  history_item.score = 10 - GetComboSelection(IDC_COMBO_ANIME_SCORE);

  // Status
  history_item.status = GetComboSelection(IDC_COMBO_ANIME_STATUS) + 1;

  // Tags
  history_item.tags = GetDlgItemText(IDC_EDIT_ANIME_TAGS);

  // Start date
  if (!IsDlgButtonChecked(IDC_CHECK_ANIME_DATE_START)) {
    history_item.date_start = Date();
  } else {
    win::ComboBox combo_year = GetDlgItem(IDC_COMBO_ANIME_DATE_START_YEAR);
    win::ComboBox combo_month = GetDlgItem(IDC_COMBO_ANIME_DATE_START_MONTH);
    win::ComboBox combo_day = GetDlgItem(IDC_COMBO_ANIME_DATE_START_DAY);
    history_item.date_start = Date(
        combo_year.GetItemData(combo_year.GetCurSel()),
        combo_month.GetItemData(combo_month.GetCurSel()),
        combo_day.GetItemData(combo_day.GetCurSel()));
    combo_day.SetWindowHandle(nullptr);
    combo_month.SetWindowHandle(nullptr);
    combo_year.SetWindowHandle(nullptr);
  }
  // Finish date
  if (!IsDlgButtonChecked(IDC_CHECK_ANIME_DATE_FINISH)) {
    history_item.date_finish = Date();
  } else {
    win::ComboBox combo_year = GetDlgItem(IDC_COMBO_ANIME_DATE_FINISH_YEAR);
    win::ComboBox combo_month = GetDlgItem(IDC_COMBO_ANIME_DATE_FINISH_MONTH);
    win::ComboBox combo_day = GetDlgItem(IDC_COMBO_ANIME_DATE_FINISH_DAY);
    history_item.date_finish = Date(
        combo_year.GetItemData(combo_year.GetCurSel()),
        combo_month.GetItemData(combo_month.GetCurSel()),
        combo_day.GetItemData(combo_day.GetCurSel()));
    combo_day.SetWindowHandle(nullptr);
    combo_month.SetWindowHandle(nullptr);
    combo_year.SetWindowHandle(nullptr);
  }

  // Alternative titles
  anime_item->SetUserSynonyms(GetDlgItemText(IDC_EDIT_ANIME_ALT));
  anime_item->SetUseAlternative(IsDlgButtonChecked(IDC_CHECK_ANIME_ALT) == TRUE);
  Meow.UpdateCleanTitles(anime_id_);

  // Folder
  anime_item->SetFolder(GetDlgItemText(IDC_EDIT_ANIME_FOLDER));

  // Save settings
  Settings.Save();

  // Add item to queue
  History.queue.Add(history_item);
  return true;
}

}  // namespace ui