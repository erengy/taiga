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
#include "../event.h"
#include "../gfx.h"
#include "../http.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"

class AnimeDialog AnimeDialog;

// =============================================================================

AnimeDialog::AnimeDialog()
    : anime_id_(anime::ID_UNKNOWN), 
      brush_darkblue_(nullptr), 
      brush_lightblue_(nullptr), 
      current_page_(0), 
      title_font_(nullptr) {
  RegisterDlgClass(L"TaigaAnimeInfoW");
 
  pages.resize(INFOPAGE_COUNT);
  for (size_t i = 0; i < INFOPAGE_COUNT; i++) {
    pages[i].index = i;
  }
}

AnimeDialog::~AnimeDialog() {
  if (brush_darkblue_)  ::DeleteObject(brush_darkblue_);
  if (brush_lightblue_) ::DeleteObject(brush_lightblue_);
  if (title_font_)      ::DeleteObject(title_font_);
}

// =============================================================================

BOOL AnimeDialog::OnInitDialog() {
  // Set anime ID
  if (anime_id_ <= anime::ID_UNKNOWN)
    anime_id_ = AnimeDatabase.GetCurrentId();

  // Create GDI objects
  if (!brush_darkblue_) brush_darkblue_ = ::CreateSolidBrush(mal::COLOR_DARKBLUE);
  if (!brush_lightblue_) brush_lightblue_ = ::CreateSolidBrush(mal::COLOR_LIGHTBLUE);
  if (!title_font_) {
    LOGFONT lFont;
    ::GetObject(GetFont(), sizeof(LOGFONT), &lFont);
    win32::Dc dc = GetDC();
    lFont.lfCharSet = DEFAULT_CHARSET;
    lFont.lfHeight = -MulDiv(12, GetDeviceCaps(dc.Get(), LOGPIXELSY), 72);
    lFont.lfWeight = FW_BOLD;
    title_font_ = ::CreateFontIndirect(&lFont);
  }
  SendDlgItemMessage(IDC_EDIT_ANIME_TITLE, WM_SETFONT, reinterpret_cast<WPARAM>(title_font_), FALSE);

  // Create tabs
  tab_.Attach(GetDlgItem(IDC_TAB_ANIME));
  tab_.InsertItem(0, L"Series information", 0);
  if (AnimeDatabase.FindItem(anime_id_)->IsInList()) {
    tab_.InsertItem(1, L"My information", 0);
  }

  // Set image position
  if (image_.rect.IsEmpty()) {
    win32::Rect rcTemp;
    ::GetWindowRect(GetDlgItem(IDC_STATIC_ANIME_IMG), &rcTemp);
    ::GetWindowRect(GetDlgItem(IDC_STATIC_ANIME_IMG), &image_.rect);
    ::ScreenToClient(m_hWindow, reinterpret_cast<LPPOINT>(&rcTemp));
    image_.rect.Offset(rcTemp.left - image_.rect.left, rcTemp.top - image_.rect.top);
  }
  // Change image mouse pointer
  ::SetClassLong(GetDlgItem(IDC_STATIC_ANIME_IMG), GCL_HCURSOR, 
    reinterpret_cast<LONG>(::LoadImage(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_SHARED)));

  // Create pages
  win32::Rect rcPage; tab_.AdjustRect(m_hWindow, FALSE, &rcPage);
  for (size_t i = 0; i < INFOPAGE_COUNT; i++) {
    pages[i].Create(IDD_ANIME_INFO_PAGE01 + i, m_hWindow, false);
    pages[i].SetPosition(nullptr, rcPage, SWP_HIDEWINDOW);
    EnableThemeDialogTexture(pages[i].GetWindowHandle(), ETDT_ENABLETAB);
  }

  // Refresh
  SetCurrentPage(current_page_);
  Refresh(anime_id_);
  return TRUE;
}

