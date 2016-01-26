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
#include "base/process.h"
#include "library/anime_db.h"
#include "library/anime_util.h"
#include "library/history.h"
#include "library/resource.h"
#include "sync/sync.h"
#include "taiga/resource.h"
#include "taiga/script.h"
#include "taiga/settings.h"
#include "taiga/taiga.h"
#include "track/recognition.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dlg/dlg_anime_info_page.h"
#include "ui/dlg/dlg_main.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace ui {

AnimeDialog DlgAnime;
NowPlayingDialog DlgNowPlaying;

AnimeDialog::AnimeDialog()
    : anime_id_(anime::ID_UNKNOWN),
      current_page_(kAnimePageSeriesInfo),
      mode_(kDialogModeAnimeInformation) {
  image_label_.parent = this;
}

NowPlayingDialog::NowPlayingDialog() {
  current_page_ = kAnimePageNone;
  mode_ = kDialogModeNowPlaying;
}

////////////////////////////////////////////////////////////////////////////////

BOOL AnimeDialog::OnInitDialog() {
  if (mode_ == kDialogModeNowPlaying) {
    SetStyle(DS_CONTROL | WS_CHILD | WS_CLIPCHILDREN, WS_OVERLAPPEDWINDOW);
    SetStyle(0, WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, GWL_EXSTYLE);
    SetParent(DlgMain.GetWindowHandle());
  }

  // Initialize image label
  image_label_.Attach(GetDlgItem(IDC_STATIC_ANIME_IMG));

  // Initialize title
  edit_title_.Attach(GetDlgItem(IDC_EDIT_ANIME_TITLE));
  edit_title_.SendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(ui::Theme.GetHeaderFont()), FALSE);

  // Initialize
  sys_link_.Attach(GetDlgItem(IDC_LINK_NOWPLAYING));
  sys_link_.Hide();

  // Initialize tabs
  tab_.Attach(GetDlgItem(IDC_TAB_ANIME));
  switch (mode_) {
    case kDialogModeAnimeInformation:
      tab_.InsertItem(0, L"Main information", 0);
      tab_.InsertItem(1, L"My list and settings", 0);
      break;
    case kDialogModeNowPlaying:
      tab_.Hide();
      break;
  }

  // Initialize pages
  page_series_info.parent = this;
  page_my_info.parent = this;
  page_series_info.Create(IDD_ANIME_INFO_PAGE01, GetWindowHandle(), false);
  switch (mode_) {
    case kDialogModeAnimeInformation:
      page_my_info.Create(IDD_ANIME_INFO_PAGE02, GetWindowHandle(), false);
      EnableThemeDialogTexture(page_series_info.GetWindowHandle(), ETDT_ENABLETAB);
      EnableThemeDialogTexture(page_my_info.GetWindowHandle(), ETDT_ENABLETAB);
      break;
    case kDialogModeNowPlaying:
      break;
  }

  // Refresh
  SetCurrentPage(current_page_);
  Refresh();

  return TRUE;
}


void AnimeDialog::OnCancel() {
  EndDialog(IDCANCEL);
  ActivateWindow(DlgMain.GetWindowHandle());
}

void AnimeDialog::OnOK() {
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  if (anime_item && anime_item->IsInList())
    if (!page_my_info.Save())
      return;

  EndDialog(IDOK);
  ActivateWindow(DlgMain.GetWindowHandle());
}

////////////////////////////////////////////////////////////////////////////////

