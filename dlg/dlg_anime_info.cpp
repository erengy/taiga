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
#include <algorithm>

#include "dlg_anime_info.h"
#include "dlg_anime_info_page.h"

#include "../anime_db.h"
#include "../common.h"
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
      tab_.InsertItem(0, L"Series information", 0);
      if (AnimeDatabase.FindItem(anime_id_)->IsInList())
        tab_.InsertItem(1, L"My information", 0);
      break;
    case DIALOG_MODE_NOW_PLAYING:
      tab_.Hide();
      break;
  }

  // Initialize pages
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
  if (mode_ == DIALOG_MODE_NOW_PLAYING) {
    HideDlgItem(IDOK);
    HideDlgItem(IDCANCEL);
  }

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
        SetTextColor(hdc, RGB(0x00, 0x33, 0x99));
      }
      return reinterpret_cast<INT_PTR>(::GetStockObject(WHITE_BRUSH));
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
          ExecuteAction(pNMLink->item.szUrl);
          return TRUE;
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

// =============================================================================

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
        sys_link_.Hide();
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
      SetDlgItemText(IDC_EDIT_ANIME_TITLE, anime_item->GetEnglishTitle(false, true).c_str());
    } else {
      SetDlgItemText(IDC_EDIT_ANIME_TITLE, anime_item->GetTitle().c_str());
    }
  } else if (anime_id_ == anime::ID_NOTINLIST) {
    SetDlgItemText(IDC_EDIT_ANIME_TITLE, CurrentEpisode.title.c_str());
  } else {
    SetDlgItemText(IDC_EDIT_ANIME_TITLE, L"Recently watched");
  }

  // Set content
  if (anime_id_ == anime::ID_UNKNOWN) {
    wstring text;
    vector<int> anime_ids;
    for (auto it = History.queue.items.rbegin(); it != History.queue.items.rend(); ++it) {
      if (it->episode && 
          std::find(anime_ids.begin(), anime_ids.end(), it->anime_id) == anime_ids.end())
        anime_ids.push_back(it->anime_id);
    }
    for (auto it = History.items.rbegin(); it != History.items.rend(); ++it) {
      if (it->episode && 
          std::find(anime_ids.begin(), anime_ids.end(), it->anime_id) == anime_ids.end())
        anime_ids.push_back(it->anime_id);
    }
    for (auto it = anime_ids.begin(); it != anime_ids.end(); ++it) {
      auto anime_item = AnimeDatabase.FindItem(*it);
      text += L"\u2022 " + anime_item->GetTitle();
      if (anime_item->GetEpisodeCount() > 0 && 
          anime_item->GetEpisodeCount() == anime_item->GetMyLastWatchedEpisode()) {
        if (anime_item->GetMyScore() == 0)
          text += L" - <a href=\"EditAll(" + ToWstr(*it) + L")\">Give a score</a>";
      } else {
        text += L" - <a href=\"PlayNext(" + ToWstr(*it) + L")\">Watch next episode</a>";
      }
      text += L"\n";
    }
    if (text.empty()) {
      text = L"You haven't watched anything recently. "
             L"How about <a href=\"PlayRandomAnime()\">trying a random one</a>?";
    }
    sys_link_.SetText(text);
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
}

// =============================================================================

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
      win32::Rect rect;
      rect.Set(0, 0, size.cx, size.cy);
      rect.Inflate(-ScaleX(WIN_CONTROL_MARGIN) * 2, -ScaleY(WIN_CONTROL_MARGIN) * 2);
      // Image
      if (current_page_ != INFOPAGE_NONE) {
        win32::Rect rect_image = rect;
        rect_image.right = rect_image.left + ScaleX(150);
        rect_image.bottom = rect_image.top + ScaleY(200);
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
        rect.bottom -= rect_button.Height() + ScaleY(WIN_CONTROL_MARGIN);
      }
      // Content
      if (sys_link_.IsVisible()) {
        sys_link_.SetPosition(nullptr, rect);
      }
      // Pages
      win32::Rect rect_page = rect;
      if (tab_.IsVisible()) {
        tab_.SetPosition(nullptr, rect_page);
        tab_.AdjustRect(m_hWindow, FALSE, &rect_page);
      }
      page_series_info.SetPosition(nullptr, rect_page);
      page_my_info.SetPosition(nullptr, rect_page);
    }
  }
}