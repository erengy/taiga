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

#include "../anime_db.h"
#include "../common.h"
#include "../foreach.h"
#include "../gfx.h"
#include "../history.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"

class AnimeDialog AnimeDialog;
class NowPlayingDialog NowPlayingDialog;

// =============================================================================

AnimeDialog::AnimeDialog()
    : anime_id_(anime::ID_UNKNOWN),
      current_page_(INFOPAGE_SERIESINFO),
      mode_(DIALOG_MODE_ANIME_INFORMATION) {
}

NowPlayingDialog::NowPlayingDialog() {
  current_page_ = INFOPAGE_NONE;
  mode_ = DIALOG_MODE_NOW_PLAYING;
}

// =============================================================================

BOOL AnimeDialog::OnInitDialog() {
  if (mode_ == DIALOG_MODE_NOW_PLAYING) {
    SetStyle(DS_CONTROL | WS_CHILD | WS_CLIPCHILDREN, WS_OVERLAPPEDWINDOW);
    SetStyle(0, WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, GWL_EXSTYLE);
    SetParent(g_hMain);
  }

  // Initialize image label
  image_label_.Attach(GetDlgItem(IDC_STATIC_ANIME_IMG));

  // Initialize title
  edit_title_.Attach(GetDlgItem(IDC_EDIT_ANIME_TITLE));
  edit_title_.SendMessage(WM_SETFONT, reinterpret_cast<WPARAM>(UI.font_header.Get()), FALSE);

  // Initialize
  sys_link_.Attach(GetDlgItem(IDC_LINK_NOWPLAYING));
  sys_link_.Hide();
  
  // Initialize tabs
  tab_.Attach(GetDlgItem(IDC_TAB_ANIME));
  switch (mode_) {
    case DIALOG_MODE_ANIME_INFORMATION:
      tab_.InsertItem(0, L"Main information", 0);
      if (AnimeDatabase.FindItem(anime_id_)->IsInList())
        tab_.InsertItem(1, L"My list and settings", 0);
      break;
    case DIALOG_MODE_NOW_PLAYING:
      tab_.Hide();
      break;
  }

  // Initialize pages
  page_series_info.parent = this;
  page_my_info.parent = this;
  page_series_info.Create(IDD_ANIME_INFO_PAGE01, m_hWindow, false);
  switch (mode_) {
    case DIALOG_MODE_ANIME_INFORMATION:
      page_my_info.Create(IDD_ANIME_INFO_PAGE02, m_hWindow, false);
      EnableThemeDialogTexture(page_series_info.GetWindowHandle(), ETDT_ENABLETAB);
      EnableThemeDialogTexture(page_my_info.GetWindowHandle(), ETDT_ENABLETAB);
      break;
    case DIALOG_MODE_NOW_PLAYING:
      break;
  }

  // Initialize buttons
  int show = SW_SHOW;
  if (mode_ == DIALOG_MODE_NOW_PLAYING ||
      !AnimeDatabase.FindItem(anime_id_)->IsInList()) {
    show = SW_HIDE;
  }
  ShowDlgItem(IDOK, show);
  ShowDlgItem(IDCANCEL, show);

  // Refresh
  SetCurrentPage(current_page_);
  Refresh();

  return TRUE;
}

void AnimeDialog::OnOK() {
  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  if (anime_item->IsInList())
    if (!page_my_info.Save())
      return;

  EndDialog(IDOK);
}

// =============================================================================

BOOL AnimeDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      HWND hwnd_control = reinterpret_cast<HWND>(lParam);
      if (hwnd_control == GetDlgItem(IDC_EDIT_ANIME_TITLE)) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, theme::COLOR_MAININSTRUCTION);
      }
      return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
    }

    case WM_DRAWITEM: {
      // Draw anime image
      if (wParam == IDC_STATIC_ANIME_IMG) {
        LPDRAWITEMSTRUCT dis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        win32::Rect rect = dis->rcItem;
        win32::Dc dc = dis->hDC;
        // Paint border
        dc.FillRect(rect, ::GetSysColor(COLOR_ACTIVEBORDER));
        rect.Inflate(-1, -1);
        dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));
        rect.Inflate(-1, -1);
        // Paint image
        dc.FillRect(rect, ::GetSysColor(COLOR_WINDOW));
        auto image = ImageDatabase.GetImage(anime_id_);
        if (anime_id_ > anime::ID_UNKNOWN && image) {
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
        dc.DetachDC();
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
      ::SetCursor(reinterpret_cast<HCURSOR>(
        ::LoadImage(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_SHARED)));
      return TRUE;
    }
  }
  
  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL AnimeDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  if (LOWORD(wParam) == IDC_STATIC_ANIME_IMG &&
      HIWORD(wParam) == STN_CLICKED) {
    mal::ViewAnimePage(anime_id_);
    return TRUE;
  }
  
  return FALSE;
}

