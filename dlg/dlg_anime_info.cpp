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
#include "dlg_anime_info.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"

CAnimeWindow AnimeWindow;

// =============================================================================

CAnimeWindow::CAnimeWindow() :
  m_hbrDarkBlue(NULL), m_hbrLightBlue(NULL), 
  m_hfTitle(NULL), m_pAnimeItem(NULL)
{
  RegisterDlgClass(L"TaigaAnimeInfoW");
}

CAnimeWindow::~CAnimeWindow() {
  if (m_hbrDarkBlue)  ::DeleteObject(m_hbrDarkBlue);
  if (m_hbrLightBlue) ::DeleteObject(m_hbrLightBlue);
  if (m_hfTitle)      ::DeleteObject(m_hfTitle);
}

BOOL CAnimeWindow::OnInitDialog() {
  // Set anime index
  if (!m_pAnimeItem) m_pAnimeItem = &AnimeList.Item[AnimeList.Index];

  // Create GDI objects
  if (!m_hbrDarkBlue) {
    m_hbrDarkBlue = ::CreateSolidBrush(RGB(46, 81, 162));
  }
  if (!m_hbrLightBlue) {
    m_hbrLightBlue = ::CreateSolidBrush(RGB(225, 231, 245));
  }
  if (!m_hfTitle) {
    LOGFONT lFont;
    ::GetObject(GetFont(), sizeof(LOGFONT), &lFont);
    CDC dc = GetDC();
    lstrcpy(lFont.lfFaceName, L"Verdana");
    lFont.lfCharSet = DEFAULT_CHARSET;
    lFont.lfHeight = -MulDiv(12, GetDeviceCaps(dc.Get(), LOGPIXELSY), 72);
    lFont.lfWeight = FW_BOLD;
    m_hfTitle = ::CreateFontIndirect(&lFont);
  }
  SendDlgItemMessage(IDC_STATIC_ANIME_TITLE, WM_SETFONT, reinterpret_cast<WPARAM>(m_hfTitle), FALSE);

  // Create edit
  m_Edit.Attach(GetDlgItem(IDC_EDIT_ANIME_INFO));
  
  // Create toolbar
  /*m_Toolbar.Create(NULL, TOOLBARCLASSNAME, NULL, 
    WS_CHILD | WS_VISIBLE | WS_TABSTOP | 
    CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | 
    TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | TBSTYLE_WRAPABLE, 
    16, 260, 120, 100, 
    GetWindowHandle(), NULL, NULL);
  m_Toolbar.SetImageList(UI.ImgList16.GetHandle(), 16, 16);
  m_Toolbar.InsertButton(0, 0, 100, 1, 0, NULL, NULL, L"Anime page");
  m_Toolbar.InsertButton(1, 1, 101, 1, 0, NULL, NULL, L"Anime forums");
  m_Toolbar.InsertButton(2, 2, 102, 1, 0, NULL, NULL, L"Anime folder");
  m_Toolbar.InsertButton(3, 3, 103, 1, 0, NULL, NULL, L"External links");*/

  // Set image position
  if (AnimeImage.Rect.IsEmpty()) {
    CRect rcTemp;
    ::GetWindowRect(GetDlgItem(IDC_STATIC_ANIME_IMG), &rcTemp);
    ::GetWindowRect(GetDlgItem(IDC_STATIC_ANIME_IMG), &AnimeImage.Rect);
    ::ScreenToClient(m_hWindow, reinterpret_cast<LPPOINT>(&rcTemp));
    AnimeImage.Rect.Offset(rcTemp.left - AnimeImage.Rect.left, rcTemp.top - AnimeImage.Rect.top);
  }

  // Refresh
  Refresh(m_pAnimeItem);
  return TRUE;
}