void AnimeDialog::OnOK() {
  // No need to save anything if anime is not in list
  auto anime_item = AnimeDatabase.FindItem(anime_id_);
  if (!anime_item->IsInList()) {
    EndDialog(IDOK);
    return;
  }

  // Create item
  EventItem item;
  item.anime_id = anime_id_;
  item.mode = HTTP_MAL_AnimeEdit;

  // Episodes watched
  item.episode = pages[INFOPAGE_MYINFO].GetDlgItemInt(IDC_EDIT_ANIME_PROGRESS);
  if (!mal::IsValidEpisode(item.episode, -1, anime_item->GetEpisodeCount())) {
    wstring msg = L"Please enter a valid episode number between 0-" + 
      ToWstr(anime_item->GetEpisodeCount()) + L".";
    MessageBox(msg.c_str(), L"Episodes watched", MB_OK | MB_ICONERROR);
    return;
  }

  // Re-watching
  item.enable_rewatching = pages[INFOPAGE_MYINFO].IsDlgButtonChecked(IDC_CHECK_ANIME_REWATCH);
  
  // Score
  item.score = 10 - pages[INFOPAGE_MYINFO].GetComboSelection(IDC_COMBO_ANIME_SCORE);
  
  // Status
  item.status = pages[INFOPAGE_MYINFO].GetComboSelection(IDC_COMBO_ANIME_STATUS) + 1;
  if (item.status == mal::MYSTATUS_UNKNOWN) item.status++;
  
  // Tags
  pages[INFOPAGE_MYINFO].GetDlgItemText(IDC_EDIT_ANIME_TAGS, item.tags);

  // Start date
  SYSTEMTIME stMyStart;
  if (pages[INFOPAGE_MYINFO].SendDlgItemMessage(IDC_DATETIME_START, DTM_GETSYSTEMTIME, 0, 
    reinterpret_cast<LPARAM>(&stMyStart)) == GDT_NONE) {
      anime_item->SetMyDate(anime::DATE_START, Date(), true, true);
  } else {
    anime_item->SetMyDate(anime::DATE_START, 
      Date(stMyStart.wYear, stMyStart.wMonth, stMyStart.wDay), true, true);
  }
  // Finish date
  SYSTEMTIME stMyFinish;
  if (pages[INFOPAGE_MYINFO].SendDlgItemMessage(IDC_DATETIME_FINISH, DTM_GETSYSTEMTIME, 0, 
    reinterpret_cast<LPARAM>(&stMyFinish)) == GDT_NONE) {
      anime_item->SetMyDate(anime::DATE_END, Date(), true, true);
  } else {
    anime_item->SetMyDate(anime::DATE_END, 
      Date(stMyFinish.wYear, stMyFinish.wMonth, stMyFinish.wDay), true, true);
  }

  // Alternative titles
  wstring titles;
  pages[INFOPAGE_MYINFO].GetDlgItemText(IDC_EDIT_ANIME_ALT, titles);
  anime_item->SetUserSynonyms(titles, true);

  // Add item to event queue
  EventQueue.Add(item);

  // Exit
  EndDialog(IDOK);
}

// =============================================================================