BOOL AnimeDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC: {
      win::Dc dc = reinterpret_cast<HDC>(wParam);
      HWND hwnd_control = reinterpret_cast<HWND>(lParam);
      dc.SetBkMode(TRANSPARENT);
      if (hwnd_control == GetDlgItem(IDC_EDIT_ANIME_TITLE))
        dc.SetTextColor(ui::kColorMainInstruction);
      dc.DetachDc();
      if (hwnd_control == GetDlgItem(IDC_EDIT_ANIME_TITLE))
        return reinterpret_cast<INT_PTR>(Theme.GetBackgroundBrush());
      return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
    }

    case WM_DRAWITEM: {
      // Draw anime image
      if (wParam == IDC_STATIC_ANIME_IMG) {
        LPDRAWITEMSTRUCT dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        win::Rect rect = dis->rcItem;
        win::Dc dc = dis->hDC;
        // Paint border
        dc.FillRect(rect, ::GetSysColor(COLOR_ACTIVEBORDER));
        rect.Inflate(-1, -1);
        dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));
        rect.Inflate(-1, -1);
        // Paint image
        auto image = ImageDatabase.GetImage(anime_id_);
        if (anime::IsValidId(anime_id_) && image) {
          dc.SetStretchBltMode(HALFTONE);
          dc.StretchBlt(rect.left, rect.top, rect.Width(), rect.Height(),
                        image->dc.Get(),
                        0, 0, image->rect.Width(), image->rect.Height(),
                        SRCCOPY);
        } else {
          dc.EditFont(nullptr, 64, TRUE);
          dc.SetBkMode(TRANSPARENT);
          dc.SetTextColor(::GetSysColor(COLOR_ACTIVEBORDER));
          dc.DrawText(L"?", 1, rect, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
          DeleteObject(dc.DetachFont());
        }
        dc.DetachDc();
        return TRUE;
      }
      break;
    }
  }

  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT AnimeDialog::ImageLabel::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_SETCURSOR: {
      if (anime::IsValidId(parent->anime_id_)) {
        SetSharedCursor(IDC_HAND);
        return TRUE;
      }
      break;
    }
  }

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT AnimeDialog::EditTitle::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_SETFOCUS: {
      WindowProcDefault(hwnd, uMsg, wParam, lParam);
      SetSel(0, 0);
      HideCaret();
      return 0;
    }
  }

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL AnimeDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  if (LOWORD(wParam) == IDC_STATIC_ANIME_IMG &&
      HIWORD(wParam) == STN_CLICKED) {
    if (anime::IsValidId(anime_id_)) {
      ExecuteAction(L"ViewAnimePage", 0, anime_id_);
      return TRUE;
    }
  }

  return FALSE;
}

LRESULT AnimeDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (idCtrl) {
    case IDC_LINK_NOWPLAYING: {
      switch (pnmh->code) {
        // Link click
        case NM_CLICK:
        case NM_RETURN: {
          PNMLINK pNMLink = reinterpret_cast<PNMLINK>(pnmh);
          std::wstring action = pNMLink->item.szUrl;
          if (IsEqual(pNMLink->item.szID, L"menu")) {
            action = ui::Menus.Show(GetWindowHandle(), 0, 0, pNMLink->item.szUrl);
          } else if (IsEqual(pNMLink->item.szID, L"search")) {
            action = L"SearchAnime(" + CurrentEpisode.anime_title() + L")";
          } else if (IsEqual(pNMLink->item.szUrl, L"score")) {
            action = L"";
            anime::LinkEpisodeToAnime(CurrentEpisode, ToInt(pNMLink->item.szID));
          }
          if (!action.empty())
            ExecuteAction(action, 0, GetCurrentId());
          return TRUE;
        }

        // Custom draw
        case NM_CUSTOMDRAW: {
          return CDRF_DODEFAULT;
        }
      }
      break;
    }

    case IDC_TAB_ANIME: {
      switch (pnmh->code) {
        // Tab select
        case TCN_SELCHANGE: {
          SetCurrentPage(tab_.GetCurrentlySelected() + 1);
          break;
        }
      }
      break;
    }
  }

  return 0;
}

void AnimeDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  win::Dc dc = hdc;
  win::Rect rect;

  // Paint background
  rect.Copy(lpps->rcPaint);
  dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));
}

void AnimeDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      UpdateControlPositions(&size);
      break;
    }
  }
}