void CAnimeWindow::Refresh(CAnime* pAnimeItem) {
  // Set anime index
  if (pAnimeItem) m_pAnimeItem = pAnimeItem;
  if (!m_pAnimeItem || !IsWindow()) return;

  // Set title
  wstring text = L"  " + m_pAnimeItem->Series_Title;
  SetDlgItemText(IDC_STATIC_ANIME_TITLE, text.c_str());
  // Set synonyms
  text.clear();
  if (!m_pAnimeItem->Series_Synonyms.empty()) {
    text = L"    " + m_pAnimeItem->Series_Synonyms;
  }
  if (!m_pAnimeItem->Synonyms.empty()) {
    text += text.empty() ? L"    " : L"; ";
    text += m_pAnimeItem->Synonyms;
  }
  SetDlgItemText(IDC_STATIC_ANIME_ALT, text.c_str());
  
  // Set synopsis
  if (m_pAnimeItem->Synopsis.empty()) {
    text = L"Loading...";
    MAL.SearchAnime(m_pAnimeItem->Series_Title, m_pAnimeItem);
  } else {
    text = m_pAnimeItem->Synopsis;
  }
  SetDlgItemText(IDC_EDIT_ANIME_INFO, text.c_str());
  
  // Set series information
  text = MAL.TranslateType(m_pAnimeItem->Series_Type) + L"\n" +
    MAL.TranslateStatus(m_pAnimeItem->Series_Status) + L"\n" + 
    MAL.TranslateDate(m_pAnimeItem->Series_Start) + L"\n" + 
    m_pAnimeItem->Score;
  SetDlgItemText(IDC_STATIC_ANIME_INFO1, text.c_str());
  // Set user information
  text = MAL.TranslateMyStatus(m_pAnimeItem->My_Status, false) + L"\n" + 
    MAL.TranslateNumber(m_pAnimeItem->My_WatchedEpisodes) + L"/" + 
    MAL.TranslateNumber(m_pAnimeItem->Series_Episodes) + L"\n" + 
    MAL.TranslateNumber(m_pAnimeItem->My_Score) + L"\n" + 
    m_pAnimeItem->My_Tags;
  SetDlgItemText(IDC_STATIC_ANIME_INFO2, text.c_str());

  // Load image
  if (AnimeImage.Load(Taiga.GetDataPath() + L"Image\\" + ToWSTR(m_pAnimeItem->Series_ID) + L".jpg")) {
    CWindow img = GetDlgItem(IDC_STATIC_ANIME_IMG);
    img.SetPosition(NULL, AnimeImage.Rect);
    img.SetWindowHandle(NULL);
  } else {
    MAL.DownloadImage(m_pAnimeItem);
  }
  InvalidateRect(&AnimeImage.Rect);
}

// =============================================================================

BOOL CAnimeWindow::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Set background and text colors
    case WM_CTLCOLORDLG: {
      return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
    }
    case WM_CTLCOLORSTATIC: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      HWND hwnd_static = reinterpret_cast<HWND>(lParam);
      if (hwnd_static == GetDlgItem(IDC_STATIC_ANIME_TITLE)) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        return reinterpret_cast<INT_PTR>(m_hbrDarkBlue);
      } else if (hwnd_static == GetDlgItem(IDC_STATIC_ANIME_ALT)) {
        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(m_hbrLightBlue);
      } else if (hwnd_static == GetDlgItem(IDC_STATIC_ANIME_DARKBLUE)) {
        return reinterpret_cast<INT_PTR>(m_hbrDarkBlue);
      } else {
        return reinterpret_cast<INT_PTR>(GetStockObject(WHITE_BRUSH));
      }
      break;
    }
    
    // Drag window
    case WM_ENTERSIZEMOVE: {
      if (::IsAppThemed() && GetWinVersion() >= WINVERSION_VISTA) {
        SetTransparency(200);
      }
      break;
    }
    case WM_EXITSIZEMOVE: {
      if (::IsAppThemed() && GetWinVersion() >= WINVERSION_VISTA) {
        SetTransparency(255);
      }
      break;
    }    
    case WM_LBUTTONDOWN: {
      SetCursor(reinterpret_cast<HCURSOR>(LoadImage(NULL, IDC_SIZEALL, IMAGE_CURSOR, 0, 0, LR_SHARED)));
      ReleaseCapture();
      SendMessage(WM_NCLBUTTONDOWN, HTCAPTION, NULL);
      break;
    }

    // Close window
    case WM_RBUTTONUP: {
      Destroy();
      return TRUE;
    }
    
    // Paint
    case WM_PAINT: {
      HDC hdc;
      PAINTSTRUCT ps;
      hdc = ::BeginPaint(hwnd, &ps);
      OnPaint(hdc, &ps);
      ::EndPaint(hwnd, &ps);
      break;
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

BOOL CAnimeWindow::PreTranslateMessage(MSG* pMsg) {
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

bool CAnimeWindow::CAnimeImage::Load(wstring file) {
  ::DeleteObject(DC.DetachBitmap());
  
  HBITMAP hbmp = NULL;
  HDC hScreen = ::GetDC(NULL);
  DC = ::CreateCompatibleDC(hScreen);
  ::ReleaseDC(NULL, hScreen);

  Gdiplus::Bitmap bmp(file.c_str());
  Width = bmp.GetWidth();
  Height = bmp.GetHeight();
  bmp.GetHBITMAP(NULL, &hbmp);
  
  if (!hbmp || !Width || !Height) {
    ::DeleteObject(hbmp);
    return false;
  } else {
    Rect.bottom = Rect.top + 
      static_cast<int>(Rect.Width() * 
      (static_cast<float>(Height) / static_cast<float>(Width)));
    DC.AttachBitmap(hbmp);
    return true;
  }
}

void CAnimeWindow::OnPaint(HDC hdc, LPPAINTSTRUCT lpps) {
  // Draw image
  CDC dc = hdc;
  dc.SetStretchBltMode(HALFTONE);
  dc.StretchBlt(AnimeImage.Rect.left, AnimeImage.Rect.top, 
    AnimeImage.Rect.Width() - 3, AnimeImage.Rect.Height() - 3, 
    AnimeImage.DC.Get(), 0, 0, AnimeImage.Width, AnimeImage.Height, 
    SRCCOPY);
}