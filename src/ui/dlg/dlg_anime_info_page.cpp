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
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_settings_page.h"
#include "ui/dialog.h"
#include "ui/theme.h"
#include "win/win_commondialog.h"

namespace ui {

PageBaseInfo::PageBaseInfo()
    : anime_id_(anime::ID_UNKNOWN), parent(nullptr) {
}

BOOL PageBaseInfo::OnClose() {
  if (parent && parent->GetMode() == kDialogModeAnimeInformation)
    parent->OnCancel();

  return TRUE;  // Disables closing via Escape key
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

  if (!anime_item)
    return;

  // Set alternative titles
  std::wstring main_title = Settings.GetBool(taiga::kApp_List_DisplayEnglishTitles) ?
      anime_item->GetEnglishTitle(true) : anime_item->GetTitle();
  std::vector<std::wstring> titles;
  anime::GetAllTitles(anime_id_, titles);
  titles.erase(std::remove(titles.begin(), titles.end(), main_title), titles.end());
  std::wstring text = Join(titles, L", ");
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

  if (!anime_item)
    return FALSE;

  switch (LOWORD(wParam)) {
    // Browse anime folder
    case IDC_BUTTON_BROWSE: {
      std::wstring default_path, path;
      if (!anime_item->GetFolder().empty()) {
        default_path = anime_item->GetFolder();
      } else if (!Settings.library_folders.empty()) {
        default_path = Settings.library_folders.front();
      }
      if (win::BrowseForFolder(parent->GetWindowHandle(), L"Select Anime Folder",
                               default_path, path)) {
        SetDlgItemText(IDC_EDIT_ANIME_FOLDER, path.c_str());
      }
      return TRUE;
    }

    // User changed rewatching checkbox
    case IDC_CHECK_ANIME_REWATCH:
      if (HIWORD(wParam) == BN_CLICKED) {
        win::ComboBox combobox = GetDlgItem(IDC_COMBO_ANIME_STATUS);
        win::Spin spin = GetDlgItem(IDC_SPIN_PROGRESS);
        int episode_value = 0;
        spin.GetPos32(episode_value);
        if (IsDlgButtonChecked(IDC_CHECK_ANIME_REWATCH)) {
          if (taiga::GetCurrentServiceId() == sync::kHummingbird)
            combobox.SetCurSel(anime::kWatching - 1);
          if (anime_item->GetMyStatus() == anime::kCompleted &&
              episode_value == anime_item->GetEpisodeCount())
            spin.SetPos32(0);
        } else {
          if (taiga::GetCurrentServiceId() == sync::kHummingbird)
            combobox.SetCurSel(anime_item->GetMyStatus() - 1);
          if (episode_value == 0)
            spin.SetPos32(anime_item->GetMyLastWatchedEpisode());
        }
        combobox.SetWindowHandle(nullptr);
        spin.SetWindowHandle(nullptr);
        return TRUE;
      }
      break;

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
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  if (!anime_item)
    return 0;

  switch (pnmh->idFrom) {
    case IDC_DATETIME_START:
      if (pnmh->code == DTN_DATETIMECHANGE)
        start_date_changed_ = true;
      break;
    case IDC_DATETIME_FINISH:
      if (pnmh->code == DTN_DATETIMECHANGE)
        finish_date_changed_ = true;
      break;

    case IDC_LINK_ANIME_FANSUB:
      switch (pnmh->code) {
        case NM_CLICK:
        case NM_RETURN: {
          // Set/change fansub group preference
          std::vector<std::wstring> groups;
          anime::GetFansubFilter(anime_id_, groups);
          if (groups.size() > 1) {
            ShowDlgSettings(kSettingsSectionTorrents, kSettingsPageTorrentsFilters);
            RefreshFansubPreference();
          } else {
            std::wstring text = Join(groups, L", ");
            InputDialog dlg;
            dlg.title = anime_item->GetTitle();
            dlg.info = L"Please enter your fansub group preference for this title:";
            dlg.text = text;
            dlg.Show(parent->GetWindowHandle());
            if (dlg.result == IDOK && dlg.text != text)
              if (anime::SetFansubFilter(anime_id_, dlg.text))
                RefreshFansubPreference();
          }
          return TRUE;
        }
      }
      break;
  }

  return 0;
}

void PageMyInfo::Refresh(int anime_id) {
  if (!anime::IsValidId(anime_id))
    return;

  anime_id_ = anime_id;
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  if (!anime_item || !anime_item->IsInList())
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
  start_date_changed_ = false;
  finish_date_changed_ = false;
  auto date_format = L"yyyy-MM-dd";
  SendDlgItemMessage(IDC_DATETIME_START, DTM_SETFORMAT, 0, (LPARAM)date_format);
  SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETFORMAT, 0, (LPARAM)date_format);
  auto set_default_systemtime = [&](int control_id, SYSTEMTIME& st) {
    SendDlgItemMessage(control_id, DTM_SETRANGE, GDTR_MIN, (LPARAM)&st);
    SendDlgItemMessage(control_id, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
  };
  if (anime::IsValidDate(anime_item->GetDateStart())) {
    SYSTEMTIME stSeriesStart = anime_item->GetDateStart();
    set_default_systemtime(IDC_DATETIME_START, stSeriesStart);
    set_default_systemtime(IDC_DATETIME_FINISH, stSeriesStart);
  }
  if (anime::IsValidDate(anime_item->GetDateEnd())) {
    SYSTEMTIME stSeriesEnd = anime_item->GetDateEnd();
    set_default_systemtime(IDC_DATETIME_FINISH, stSeriesEnd);
  }
  auto fix_systemtime = [](SYSTEMTIME& st) {
    if (!st.wMonth) st.wMonth = 1;
    if (!st.wDay) st.wDay = 1;
  };
  if (anime::IsValidDate(anime_item->GetMyDateStart())) {
    SYSTEMTIME stMyStart = anime_item->GetMyDateStart();
    fix_systemtime(stMyStart);
    SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stMyStart);
  } else {
    SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_NONE, 0);
  }
  if (anime::IsValidDate(anime_item->GetMyDateEnd())) {
    SYSTEMTIME stMyFinish = anime_item->GetMyDateEnd();
    fix_systemtime(stMyFinish);
    SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stMyFinish);
  } else {
    SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETSYSTEMTIME, GDT_NONE, 0);
  }

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

  if (!anime_item)
    return false;

  // Create item
  HistoryItem history_item;
  history_item.anime_id = anime_id_;
  history_item.mode = taiga::kHttpServiceUpdateLibraryEntry;

  // Episodes watched
  history_item.episode = GetDlgItemInt(IDC_EDIT_ANIME_PROGRESS);
  if (!anime::IsValidEpisodeNumber(*history_item.episode, anime_item->GetEpisodeCount())) {
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
  if (start_date_changed_) {
    SYSTEMTIME stMyStart = {0};
    if (SendDlgItemMessage(IDC_DATETIME_START, DTM_GETSYSTEMTIME, 0,
                           reinterpret_cast<LPARAM>(&stMyStart)) == GDT_NONE) {
      history_item.date_start = Date();
    } else {
      history_item.date_start = Date(stMyStart.wYear, stMyStart.wMonth, stMyStart.wDay);
    }
  }
  // Finish date
  if (finish_date_changed_) {
    SYSTEMTIME stMyFinish = {0};
    if (SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_GETSYSTEMTIME, 0,
                           reinterpret_cast<LPARAM>(&stMyFinish)) == GDT_NONE) {
      history_item.date_finish = Date();
    } else {
      history_item.date_finish = Date(stMyFinish.wYear, stMyFinish.wMonth, stMyFinish.wDay);
    }
  }

  // Alternative titles
  anime_item->SetUserSynonyms(GetDlgItemText(IDC_EDIT_ANIME_ALT));
  anime_item->SetUseAlternative(IsDlgButtonChecked(IDC_CHECK_ANIME_ALT) == TRUE);
  Meow.UpdateTitles(*anime_item, true);

  // Folder
  anime_item->SetFolder(GetDlgItemText(IDC_EDIT_ANIME_FOLDER));

  // Save settings
  Settings.Save();

  // Add item to queue
  History.queue.Add(history_item);
  return true;
}

}  // namespace ui