BOOL AnimeDialog::PreTranslateMessage(MSG* pMsg) {
  if (pMsg->message == WM_KEYDOWN) {
    switch (pMsg->wParam) {
      // Switch tabs
      case VK_TAB:
        if (IsTabVisible()) {
          if (GetKeyState(VK_CONTROL) & 0x8000) {
            if (GetKeyState(VK_SHIFT) & 0x8000) {
              GoToPreviousTab();
            } else {
              GoToNextTab();
            }
            return TRUE;
          }
        }
        break;
      // Refresh
      case VK_F5:
        page_my_info.Refresh(anime_id_);
        page_series_info.Refresh(anime_id_, false);
        if (anime::IsValidId(anime_id_)) {
          UpdateTitle(true);
          auto anime_item = AnimeDatabase.FindItem(anime_id_);
          if (anime_item) {
            sync::GetMetadataById(anime_id_);
            sync::DownloadImage(anime_id_, anime_item->GetImageUrl());
          }
        }
        return TRUE;
    }
  }

  if (mode_ == kDialogModeAnimeInformation) {
    if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST) {
      if (!IsModal() && IsDialogMessage(GetWindowHandle(), pMsg)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}

LRESULT AnimeDialog::Tab::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_PAINT: {
      if (::GetUpdateRect(hwnd, NULL, FALSE)) {
        PAINTSTRUCT ps;
        HDC hdc = ::BeginPaint(hwnd, &ps);
        OnPaint(hdc, &ps);
        ::EndPaint(hwnd, &ps);
      } else {
        HDC hdc = ::GetDC(hwnd);
        OnPaint(hdc, NULL);
        ::ReleaseDC(hwnd, hdc);
      }
      break;
    }
  }

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

void AnimeDialog::Tab::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  // Adapted from Paul Sanders' example code, located at:
  // http://www.glennslayden.com/code/win32/tab-control-background-brush

  ::CallWindowProc(prev_window_proc_, GetWindowHandle(), WM_PRINTCLIENT,
                   reinterpret_cast<WPARAM>(hdc), PRF_CLIENT);

  HRGN region = ::CreateRectRgn(0, 0, 0, 0);
  RECT rect, lh_corner = {0}, rh_corner = {0};

  int item_count = GetItemCount();
  int current_item = GetCurrentlySelected();
  int tab_height = 0;

  bool is_vista = win::GetVersion() >= win::kVersionVista;
  bool is_themed_xp = !is_vista && ::IsThemeActive();

  for (int i = 0; i < item_count; ++i) {
    TabCtrl_GetItemRect(GetWindowHandle(), i, &rect);
    if (i == current_item) {
      tab_height = (rect.bottom - rect.top) + 2;
      rect.left -= 1;
      rect.right += 1;
      rect.top -= 2;
      if (i == 0) {
        rect.left -= 1;
        if (!is_themed_xp)
          rect.right += 1;
      }
      if (i == item_count - 1)
        rect.right += 1;
    } else {
      rect.right -= 1;
      if ((is_themed_xp || is_vista) && i == item_count - 1)
        rect.right -= 1;
    }

    if (is_themed_xp) {
      if (i != current_item + 1) {
        lh_corner = rect;
        lh_corner.bottom = lh_corner.top + 1;
        lh_corner.right = lh_corner.left + 1;
      }
      rh_corner = rect;
      rh_corner.bottom = rh_corner.top + 1;
      rh_corner.left = rh_corner.right - 1;
    }

    HRGN tab_region = ::CreateRectRgn(rect.left, rect.top, rect.right, rect.bottom);
    ::CombineRgn(region, region, tab_region, RGN_OR);
    ::DeleteObject(tab_region);

    if (lh_corner.right > lh_corner.left) {
      HRGN rounded_corner = ::CreateRectRgn(
        lh_corner.left, lh_corner.top, lh_corner.right, lh_corner.bottom);
      ::CombineRgn(region, region, rounded_corner, RGN_DIFF);
      ::DeleteObject(rounded_corner);
    }
    if (rh_corner.right > rh_corner.left) {
      HRGN rounded_corner = ::CreateRectRgn(
        rh_corner.left, rh_corner.top, rh_corner.right, rh_corner.bottom);
      ::CombineRgn(region, region, rounded_corner, RGN_DIFF);
      ::DeleteObject(rounded_corner);
    }
  }

  GetClientRect(&rect);
  HRGN fill_region = ::CreateRectRgn(
    rect.left, rect.top, rect.right, rect.top + tab_height);
  ::CombineRgn(fill_region, fill_region, region, RGN_DIFF);
  ::SelectClipRgn(hdc, fill_region);
  HBRUSH hBGBrush = ::GetSysColorBrush(COLOR_WINDOW);
  ::FillRgn(hdc, fill_region, hBGBrush);
  ::DeleteObject(fill_region);
  ::DeleteObject(region);
}