LRESULT AnimeDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  switch (idCtrl) {
    case IDC_LINK_NOWPLAYING: {
      switch (pnmh->code) {
        // Link click
        case NM_CLICK: {
          PNMLINK pNMLink = reinterpret_cast<PNMLINK>(pnmh);
          wstring action = pNMLink->item.szUrl;
          if (IsEqual(pNMLink->item.szID, L"menu"))
            action = UI.Menus.Show(m_hWindow, 0, 0, pNMLink->item.szUrl);
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
  win32::Dc dc = hdc;
  win32::Rect rect;

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
    // Close window
    if (pMsg->wParam == VK_ESCAPE) {
      if (mode_ == DIALOG_MODE_ANIME_INFORMATION) {
        Destroy();
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

  ::CallWindowProc(m_PrevWindowProc, m_hWindow, WM_PRINTCLIENT, 
                   reinterpret_cast<WPARAM>(hdc), PRF_CLIENT);

  HRGN region = ::CreateRectRgn(0, 0, 0, 0);
  RECT rect, lh_corner = {0}, rh_corner = {0};

  int item_count = GetItemCount();
  int current_item = GetCurrentlySelected();
  int tab_height = 0;
  
  bool is_vista = win32::GetWinVersion() >= win32::VERSION_VISTA;
  bool is_themed_xp = !is_vista && ::IsThemeActive();

  for (int i = 0; i < item_count; ++i) {
    TabCtrl_GetItemRect(m_hWindow, i, &rect);
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

// =============================================================================

bool AnimeDialog::IsTabVisible() const {
  return tab_.IsVisible() != FALSE;
}

int AnimeDialog::GetCurrentId() const {
  return anime_id_;
}

void AnimeDialog::SetCurrentId(int anime_id) {
  anime_id_ = anime_id;
  
  switch (anime_id_) {
    case anime::ID_UNKNOWN:
      SetCurrentPage(INFOPAGE_NONE);
      break;
    default:
      SetCurrentPage(INFOPAGE_SERIESINFO);
      break;
  }

  Refresh();
}

void AnimeDialog::SetCurrentPage(int index) {
  current_page_ = index;
  
  if (IsWindow()) {
    switch (index) {
      case INFOPAGE_NONE:
        image_label_.Hide();
        page_my_info.Hide();
        page_series_info.Hide();
        sys_link_.Show();
        break;
      case INFOPAGE_SERIESINFO:
        image_label_.Show();
        page_my_info.Hide();
        page_series_info.Show();
        sys_link_.Show(mode_ == DIALOG_MODE_NOW_PLAYING);
        break;
      case INFOPAGE_MYINFO:
        image_label_.Show();
        page_series_info.Hide();
        page_my_info.Show();
        sys_link_.Hide();
        break;
    }
    tab_.SetCurrentlySelected(index - 1);
  }
}

void AnimeDialog::Refresh(bool image, bool series_info, bool my_info, bool connect) {
  if (!IsWindow()) return;

  auto anime_item = AnimeDatabase.FindItem(anime_id_);

  // Load image
  if (image) {
    ImageDatabase.Load(anime_id_, true, connect);
    win32::Rect rect;
    GetClientRect(&rect);
    SIZE size = {rect.Width(), rect.Height()};
    OnSize(WM_SIZE, 0, size);
    InvalidateRect();
  }

  // Set title
  if (anime_item) {
    if (Settings.Program.List.english_titles) {
      SetDlgItemText(IDC_EDIT_ANIME_TITLE, anime_item->GetEnglishTitle(true).c_str());
    } else {
      SetDlgItemText(IDC_EDIT_ANIME_TITLE, anime_item->GetTitle().c_str());
    }
  } else if (anime_id_ == anime::ID_NOTINLIST) {
    SetDlgItemText(IDC_EDIT_ANIME_TITLE, CurrentEpisode.title.c_str());
  } else {
    SetDlgItemText(IDC_EDIT_ANIME_TITLE, L"Now Playing");
  }

  // Set content
  if (anime_id_ == anime::ID_UNKNOWN) {
    wstring content;
    Date date_now = GetDate();
    int date_diff = 0;
    const int day_limit = 7;
    int link_count = 0;

    // Recently watched
    vector<int> anime_ids;
    foreach_cr_(it, History.queue.items) {
      if (it->episode &&
          std::find(anime_ids.begin(), anime_ids.end(), it->anime_id) == anime_ids.end()) {
        auto anime_item = AnimeDatabase.FindItem(it->anime_id);
        if (anime_item->GetMyStatus() != mal::MYSTATUS_COMPLETED ||
            anime_item->GetMyScore() == 0)
          anime_ids.push_back(it->anime_id);
      }
    }
    foreach_cr_(it, History.items) {
      if (it->episode &&
          std::find(anime_ids.begin(), anime_ids.end(), it->anime_id) == anime_ids.end()) {
        auto anime_item = AnimeDatabase.FindItem(it->anime_id);
        if (anime_item->GetMyStatus() != mal::MYSTATUS_COMPLETED ||
            anime_item->GetMyScore() == 0)
          anime_ids.push_back(it->anime_id);
      }
    }
    foreach_c_(it, anime_ids) {
      auto anime_item = AnimeDatabase.FindItem(*it);
      content += L"  \u2022 " + anime_item->GetTitle();
      if (anime_item->GetMyStatus() == mal::MYSTATUS_COMPLETED) {
        content += L" \u2014 <a href=\"EditAll(" + ToWstr(*it) + L")\">Give a score</a>";
        link_count++;
      } else {
        int last_watched = anime_item->GetMyLastWatchedEpisode();
        if (last_watched > 0)
          content += L" #" + ToWstr(last_watched);
        content += L" \u2014 <a href=\"PlayNext(" + ToWstr(*it) + L")\">Watch next episode</a>";
        link_count++;
      }
      content += L"\n";
    }
    if (content.empty()) {
      content = L"You haven't watched anything recently. "
                L"How about <a href=\"PlayRandomAnime()\">trying a random one</a>?\n\n";
      link_count++;
    } else {
      content = L"Recently watched:\n" + content + L"\n";
      int recently_watched = 0;
      foreach_c_(it, History.queue.items) {
        if (!it->episode) continue;
        date_diff = date_now - (Date)(it->time.substr(0, 10));
        if (date_diff <= day_limit)
          recently_watched++;
      }
      foreach_c_(it, History.items) {
        if (!it->episode) continue;
        date_diff = date_now - (Date)(it->time.substr(0, 10));
        if (date_diff <= day_limit)
          recently_watched++;
      }
      if (recently_watched > 0)
        content += L"You've watched " + ToWstr(recently_watched) + L" episodes in the last week.\n\n";
    }

    // Available episodes
    int available_episodes = 0;
    foreach_c_(it, AnimeDatabase.items) {
      if (it->second.IsInList() && it->second.IsNewEpisodeAvailable())
        available_episodes ++;
    }
    if (available_episodes > 0)
      content += L"There are at least " + ToWstr(available_episodes) + L" new episodes available on your computer.\n\n";

    // Airing times
    vector<int> recently_started, recently_finished, upcoming;
    foreach_c_(it, AnimeDatabase.items) {
      const Date& date_start = it->second.GetDate(anime::DATE_START);
      const Date& date_end = it->second.GetDate(anime::DATE_END);
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
    if (!recently_started.empty()) {
      content += L"Recently started airing:\n";
      foreach_c_(it, recently_started)
        content += L"  \u2022 " + AnimeDatabase.FindItem(*it)->GetTitle() + L"\n";
      content += L"\n";
    }
    if (!recently_finished.empty()) {
      content += L"Recently finished airing:\n";
      foreach_c_(it, recently_finished)
        content += L"  \u2022 " + AnimeDatabase.FindItem(*it)->GetTitle() + L"\n";
      content += L"\n";
    }
    if (!upcoming.empty()) {
      content += L"Upcoming:\n";
      foreach_c_(it, upcoming)
        content += L"  \u2022 " + AnimeDatabase.FindItem(*it)->GetTitle() + L"\n";
      content += L"\n";
    } else {
      content += L"<a href=\"ViewUpcomingAnime()\">View upcoming anime</a>";
      link_count++;
    }

    sys_link_.SetText(content);
    
    /*for (int i = 0; i < link_count; i++)
      sys_link_.SetItemState(i, LIS_ENABLED | LIS_HOTTRACK | LIS_DEFAULTCOLORS);*/
  
  } else {
    int episode_number = GetEpisodeLow(CurrentEpisode.number);
    if (episode_number == 0) episode_number = 1;
    wstring content = L"Now playing: Episode " + ToWstr(episode_number);
    if (!CurrentEpisode.group.empty())
      content += L" by " + CurrentEpisode.group;
    content += L"\n";
    content += L"<a href=\"EditAll(" + ToWstr(anime_id_) + L")\">Edit</a>";
    content += L" \u2022 <a id=\"menu\" href=\"Announce\">Share</a>";
    if (anime_item->GetEpisodeCount() == 0 || 
        anime_item->GetEpisodeCount() > episode_number) {
      content += L" \u2022 <a href=\"PlayEpisode(" + ToWstr(episode_number + 1) + L"\">Watch next episode</a>";
    }
    sys_link_.SetText(content);
  }

  // Toggle tabs
  if (anime_item && anime_item->IsInList() && 
      mode_ == DIALOG_MODE_ANIME_INFORMATION) {
    tab_.Show();
  } else {
    tab_.Hide();
  }

  // Refresh pages
  if (series_info) page_series_info.Refresh(anime_id_, connect);
  if (my_info) page_my_info.Refresh(anime_id_);

  // Update controls
  UpdateControlPositions();
}

void AnimeDialog::UpdateControlPositions(const SIZE* size) {
  win32::Rect rect;
  if (size == nullptr) {
    GetClientRect(&rect);
  } else {
    rect.Set(0, 0, size->cx, size->cy);
  }

  rect.Inflate(-ScaleX(WIN_CONTROL_MARGIN) * 2, 
               -ScaleY(WIN_CONTROL_MARGIN) * 2);
  
  // Image
  if (current_page_ != INFOPAGE_NONE) {
    win32::Rect rect_image = rect;
    rect_image.right = rect_image.left + ScaleX(150);
    auto image = ImageDatabase.GetImage(anime_id_);
    if (image)
      rect_image = ResizeRect(rect_image, 
                              image->rect.Width(), image->rect.Height(), 
                              true, true, false);
    image_label_.SetPosition(nullptr, rect_image);
    rect.left = rect_image.right + ScaleX(WIN_CONTROL_MARGIN) * 2;
  }

  // Title
  win32::Rect rect_title;
  edit_title_.GetWindowRect(&rect_title);
  rect_title.Set(rect.left, rect.top,
                  rect.right, rect.top + rect_title.Height());
  edit_title_.SetPosition(nullptr, rect_title);
  rect.top = rect_title.bottom + ScaleY(WIN_CONTROL_MARGIN);

  // Buttons
  if (mode_ == DIALOG_MODE_ANIME_INFORMATION) {
    win32::Rect rect_button;
    ::GetWindowRect(GetDlgItem(IDOK), &rect_button);
    rect.bottom -= rect_button.Height() + ScaleY(WIN_CONTROL_MARGIN) * 2;
  }

  // Content
  if (mode_ == DIALOG_MODE_NOW_PLAYING) {
    if (anime_id_ == anime::ID_UNKNOWN) {
      rect.left += ScaleX(WIN_CONTROL_MARGIN);
      sys_link_.SetPosition(nullptr, rect);
    } else {
      win32::Dc dc = sys_link_.GetDC();
      int text_height = GetTextHeight(dc.Get());
      win32::Rect rect_content = rect;
      rect_content.Inflate(-ScaleX(WIN_CONTROL_MARGIN * 2), 0);
      rect_content.bottom = rect_content.top + text_height * 2;
      sys_link_.SetPosition(nullptr, rect_content);
      rect.top = rect_content.bottom + ScaleY(WIN_CONTROL_MARGIN) * 3;
    }
  }

  // Pages
  win32::Rect rect_page = rect;
  if (tab_.IsVisible()) {
    tab_.SetPosition(nullptr, rect_page);
    tab_.AdjustRect(m_hWindow, FALSE, &rect_page);
    rect_page.Inflate(-ScaleX(WIN_CONTROL_MARGIN), -ScaleY(WIN_CONTROL_MARGIN));
  }
  page_series_info.SetPosition(nullptr, rect_page);
  page_my_info.SetPosition(nullptr, rect_page);
}