BOOL AnimeDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_CTLCOLORSTATIC: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      HWND hwnd_control = reinterpret_cast<HWND>(lParam);
      if (hwnd_control == GetDlgItem(IDC_STATIC_ANIME_TITLE) || 
          hwnd_control == GetDlgItem(IDC_EDIT_ANIME_TITLE)) {
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, mal::COLOR_DARKBLUE);
            return reinterpret_cast<INT_PTR>(brush_lightblue_);
      }
      break;
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
        dc.FillRect(rect, RGB(255, 255, 255));
        dc.SetStretchBltMode(HALFTONE);
        dc.StretchBlt(rect.left, rect.top, rect.Width(), rect.Height(), 
          image_.dc.Get(), 0, 0, image_.width, image_.height, SRCCOPY);
        dc.DetachDC();
        return TRUE;
      }
      break;
    }

    // Drag window
    case WM_ENTERSIZEMOVE: {
      if (::IsAppThemed() && win32::GetWinVersion() >= win32::VERSION_VISTA) {
        SetTransparency(200);
      }
      break;
    }
    case WM_EXITSIZEMOVE: {
      if (::IsAppThemed() && win32::GetWinVersion() >= win32::VERSION_VISTA) {
        SetTransparency(255);
      }
      break;
    }    
    case WM_LBUTTONDOWN: {
      SetCursor(reinterpret_cast<HCURSOR>(LoadImage(nullptr, IDC_SIZEALL, IMAGE_CURSOR, 0, 0, LR_SHARED)));
      ReleaseCapture();
      SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, 0);
      break;
    }

    // Close window
    case WM_RBUTTONUP: {
      Destroy();
      return TRUE;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL AnimeDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  if (LOWORD(wParam) == IDC_STATIC_ANIME_IMG) {
    if (HIWORD(wParam) == STN_CLICKED) {
      mal::ViewAnimePage(anime_id_);
      return TRUE;
    }
  }
  
  return FALSE;
}

LRESULT AnimeDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  if (idCtrl == IDC_TAB_ANIME) {
    switch (pnmh->code) {
      // Tab select
      case TCN_SELCHANGE: {
        SetCurrentPage(tab_.GetCurrentlySelected());
        break;
      }
    }
  }
  
  return 0;
}

BOOL AnimeDialog::PreTranslateMessage(MSG* pMsg) {
  if (pMsg->message == WM_KEYDOWN) {
    // Close window
    if (pMsg->wParam == VK_ESCAPE) {
      Destroy();
      return TRUE;
    }
  }
  return FALSE;
}

// =============================================================================

void AnimeDialog::SetCurrentPage(int index) {
  current_page_ = index;
  if (IsWindow()) {
    for (int i = 0; i < INFOPAGE_COUNT; i++) {
      if (i != index) pages[i].Hide();
    }
    pages[index].Show();
    tab_.SetCurrentlySelected(index);
  }
}

int AnimeDialog::GetAnimeID() const {
  return anime_id_;
}

void AnimeDialog::Refresh(int anime_id, bool series_info, bool my_info) {
  // Set anime index
  if (anime_id > anime::ID_UNKNOWN) anime_id_ = anime_id;
  if (anime_id_ <= anime::ID_UNKNOWN || !IsWindow()) return;

  // Set title
  auto anime_item = AnimeDatabase.FindItem(anime_id_);
  SetDlgItemText(IDC_EDIT_ANIME_TITLE, anime_item->GetTitle().c_str());

  // Load image
  if (image_.Load(anime::GetImagePath(anime_id_))) {
    win32::Window img = GetDlgItem(IDC_STATIC_ANIME_IMG);
    img.SetPosition(nullptr, image_.rect);
    img.SetWindowHandle(nullptr);
    // Refresh if current file is too old
    if (anime_item->GetAiringStatus() != mal::STATUS_FINISHED) {
      // Check last modified date (>= 7 days)
      if (GetFileAge(anime::GetImagePath(anime_id_)) / (60 * 60 * 24) >= 7) {
        mal::DownloadImage(anime_id_, anime_item->GetImageUrl());
      }
    }
  } else {
    mal::DownloadImage(anime_id_, anime_item->GetImageUrl());
  }
  InvalidateRect(&image_.rect);

  // Refresh pages
  if (series_info) {
    pages[INFOPAGE_SERIESINFO].Refresh(anime_id_);
  }
  if (my_info) {
    pages[INFOPAGE_MYINFO].Refresh(anime_id_);
  }
}

// =============================================================================

void AnimeDialog::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  win32::Dc dc = hdc;
  win32::Rect rect;

  // Paint background
  GetClientRect(&rect);
  rect.top = image_.rect.top + 20;
  dc.FillRect(rect, RGB(255, 255, 255));
  rect.bottom = rect.top + 200;
  GradientRect(dc.Get(), &rect, ::GetSysColor(COLOR_BTNFACE), RGB(255, 255, 255), true);
}