////////////////////////////////////////////////////////////////////////////////

bool AnimeDialog::IsTabVisible() const {
  return tab_.IsVisible() != FALSE;
}

void AnimeDialog::GoToPreviousTab() {
  SetCurrentPage(current_page_ == kAnimePageSeriesInfo ?
      kAnimePageMyInfo : kAnimePageSeriesInfo);
}

void AnimeDialog::GoToNextTab() {
  GoToPreviousTab();
}

int AnimeDialog::GetMode() const {
  return mode_;
}

int AnimeDialog::GetCurrentId() const {
  return anime_id_;
}

void AnimeDialog::SetCurrentId(int anime_id) {
  anime_id_ = anime_id;

  switch (anime_id_) {
    case anime::ID_NOTINLIST:
      SetCurrentPage(kAnimePageNotRecognized);
      break;
    case anime::ID_UNKNOWN:
      SetCurrentPage(kAnimePageNone);
      break;
    default:
      SetCurrentPage(kAnimePageSeriesInfo);
      break;
  }

  Refresh();
}

void AnimeDialog::SetCurrentPage(int index) {
  const auto previous_page = current_page_;
  current_page_ = index;

  if (!IsWindow())
    return;

  auto anime_item = AnimeDatabase.FindItem(anime_id_);
  bool anime_in_list = anime_item && anime_item->IsInList();

  switch (index) {
    case kAnimePageNone:
      image_label_.Hide();
      page_my_info.Hide();
      page_series_info.Hide();
      sys_link_.Show();
      break;
    case kAnimePageSeriesInfo:
      image_label_.Show();
      page_my_info.Hide();
      page_series_info.Show();
      sys_link_.Show(mode_ == kDialogModeNowPlaying || !anime_in_list);
      break;
    case kAnimePageMyInfo:
      image_label_.Show();
      page_series_info.Hide();
      page_my_info.Show();
      sys_link_.Hide();
      break;
    case kAnimePageNotRecognized:
      image_label_.Show();
      page_my_info.Hide();
      page_series_info.Hide();
      sys_link_.Show();
      break;
  }

  if (previous_page != current_page_) {
    const HWND hwnd = GetFocus();
    if (::IsWindow(hwnd) && !::IsWindowVisible(hwnd)) {
      switch (current_page_) {
        case kAnimePageNone:
          sys_link_.SetFocus();
          break;
        case kAnimePageSeriesInfo:
          page_series_info.SetFocus();
          break;
        case kAnimePageMyInfo:
          page_my_info.SetFocus();
          break;
        case kAnimePageNotRecognized:
          sys_link_.SetFocus();
          break;
      }
    }
  }

  tab_.SetCurrentlySelected(index - 1);

  int show = SW_SHOW;
  if (mode_ == kDialogModeNowPlaying || !anime_in_list) {
    show = SW_HIDE;
  }
  ShowDlgItem(IDOK, show);
  ShowDlgItem(IDCANCEL, show);
}

void AnimeDialog::SetScores(const sorted_scores_t& scores) {
  scores_ = scores;
}

