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

#include <windows/win/common_dialogs.h>

#include "base/format.h"
#include "base/string.h"
#include "media/anime_db.h"
#include "media/anime_util.h"
#include "media/library/queue.h"
#include "sync/anilist_ratings.h"
#include "sync/anilist_util.h"
#include "sync/kitsu_util.h"
#include "sync/myanimelist_util.h"
#include "sync/sync.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "track/feed_filter_manager.h"
#include "track/recognition.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_anime_info_page.h"
#include "ui/dlg/dlg_input.h"
#include "ui/dlg/dlg_settings.h"
#include "ui/dlg/dlg_settings_page.h"
#include "ui/dialog.h"
#include "ui/theme.h"
#include "ui/translate.h"
#include "ui/ui.h"

namespace ui {

PageBaseInfo::PageBaseInfo()
    : anime_id_(anime::ID_UNKNOWN), parent(nullptr) {
}

BOOL PageBaseInfo::OnClose() {
  if (parent && parent->GetMode() == AnimeDialogMode::AnimeInformation)
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

  if (!lpps)
    return;

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
      rect.Inflate(-ScaleX(kControlMargin), -ScaleY(kControlMargin));

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
      rect.Inflate(-ScaleX(kControlMargin), -ScaleY(kControlMargin));

      // Synonyms
      win::Rect rect_child;
      win::Window window = GetDlgItem(IDC_EDIT_ANIME_ALT);
      window.GetWindowRect(GetWindowHandle(), &rect_child);
      rect_child.right = rect.right - ScaleX(kControlMargin);
      window.SetPosition(nullptr, rect_child);

      // Details
      window.SetWindowHandle(GetDlgItem(IDC_STATIC_ANIME_DETAILS));
      window.GetWindowRect(GetWindowHandle(), &rect_child);
      rect_child.right = rect.right - ScaleX(kControlMargin);
      window.SetPosition(nullptr, rect_child);

      // Synopsis
      window.SetWindowHandle(GetDlgItem(IDC_EDIT_ANIME_SYNOPSIS));
      window.GetWindowRect(GetWindowHandle(), &rect_child);
      rect_child.right = rect.right - ScaleX(kControlMargin);
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
  auto anime_item = anime::db.Find(anime_id_);

  // Update window title
  parent->UpdateTitle(false);

  if (!anime_item)
    return;

  // Set alternative titles
  const auto main_title = anime::GetPreferredTitle(*anime_item);
  std::vector<std::wstring> titles;
  anime::GetAllTitles(anime_id_, titles);
  titles.erase(std::remove(titles.begin(), titles.end(), main_title), titles.end());
  std::wstring text = Join(titles, L", ");
  if (text.empty())
    text = L"-";
  SetDlgItemText(IDC_EDIT_ANIME_ALT, text.c_str());

  // Set information
  switch (sync::GetCurrentServiceId()) {
    case sync::ServiceId::Kitsu:
      SetDlgItemText(IDC_STATIC_ANIME_DETAILS_NAMES,
          L"Type:\nEpisodes:\nStatus:\nSeason:\nCategories:\nProducers:\nScore:");
      break;
  }
  auto genres = anime_item->GetGenres();
  const auto tags = anime_item->GetTags();
  genres.insert(genres.end(), tags.begin(), tags.end());
  const auto producers = anime::GetStudiosAndProducers(*anime_item);
  text = ui::TranslateType(anime_item->GetType()) + L"\n" +
         ui::TranslateNumber(anime_item->GetEpisodeCount(), L"Unknown") + L"\n" +
         ui::TranslateStatus(anime_item->GetAiringStatus()) + L"\n" +
         ui::TranslateDateToSeasonString(anime_item->GetDateStart()) + L"\n" +
         (genres.empty() ? L"Unknown" : Join(genres, L", ")) + L"\n" +
         (producers.empty() ? L"Unknown" : Join(producers, L", ")) + L"\n" +
         ui::TranslateScore(anime_item->GetScore());
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
  auto anime_item = anime::db.Find(anime_id_);

  if (!anime_item)
    return FALSE;

  switch (LOWORD(wParam)) {
    // Browse anime folder
    case IDC_BUTTON_BROWSE: {
      std::wstring default_path, path;
      const auto library_folders = taiga::settings.GetLibraryFolders();
      if (!anime_item->GetFolder().empty()) {
        default_path = anime_item->GetFolder();
      } else if (!library_folders.empty()) {
        default_path = library_folders.front();
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
        win::Spin spin = GetDlgItem(IDC_SPIN_ANIME_PROGRESS);
        int episode_value = 0;
        spin.GetPos32(episode_value);
        if (IsDlgButtonChecked(IDC_CHECK_ANIME_REWATCH)) {
          switch (sync::GetCurrentServiceId()) {
            case sync::ServiceId::Kitsu:
            case sync::ServiceId::AniList:
              combobox.SetCurSel(static_cast<int>(anime::MyStatus::Watching) - 1);
              break;
          }
          if (anime_item->GetMyStatus() == anime::MyStatus::Completed &&
              episode_value == anime_item->GetEpisodeCount())
            spin.SetPos32(0);
        } else {
          switch (sync::GetCurrentServiceId()) {
            case sync::ServiceId::Kitsu:
            case sync::ServiceId::AniList:
              combobox.SetCurSel(static_cast<int>(anime_item->GetMyStatus()) - 1);
              break;
          }
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
        if (combobox.GetItemData(combobox.GetCurSel()) == static_cast<int>(anime::MyStatus::Completed))
          if (anime_item->GetMyStatus() != anime::MyStatus::Completed &&
              anime_item->GetEpisodeCount() > 0)
            SendDlgItemMessage(IDC_SPIN_ANIME_PROGRESS, UDM_SETPOS32, 0, anime_item->GetEpisodeCount());
        combobox.SetWindowHandle(nullptr);
        return TRUE;
      }
      break;
  }

  return FALSE;
}

LRESULT PageMyInfo::OnNotify(int idCtrl, LPNMHDR pnmh) {
  auto anime_item = anime::db.Find(anime_id_);

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

    case IDC_SPIN_ANIME_SCORE:
      if (pnmh->code == UDN_DELTAPOS) {
        const std::pair<int, int> range;
        SendDlgItemMessage(IDC_SPIN_ANIME_SCORE, UDM_GETRANGE32,
                           reinterpret_cast<WPARAM>(&range.first),
                           reinterpret_cast<LPARAM>(&range.second));
        const auto nmud = *reinterpret_cast<LPNMUPDOWN>(pnmh);
        const int proposed_value = nmud.iPos + nmud.iDelta;
        if (range.first <= proposed_value && proposed_value <= range.second) {
          const auto value = ui::TranslateMyScore(proposed_value, L"0");
          SetDlgItemText(IDC_EDIT_ANIME_SCORE, value.c_str());
        }
      }
      break;

    case IDC_LINK_ANIME_FANSUB:
      switch (pnmh->code) {
        case NM_CLICK:
        case NM_RETURN: {
          // Set/change fansub group preference
          std::vector<std::wstring> groups;
          track::feed_filter_manager.GetFansubFilter(anime_id_, groups);
          if (groups.size() > 1) {
            ShowDlgSettings(kSettingsSectionTorrents, kSettingsPageTorrentsFilters);
            RefreshFansubPreference();
          } else {
            std::wstring text = Join(groups, L", ");
            InputDialog dlg;
            dlg.title = anime::GetPreferredTitle(*anime_item);
            dlg.info = L"Please enter your fansub group preference for this title:";
            dlg.text = text;
            dlg.Show(parent->GetWindowHandle());
            if (dlg.result == IDOK && dlg.text != text)
              if (track::feed_filter_manager.SetFansubFilter(anime_id_, dlg.text, L""))
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
  auto anime_item = anime::db.Find(anime_id_);

  if (!anime_item || !anime_item->IsInList())
    return;

  // Episodes watched
  win::Spin spin = GetDlgItem(IDC_SPIN_ANIME_PROGRESS);
  spin.SetRange32(0, anime_item->GetEpisodeCount() > 0 ? anime_item->GetEpisodeCount() : 9999);
  spin.SetPos32(anime_item->GetMyLastWatchedEpisode());
  spin.SetWindowHandle(nullptr);

  // Times rewatched
  spin.SetWindowHandle(GetDlgItem(IDC_SPIN_ANIME_REWATCHES));
  spin.SetRange32(0, 9999);
  spin.SetPos32(anime_item->GetMyRewatchedTimes());
  spin.SetWindowHandle(nullptr);

  // Rewatching
  CheckDlgButton(IDC_CHECK_ANIME_REWATCH, anime_item->GetMyRewatching());

  // Status
  win::ComboBox combobox = GetDlgItem(IDC_COMBO_ANIME_STATUS);
  if (combobox.GetCount() == 0) {
    for (const auto status : anime::kMyStatuses) {
      combobox.AddItem(ui::TranslateMyStatus(status, false).c_str(), static_cast<int>(status));
    }
  }
  combobox.SetCurSel(static_cast<int>(anime_item->GetMyStatus()) - 1);
  combobox.SetWindowHandle(nullptr);

  // Score
  combobox.SetWindowHandle(GetDlgItem(IDC_COMBO_ANIME_SCORE));
  win::Edit edit = GetDlgItem(IDC_EDIT_ANIME_SCORE);
  spin.SetWindowHandle(GetDlgItem(IDC_SPIN_ANIME_SCORE));
  if (combobox.GetCount() == 0) {
    std::vector<sync::Rating> ratings;
    std::wstring current_rating;
    int selected_item = -1;
    switch (sync::GetCurrentServiceId()) {
      case sync::ServiceId::MyAnimeList:
        ratings = sync::myanimelist::GetMyRatings();
        current_rating = sync::myanimelist::TranslateMyRating(anime_item->GetMyScore(), true);
        break;
      case sync::ServiceId::Kitsu: {
        const auto rating_system = sync::kitsu::GetRatingSystem();
        ratings = sync::kitsu::GetMyRatings(rating_system);
        current_rating = sync::kitsu::TranslateMyRating(anime_item->GetMyScore(), rating_system);
        break;
      }
      case sync::ServiceId::AniList: {
        const auto rating_system = sync::anilist::GetRatingSystem();
        ratings = sync::anilist::GetMyRatings(rating_system);
        current_rating = sync::anilist::TranslateMyRating(anime_item->GetMyScore(), rating_system);
        break;
      }
    }
    for (auto it = ratings.rbegin(); it != ratings.rend(); ++it) {
      const auto& rating = *it;
      combobox.AddItem(rating.text.c_str(), rating.value);
      if (selected_item == -1 && current_rating == rating.text)
        selected_item = combobox.GetCount() - 1;
    }
    if (selected_item > -1)
      combobox.SetCurSel(selected_item);
    edit.SetText(current_rating);
    spin.SetRange32(0, anime::kUserScoreMax);
    spin.SetPos32(anime_item->GetMyScore());
    const bool use_spin_for_score_input = IsAdvancedScoreInput();
    combobox.Show(!use_spin_for_score_input ? SW_SHOWNORMAL : SW_HIDE);
    edit.Show(use_spin_for_score_input ? SW_SHOWNORMAL : SW_HIDE);
    spin.Show(use_spin_for_score_input ? SW_SHOWNORMAL : SW_HIDE);
  }
  combobox.SetWindowHandle(nullptr);
  edit.SetWindowHandle(nullptr);
  spin.SetWindowHandle(nullptr);

  // Notes
  edit.SetWindowHandle(GetDlgItem(IDC_EDIT_ANIME_NOTES));
  edit.SetCueBannerText(L"Enter your notes about this anime");
  edit.SetText(anime_item->GetMyNotes());
  edit.SetWindowHandle(nullptr);

  // Dates
  start_date_changed_ = false;
  finish_date_changed_ = false;
  auto date_format = L"yyyy-MM-dd";
  SendDlgItemMessage(IDC_DATETIME_START, DTM_SETFORMAT, 0, (LPARAM)date_format);
  SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_SETFORMAT, 0, (LPARAM)date_format);
  auto set_default_systemtime = [&](int control_id, SYSTEMTIME& st) {
    SendDlgItemMessage(control_id, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&st);
  };
  if (anime::IsValidDate(anime_item->GetDateStart())) {
    auto stSeriesStart = static_cast<SYSTEMTIME>(anime_item->GetDateStart());
    set_default_systemtime(IDC_DATETIME_START, stSeriesStart);
    set_default_systemtime(IDC_DATETIME_FINISH, stSeriesStart);
  }
  if (anime::IsValidDate(anime_item->GetDateEnd())) {
    auto stSeriesEnd = static_cast<SYSTEMTIME>(anime_item->GetDateEnd());
    set_default_systemtime(IDC_DATETIME_FINISH, stSeriesEnd);
  }
  auto fix_systemtime = [](SYSTEMTIME& st) {
    if (!st.wMonth) st.wMonth = 1;
    if (!st.wDay) st.wDay = 1;
  };
  if (anime::IsValidDate(anime_item->GetMyDateStart())) {
    auto stMyStart = static_cast<SYSTEMTIME>(anime_item->GetMyDateStart());
    fix_systemtime(stMyStart);
    SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_VALID, (LPARAM)&stMyStart);
  } else {
    SendDlgItemMessage(IDC_DATETIME_START, DTM_SETSYSTEMTIME, GDT_NONE, 0);
  }
  if (anime::IsValidDate(anime_item->GetMyDateEnd())) {
    auto stMyFinish = static_cast<SYSTEMTIME>(anime_item->GetMyDateEnd());
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

  if (track::feed_filter_manager.GetFansubFilter(anime_id_, groups)) {
    for (const auto& group : groups) {
      if (!text.empty())
        text += L" or ";
      text += L"\"" + group + L"\"";
    }
  } else {
    text = L"None";
  }

  text = L"Fansub group preference: " + text + L" <a href=\"#\">(Change)</a>";
  SetDlgItemText(IDC_LINK_ANIME_FANSUB, text.c_str());
}

bool PageMyInfo::Save() {
  auto anime_item = anime::db.Find(anime_id_);

  if (!anime_item)
    return false;

  // Create item
  library::QueueItem queue_item;
  queue_item.anime_id = anime_id_;
  queue_item.mode = library::QueueItemMode::Update;

  // Episodes watched
  queue_item.episode = GetDlgItemInt(IDC_EDIT_ANIME_PROGRESS);
  if (!anime::IsValidEpisodeNumber(*queue_item.episode, anime_item->GetEpisodeCount())) {
    std::wstring msg = L"Please enter a valid episode number between 0-{}."_format(
        anime_item->GetEpisodeCount());
    MessageBox(msg.c_str(), L"Episodes watched", MB_OK | MB_ICONERROR);
    return false;
  }

  // Times rewatched
  queue_item.rewatched_times = GetDlgItemInt(IDC_EDIT_ANIME_REWATCHES);

  // Rewatching
  queue_item.enable_rewatching = IsDlgButtonChecked(IDC_CHECK_ANIME_REWATCH);

  // Score
  win::ComboBox combobox = GetDlgItem(IDC_COMBO_ANIME_SCORE);
  if (!IsAdvancedScoreInput()) {
    queue_item.score = static_cast<int>(combobox.GetItemData(combobox.GetCurSel()));
  } else {
    const auto score_text = GetDlgItemText(IDC_EDIT_ANIME_SCORE);
    switch (sync::GetCurrentServiceId()) {
      case sync::ServiceId::AniList:
        switch (sync::anilist::GetRatingSystem()) {
          case sync::anilist::RatingSystem::Point_10_Decimal:
            queue_item.score = static_cast<int>(ToDouble(score_text) * 10);
            break;
        }
        break;
    }
    if (!queue_item.score)
      queue_item.score = ToInt(score_text);
  }
  combobox.SetWindowHandle(nullptr);

  // Status
  queue_item.status = static_cast<anime::MyStatus>(
      GetComboSelection(IDC_COMBO_ANIME_STATUS) + 1);

  // Notes
  queue_item.notes = GetDlgItemText(IDC_EDIT_ANIME_NOTES);

  // Start date
  if (start_date_changed_) {
    SYSTEMTIME stMyStart = {0};
    if (SendDlgItemMessage(IDC_DATETIME_START, DTM_GETSYSTEMTIME, 0,
                           reinterpret_cast<LPARAM>(&stMyStart)) == GDT_NONE) {
      queue_item.date_start = Date();
    } else {
      queue_item.date_start = Date(stMyStart.wYear, stMyStart.wMonth, stMyStart.wDay);
    }
  }
  // Finish date
  if (finish_date_changed_) {
    SYSTEMTIME stMyFinish = {0};
    if (SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_GETSYSTEMTIME, 0,
                           reinterpret_cast<LPARAM>(&stMyFinish)) == GDT_NONE) {
      queue_item.date_finish = Date();
    } else {
      queue_item.date_finish = Date(stMyFinish.wYear, stMyFinish.wMonth, stMyFinish.wDay);
    }
  }

  // Alternative titles
  anime_item->SetUserSynonyms(GetDlgItemText(IDC_EDIT_ANIME_ALT));
  anime_item->SetUseAlternative(IsDlgButtonChecked(IDC_CHECK_ANIME_ALT) == TRUE);
  Meow.UpdateTitles(*anime_item, true);

  // Folder
  anime_item->SetFolder(GetDlgItemText(IDC_EDIT_ANIME_FOLDER));

  // Save settings
  taiga::settings.Save();

  // Add item to queue
  library::queue.Add(queue_item);
  return true;
}

bool PageMyInfo::IsAdvancedScoreInput() const {
  switch (sync::GetCurrentServiceId()) {
    case sync::ServiceId::AniList: {
      switch (sync::anilist::GetRatingSystem()) {
        case sync::anilist::RatingSystem::Point_100:
        case sync::anilist::RatingSystem::Point_10_Decimal:
          return true;
      }
      break;
    }
  }

  return false;
}

}  // namespace ui