void AnimeDialog::Refresh(bool image, bool series_info, bool my_info, bool connect) {
  if (!IsWindow())
    return;

  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  // Load image
  if (image) {
    ImageDatabase.Load(anime_id_, true, connect);
    win::Rect rect;
    GetClientRect(&rect);
    SIZE size = {rect.Width(), rect.Height()};
    OnSize(WM_SIZE, 0, size);
    RedrawWindow();
  }

  // Set title
  if (anime_item) {
    if (Settings.GetBool(taiga::kApp_List_DisplayEnglishTitles)) {
      edit_title_.SetText(anime_item->GetEnglishTitle(true));
    } else {
      edit_title_.SetText(anime_item->GetTitle());
    }
  } else if (anime_id_ == anime::ID_NOTINLIST) {
    edit_title_.SetText(CurrentEpisode.anime_title());
  } else {
    edit_title_.SetText(L"Now Playing");
  }

  // Set content
  if (anime_id_ == anime::ID_NOTINLIST) {
    std::wstring content = L"Taiga was unable to identify this title, and it needs your help.\n\n";
    if (!scores_.empty()) {
      int count = 0;
      content += L"Please choose the correct one from the list below:\n\n";
      foreach_c_(it, scores_) {
        content += L"  \u2022 <a href=\"score\" id=\"" + ToWstr(it->first) + L"\">" +
                   AnimeDatabase.items[it->first].GetTitle() + L"</a>";
        if (Taiga.debug_mode)
          content += L" [Score: " + ToWstr(it->second) + L"]";
        content += L"\n";
        if (++count >= 10)
          break;
      }
      content += L"\nNot in the list? <a id=\"search\">Search</a> for more.";
    } else {
      content += L"<a id=\"search\">Search</a> for this title.";
    }
    sys_link_.SetText(content);

  } else if (anime_id_ == anime::ID_UNKNOWN) {
    std::wstring content;
    Date date_now = GetDate();
    int date_diff = 0;
    const int day_limit = 7;
    int link_count = 0;

    // Recently watched
    std::vector<int> anime_ids;
    auto list_anime_ids = [&anime_ids](const std::vector<HistoryItem>& items) {
      foreach_cr_(it, items) {
        if (it->episode) {
          if (std::find(anime_ids.begin(), anime_ids.end(),
                        it->anime_id) == anime_ids.end()) {
            auto anime_item = AnimeDatabase.FindItem(it->anime_id);
            if (anime_item)
              if (anime_item->GetMyStatus() == anime::kWatching || anime_item->GetMyRewatching())
                anime_ids.push_back(it->anime_id);
          }
        }
      }
    };
    list_anime_ids(History.queue.items);
    list_anime_ids(History.items);
    int recently_watched = 0;
    foreach_c_(it, anime_ids) {
      auto anime_item = AnimeDatabase.FindItem(*it);
      if (!anime_item || !anime_item->IsNextEpisodeAvailable())
        continue;
      std::wstring title = anime_item->GetTitle() + L" #" + ToWstr(anime_item->GetMyLastWatchedEpisode() + 1);
      content += L"\u2022 <a href=\"PlayNext(" + ToWstr(*it) + L")\">" + title + L"</a>\n";
      link_count++;
      recently_watched++;
      if (recently_watched >= 20)
        break;
    }
    if (content.empty()) {
      content = L"Continue Watching:\n"
                L"<a href=\"ScanEpisodesAll()\">Scan available episodes</a> to see recently watched anime. "
                L"Or how about you <a href=\"PlayRandomAnime()\">try a random one</a>?\n\n";
      link_count += 2;
    } else {
      content = L"Continue Watching:\n" + content + L"\n";
    }
    int watched_last_week = 0;
    foreach_c_(it, History.queue.items) {
      if (!it->episode || *it->episode == 0)
        continue;
      date_diff = date_now - (Date)(it->time.substr(0, 10));
      if (date_diff <= day_limit)
        watched_last_week++;
    }
    foreach_c_(it, History.items) {
      if (!it->episode || *it->episode == 0)
        continue;
      date_diff = date_now - (Date)(it->time.substr(0, 10));
      if (date_diff <= day_limit)
        watched_last_week++;
    }
    if (watched_last_week > 0) {
      content += L"You've watched " + ToWstr(watched_last_week) + L" ";
      content += watched_last_week == 1 ? L"episode" : L"episodes";
      content += L" in the last week.\n\n";
    }

    // Available episodes
    int available_episodes = 0;
    foreach_c_(it, AnimeDatabase.items) {
      if (it->second.IsInList() && it->second.IsNextEpisodeAvailable())
        available_episodes++;
    }
    if (available_episodes > 0) {
      content += L"There are at least " + ToWstr(available_episodes) + L" new ";
      content += available_episodes == 1 ? L"episode" : L"episodes";
      content += L" available in library folders.\n\n";
    }

    // Airing times
    std::vector<int> recently_started, recently_finished, upcoming;
    foreach_c_(it, AnimeDatabase.items) {
      if (it->second.GetMyStatus() != anime::kPlanToWatch)
        continue;
      const Date& date_start = it->second.GetDateStart();
      const Date& date_end = it->second.GetDateEnd();
      if (date_start.year && date_start.month && date_start.day) {
        date_diff = date_now - date_start;
        if (date_diff > 0 && date_diff <= day_limit) {
          recently_started.push_back(it->first);
          continue;
        }
        date_diff = date_start - date_now;
        if (date_diff > 0 && date_diff <= day_limit) {
          upcoming.push_back(it->first);
          continue;
        }
      }
      if (date_end.year && date_end.month && date_end.day) {
        date_diff = date_now - date_end;
        if (date_diff > 0 && date_diff <= day_limit) {
          recently_finished.push_back(it->first);
          continue;
        }
      }
    }
    
    auto add_info_lines = [&](const std::vector<int>& ids) {
      std::wstring text;
      for (const auto& id : ids) {
        auto title = AnimeDatabase.FindItem(id)->GetTitle();
        AppendString(text, L"<a href=\"Info(" + ToWstr(id) + L")\">" + title + L"</a>", L"  \u2022  ");
        link_count++;
      }
      content += text;
    };
    if (!recently_started.empty()) {
      content += L"Recently Started Airing:\n";
      add_info_lines(recently_started);
      content += L"\n\n";
    }
    if (!recently_finished.empty()) {
      content += L"Recently Finished Airing:\n";
      add_info_lines(recently_finished);
      content += L"\n\n";
    }
    if (!upcoming.empty()) {
      content += L"Upcoming:\n";
      add_info_lines(upcoming);
      content += L"\n\n";
    } else {
      content += L"<a href=\"ViewUpcomingAnime()\">View upcoming anime</a>";
      link_count++;
    }

    sys_link_.SetText(content);

    /*
    for (int i = 0; i < link_count; i++)
      sys_link_.SetItemState(i, LIS_ENABLED | LIS_HOTTRACK | LIS_DEFAULTCOLORS);
    */

  } else {
    std::wstring content;
    if (mode_ == kDialogModeNowPlaying) {
      content += L"Now playing: Episode " + anime::GetEpisodeRange(CurrentEpisode);
      if (!CurrentEpisode.release_group().empty())
        content += L" by " + CurrentEpisode.release_group();
      content += L"\n";
    }
    if (anime_item && anime_item->IsInList()) {
      content += L"<a href=\"EditAll(" + ToWstr(anime_id_) + L")\">Edit</a>";
    } else {
      int status = anime::kPlanToWatch;
      if (mode_ == kDialogModeNowPlaying || CurrentEpisode.anime_id == anime_id_)
        status = anime::kWatching;
      content += L"<a href=\"AddToList(" + ToWstr(status) + L")\">Add to list</a>";
    }
    if (anime_item && mode_ == kDialogModeNowPlaying) {
      content += L" \u2022 <a id=\"menu\" href=\"Announce\">Share</a>";
      int episode_number = anime::GetEpisodeHigh(CurrentEpisode);
      if (episode_number == 0)
        episode_number = 1;
      if (!anime::IsValidEpisodeCount(anime_item->GetEpisodeCount()) ||
          anime_item->GetEpisodeCount() > episode_number) {
        content += L" \u2022 <a href=\"PlayEpisode(" + ToWstr(episode_number + 1) + L"\">Watch next episode</a>";
      }
    }
    sys_link_.SetText(content);
  }

  // Toggle tabs
  if (anime_item && anime_item->IsInList() &&
      mode_ == kDialogModeAnimeInformation) {
    tab_.Show();
  } else {
    tab_.Hide();
  }

  // Refresh pages
  if (series_info)
    page_series_info.Refresh(anime_id_, connect);
  if (my_info)
    page_my_info.Refresh(anime_id_);
  SetCurrentPage(current_page_);

  // Update controls
  UpdateControlPositions();
}

void AnimeDialog::UpdateControlPositions(const SIZE* size) {
  win::Rect rect;
  if (size == nullptr) {
    GetClientRect(&rect);
  } else {
    rect.Set(0, 0, size->cx, size->cy);
  }

  rect.Inflate(-ScaleX(win::kControlMargin) * 2,
               -ScaleY(win::kControlMargin) * 2);

  // Image
  if (current_page_ != kAnimePageNone) {
    win::Rect rect_image = rect;
    rect_image.right = rect_image.left + ScaleX(150);
    auto image = ImageDatabase.GetImage(anime_id_);
    if (image) {
      rect_image = ResizeRect(rect_image,
                              image->rect.Width(), image->rect.Height(),
                              true, true, false);
    } else {
      rect_image.bottom = rect_image.top + static_cast<int>(rect_image.Width() * 1.4);
    }
    image_label_.SetPosition(nullptr, rect_image);
    rect.left = rect_image.right + ScaleX(win::kControlMargin) * 2;
  }

  // Title
  win::Rect rect_title;
  edit_title_.GetWindowRect(&rect_title);
  rect_title.Set(rect.left, rect.top,
                 rect.right, rect.top + rect_title.Height());
  edit_title_.SetPosition(nullptr, rect_title);
  rect.top = rect_title.bottom + ScaleY(win::kControlMargin);

  // Buttons
  if (mode_ == kDialogModeAnimeInformation) {
    win::Rect rect_button;
    ::GetWindowRect(GetDlgItem(IDOK), &rect_button);
    rect.bottom -= rect_button.Height() + ScaleY(win::kControlMargin) * 2;
  }

  // Content
  auto anime_item = AnimeDatabase.FindItem(anime_id_);
  bool anime_in_list = anime_item && anime_item->IsInList();
  if (mode_ == kDialogModeNowPlaying && !anime::IsValidId(anime_id_)) {
    rect.left += ScaleX(win::kControlMargin);
    sys_link_.SetPosition(nullptr, rect);
  } else if (mode_ == kDialogModeNowPlaying || !anime_in_list) {
    win::Dc dc = sys_link_.GetDC();
    dc.AttachFont(sys_link_.GetFont());
    const int text_height = GetTextHeight(dc.Get());
    dc.DetachFont();
    dc.DetachDc();
    int line_count = mode_ == kDialogModeNowPlaying ? 2 : 1;
    win::Rect rect_content = rect;
    rect_content.Inflate(-ScaleX(win::kControlMargin * 2), 0);
    rect_content.bottom = rect_content.top + text_height * line_count;
    sys_link_.SetPosition(nullptr, rect_content);
    rect.top = rect_content.bottom + ScaleY(win::kControlMargin) * 3;
  }

  // Pages
  win::Rect rect_page = rect;
  if (tab_.IsVisible()) {
    tab_.SetPosition(nullptr, rect_page);
    tab_.AdjustRect(GetWindowHandle(), FALSE, &rect_page);
    rect_page.Inflate(-ScaleX(win::kControlMargin), -ScaleY(win::kControlMargin));
  }
  page_series_info.SetPosition(nullptr, rect_page);
  page_my_info.SetPosition(nullptr, rect_page);
}

void AnimeDialog::UpdateTitle(bool refreshing) {
  if (refreshing) {
    SetText(L"Anime Information (Refreshing...)");
  } else {
    SetText(L"Anime Information");
  }
}

}  // namespace ui
