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

#include "dlg_anime_list.h"

#include "dlg_main.h"
#include "dlg_torrent.h"

#include "../anime_db.h"
#include "../anime_filter.h"
#include "../common.h"
#include "../gfx.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"

#include "../win32/win_gdi.h"

class AnimeListDialog AnimeListDialog;

// =============================================================================

BOOL AnimeListDialog::OnInitDialog() {
  // Create tab control
  tab.Attach(GetDlgItem(IDC_TAB_MAIN));

  // Create main list
  listview.parent = this;
  listview.Attach(GetDlgItem(IDC_LIST_MAIN));
  listview.SetExtendedStyle(LVS_EX_AUTOSIZECOLUMNS | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  listview.SetImageList(UI.ImgList16.GetHandle());
  listview.Sort(0, 1, 0, ListViewCompareProc);
  listview.SetTheme();

  // Insert list columns
  listview.InsertColumn(0, GetSystemMetrics(SM_CXSCREEN), 340, LVCFMT_LEFT, L"Anime title");
  listview.InsertColumn(1, 200, 200, LVCFMT_CENTER, L"Progress");
  listview.InsertColumn(2,  62,  62, LVCFMT_CENTER, L"Score");
  listview.InsertColumn(3,  62,  62, LVCFMT_CENTER, L"Type");
  listview.InsertColumn(4, 105, 105, LVCFMT_RIGHT,  L"Season");

  // Insert tabs and list groups
  listview.InsertGroup(mal::MYSTATUS_NOTINLIST, mal::TranslateMyStatus(mal::MYSTATUS_NOTINLIST, false).c_str());
  for (int i = mal::MYSTATUS_WATCHING; i <= mal::MYSTATUS_PLANTOWATCH; i++) {
    if (i != mal::MYSTATUS_UNKNOWN) {
      tab.InsertItem(i - 1, mal::TranslateMyStatus(i, true).c_str(), (LPARAM)i);
      listview.InsertGroup(i, mal::TranslateMyStatus(i, false).c_str());
    }
  }

  // Refresh
  RefreshList(mal::MYSTATUS_WATCHING);
  RefreshTabs(mal::MYSTATUS_WATCHING);

  // Success
  return TRUE;
}

// =============================================================================

INT_PTR AnimeListDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_MOUSEMOVE: {
      // Drag list item
      if (listview.dragging) {
        bool allow_drop = false;
        
        if (tab.HitTest() > -1)
          allow_drop = true;

        if (!allow_drop) {
          POINT pt;
          GetCursorPos(&pt);
          win32::Rect rect_edit;
          MainDialog.edit.GetWindowRect(&rect_edit);
          if (rect_edit.PtIn(pt))
            allow_drop = true;
        }

        if (!allow_drop) {
          TVHITTESTINFO ht = {0};
          MainDialog.treeview.HitTest(&ht, true);
          if (ht.flags & TVHT_ONITEM) {
            int index = MainDialog.treeview.GetItemData(ht.hItem);
            switch (index) {
              case SIDEBAR_ITEM_SEARCH:
              case SIDEBAR_ITEM_FEEDS:
                allow_drop = true;
                break;
            }
          }
        }
        
        POINT pt;
        GetCursorPos(&pt);
        ::ScreenToClient(MainDialog.GetWindowHandle(), &pt);
        listview.drag_image.DragMove(pt.x + 16, pt.y + 32);
        SetCursor(LoadCursor(nullptr, allow_drop ? IDC_ARROW : IDC_NO));
      }
      break;
    }

    case WM_LBUTTONUP: {
      // Drop list item
      if (listview.dragging) {
        listview.drag_image.DragLeave(MainDialog.GetWindowHandle());
        listview.drag_image.EndDrag();
        listview.drag_image.Destroy();
        listview.dragging = false;
        ReleaseCapture();
        
        int tab_index = tab.HitTest();
        if (tab_index > -1) {
          int status = tab.GetItemParam(tab_index);
          ExecuteAction(L"EditStatus(" + ToWstr(status) + L")");
          break;
        }
        
        auto anime_item = AnimeDatabase.GetCurrentItem();
        if (!anime_item)
          break;
        wstring text = Settings.Program.List.english_titles ? 
          anime_item->GetEnglishTitle(true) : anime_item->GetTitle();

        POINT pt;
        GetCursorPos(&pt);
        win32::Rect rect_edit;
        MainDialog.edit.GetWindowRect(&rect_edit);
        if (rect_edit.PtIn(pt)) {
          MainDialog.edit.SetText(text);
          break;
        }

        TVHITTESTINFO ht = {0};
        MainDialog.treeview.HitTest(&ht, true);
        if (ht.flags & TVHT_ONITEM) {
          int index = MainDialog.treeview.GetItemData(ht.hItem);
          switch (index) {
            case SIDEBAR_ITEM_SEARCH:
              ExecuteAction(L"SearchAnime(" + text + L")");
              break;
            case SIDEBAR_ITEM_FEEDS:
              TorrentDialog.Search(Settings.RSS.Torrent.search_url, text);
              break;
          }
        }
      }
      break;
    }

    case WM_MEASUREITEM: {
      if (wParam == IDC_LIST_MAIN) {
        auto mis = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        mis->itemHeight = 48;
        return TRUE;
      }
      break;
    }

    case WM_DRAWITEM: {
      if (wParam == IDC_LIST_MAIN) {
        auto dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        win32::Dc dc = dis->hDC;
        win32::Rect rect = dis->rcItem;

        int anime_id = dis->itemData;
        auto anime_item = AnimeDatabase.FindItem(anime_id);
        if (!anime_item) return TRUE;

        if ((dis->itemState & ODS_SELECTED) == ODS_SELECTED) {
          dc.FillRect(rect, RGB(230, 230, 255));
        }
        rect.Inflate(-2, -2);
        dc.FillRect(rect, RGB(250, 250, 250));

        // Draw image
        win32::Rect rect_image = rect;
        rect_image.right = rect_image.left + static_cast<int>(rect_image.Height() / 1.4);
        dc.FillRect(rect_image, RGB(230, 230, 230));
        if (ImageDatabase.Load(anime_id, false, false)) {
          auto image = ImageDatabase.GetImage(anime_id);
          int sbm = dc.SetStretchBltMode(HALFTONE);
          dc.StretchBlt(rect_image.left, rect_image.top, 
            rect_image.Width(), rect_image.Height(), 
            image->dc.Get(), 0, 0, 
            image->rect.Width(), 
            image->rect.Height(), 
            SRCCOPY);
          dc.SetStretchBltMode(sbm);
        }

        // Draw title
        rect.left += rect_image.Width() + 8;
        int bk_mode = dc.SetBkMode(TRANSPARENT);
        dc.AttachFont(UI.font_header);
        dc.DrawText(anime_item->GetTitle().c_str(), anime_item->GetTitle().length(), rect, 
                    DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);
        dc.DetachFont();

        // Draw second line of information
        rect.top += 20;
        COLORREF text_color = dc.SetTextColor(RGB(128, 128, 128));
        wstring text = /*L"Watched " +*/ ToWstr(anime_item->GetMyLastWatchedEpisode()) + 
                       L"/" + ToWstr(anime_item->GetEpisodeCount());
        dc.DrawText(text.c_str(), -1, rect, 
                    DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);
        dc.SetTextColor(text_color);
        dc.SetBkMode(bk_mode);

        // Draw progress bar
        rect.left -= 2;
        rect.top += 12;
        rect.bottom = rect.top + 12;
        rect.right -= 8;
        listview.DrawProgressBar(dc.Get(), &rect, 0, anime_item);

        dc.DetachDC();
        return TRUE;
      }
      break;
    }

    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL: {
      return listview.SendMessage(uMsg, wParam, lParam);
    }
  }
  
  return DialogProcDefault(hwnd, uMsg, wParam, lParam);
}

// =============================================================================

LRESULT AnimeListDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // ListView control
  if (idCtrl == IDC_LIST_MAIN || pnmh->hwndFrom == listview.GetHeader()) {
    return OnListNotify(reinterpret_cast<LPARAM>(pnmh));
  
  // Tab control
  } else if (idCtrl == IDC_TAB_MAIN) {
    return OnTabNotify(reinterpret_cast<LPARAM>(pnmh));
  }
  
  return 0;
}

void AnimeListDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      // Set client area
      win32::Rect rcWindow(0, 0, size.cx, size.cy);
      // Resize tab
      rcWindow.left -= 1;
      rcWindow.top -= 1;
      rcWindow.right += 3;
      rcWindow.bottom += 2;
      tab.SetPosition(nullptr, rcWindow);
      // Resize list
      tab.AdjustRect(nullptr, FALSE, &rcWindow);
      rcWindow.left -= 3;
      rcWindow.top -= 1;
      rcWindow.bottom += 2;
      listview.SetPosition(nullptr, rcWindow, 0);
    }
  }
}

// =============================================================================

/* ListView control */

int AnimeListDialog::ListView::GetSortType(int column) {
  switch (column) {
    // Progress
    case 1:
      return LIST_SORTTYPE_PROGRESS;
    // Score
    case 2:
      return LIST_SORTTYPE_NUMBER;
    // Season
    case 4:
      return LIST_SORTTYPE_STARTDATE;
    // Other columns
    default:
      return LIST_SORTTYPE_DEFAULT;
  }
}

LRESULT AnimeListDialog::ListView::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Middle mouse button
    case WM_MBUTTONDOWN: {
      int item_index = HitTest();
      if (item_index > -1) {
        SetSelectedItem(item_index);
        switch (Settings.Program.List.middle_click) {
          case 1: ExecuteAction(L"EditAll");    break;
          case 2: ExecuteAction(L"OpenFolder"); break;
          case 3: ExecuteAction(L"PlayNext");   break;
          case 4: ExecuteAction(L"Info");       break;
        }
      }
      break;
    }

    // Set cursor
    case WM_SETCURSOR: {
      POINT pt;
      ::GetCursorPos(&pt);
      ::ScreenToClient(GetWindowHandle(), &pt);

      win32::Rect rect_item;
      int item_index = GetNextItem(-1, LVIS_SELECTED);
      if (item_index < 0) break;

      GetSubItemRect(item_index, 1, &rect_item);
      if (Settings.Program.List.progress_show_eps)
        rect_item.right -= 50;
      rect_item.Inflate(-5, -5);
      win32::Rect rect_button[2];
      rect_button[0].Copy(rect_item);
      rect_button[1].Copy(rect_item);
      rect_button[0].right = rect_button[0].left + 9;
      rect_button[1].left = rect_button[1].right - 9;

      if ((button_visible[0] && rect_button[0].PtIn(pt)) || 
          (button_visible[1] && rect_button[1].PtIn(pt))) {
        ::SetCursor(reinterpret_cast<HCURSOR>(
          ::LoadImage(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_SHARED)));
        return TRUE;
      }
      break;
     }
  }
  
  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT AnimeListDialog::OnListNotify(LPARAM lParam) {
  LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
  switch (pnmh->code) {
    // Item drag
    case LVN_BEGINDRAG: {
      POINT pt = {};
      auto lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      listview.drag_image = listview.CreateDragImage(lplv->iItem, &pt);
      if (listview.drag_image.GetHandle()) {
        pt = lplv->ptAction;
        listview.drag_image.BeginDrag(0, 0, 0);
        listview.drag_image.DragEnter(MainDialog.GetWindowHandle(), pt.x, pt.y);
        listview.dragging = true;
        SetCapture();
      }
      break;
    }
    
    // Column click
    case LVN_COLUMNCLICK: {
      auto lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      int order = 1;
      if (lplv->iSubItem == listview.GetSortColumn()) order = listview.GetSortOrder() * -1;
      listview.Sort(lplv->iSubItem, order, listview.GetSortType(lplv->iSubItem), ListViewCompareProc);
      break;
    }

    // Item select
    case LVN_ITEMCHANGED: {
      auto lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      auto anime_id = static_cast<int>(lplv->lParam);
      AnimeDatabase.SetCurrentId(anime_id);
      listview.button_visible[0] = false;
      listview.button_visible[1] = false;
      if (lplv->uNewState != 0) {
        auto anime_item = AnimeDatabase.FindItem(anime_id);
        if (anime_item && anime_item->IsInList()) {
          if (anime_item->GetMyLastWatchedEpisode() > 0)
            listview.button_visible[0] = true;
          if (anime_item->GetEpisodeCount() > anime_item->GetMyLastWatchedEpisode() ||
              anime_item->GetEpisodeCount() == 0)
            listview.button_visible[1] = true;
        }
      }
      break;
    }

    // Double click
    case NM_DBLCLK: {
      if (listview.GetSelectedCount() > 0) {
        switch (Settings.Program.List.double_click) {
          case 1: ExecuteAction(L"EditAll");    break;
          case 2: ExecuteAction(L"OpenFolder"); break;
          case 3: ExecuteAction(L"PlayNext");   break;
          case 4: ExecuteAction(L"Info");       break;
        }
      }
      break;
    }

    // Left click
    case NM_CLICK: {
      if (pnmh->hwndFrom == listview.GetWindowHandle()) {
        if (listview.GetSelectedCount() > 0) {
          POINT pt;
          ::GetCursorPos(&pt);
          ::ScreenToClient(listview.GetWindowHandle(), &pt);
        
          win32::Rect rect_item;
          listview.GetSubItemRect(listview.GetNextItem(-1, LVIS_SELECTED), 1, &rect_item);
          if (Settings.Program.List.progress_show_eps)
            rect_item.right -= 50;
          rect_item.Inflate(-5, -5);
          win32::Rect rect_button[2];
          rect_button[0].Copy(rect_item);
          rect_button[1].Copy(rect_item);
          rect_button[0].right = rect_button[0].left + 9;
          rect_button[1].left = rect_button[1].right - 9;

          if (listview.button_visible[0] && rect_button[0].PtIn(pt)) {
            ExecuteAction(L"DecrementEpisode");
          } else if (listview.button_visible[1] && rect_button[1].PtIn(pt)) {
            ExecuteAction(L"IncrementEpisode");
          }
        }
      }
      break;
    }

    // Right click
    case NM_RCLICK: {
      if (pnmh->hwndFrom == listview.GetWindowHandle()) {
        if (listview.GetSelectedCount() > 0) {
          auto anime_item = AnimeDatabase.GetCurrentItem();
          UpdateAllMenus(anime_item);
          int index = listview.HitTest(true);
          if (anime_item->IsInList()) {
            switch (index) {
              // Progress
              case 1:
                ExecuteAction(UI.Menus.Show(g_hMain, 0, 0, L"PlayEpisode"));
                break;
              // Score
              case 2:
                ExecuteAction(UI.Menus.Show(g_hMain, 0, 0, L"EditScore"));
                break;
              // Other
              default:
                ExecuteAction(UI.Menus.Show(g_hMain, 0, 0, L"RightClick"));
                break;
            }
            UpdateAllMenus(anime_item);
          } else {
            UpdateSearchListMenu(true);
            ExecuteAction(UI.Menus.Show(g_hMain, 0, 0, L"SearchList"), 
              0, static_cast<LPARAM>(anime_item->GetId()));
          }
        }
      }
      break;
    }

    // Text callback
    case LVN_GETDISPINFO: {
      NMLVDISPINFO* plvdi = reinterpret_cast<NMLVDISPINFO*>(lParam);
      auto anime_item = AnimeDatabase.FindItem(static_cast<int>(plvdi->item.lParam));
      if (!anime_item) break;
      switch (plvdi->item.iSubItem) {
        case 0: // Anime title
          if (Settings.Program.List.english_titles) {
            plvdi->item.pszText = const_cast<LPWSTR>(
              anime_item->GetEnglishTitle(true).data());
          } else {
            plvdi->item.pszText = const_cast<LPWSTR>(
              anime_item->GetTitle().data());
          }
          break;
      }
      break;
    }

    // Key press
    case LVN_KEYDOWN: {
      LPNMLVKEYDOWN pnkd = reinterpret_cast<LPNMLVKEYDOWN>(lParam);
      switch (pnkd->wVKey) {
        // Delete item
        case VK_DELETE: {
          if (listview.GetSelectedCount() > 0)
            ExecuteAction(L"EditDelete()");
          break;
        }
        // Check episodes
        case VK_F5: {
          ExecuteAction(L"CheckEpisodes()");
          break;
        }
        default: {
          if (listview.GetSelectedCount() > 0 &&
              GetKeyState(VK_CONTROL) & 0x8000) {
            // Edit episode
            if (pnkd->wVKey == VK_ADD) {
              auto anime_item = AnimeDatabase.GetCurrentItem();
              if (anime_item) {
                int value = anime_item->GetMyLastWatchedEpisode();
                ExecuteAction(L"EditEpisode(" + ToWstr(value + 1) + L")");
              }
            } else if (pnkd->wVKey == VK_SUBTRACT) {
              auto anime_item = AnimeDatabase.GetCurrentItem();
              if (anime_item) {
                int value = anime_item->GetMyLastWatchedEpisode();
                ExecuteAction(L"EditEpisode(" + ToWstr(value - 1) + L")");
              }
            // Edit score
            } else if (pnkd->wVKey >= '0' && pnkd->wVKey <= '9') {
              ExecuteAction(L"EditScore(" + ToWstr(pnkd->wVKey - '0') + L")");
            } else if (pnkd->wVKey >= VK_NUMPAD0 && pnkd->wVKey <= VK_NUMPAD9) {
              ExecuteAction(L"EditScore(" + ToWstr(pnkd->wVKey - VK_NUMPAD0) + L")");
            }
          }
          break;
        }
      }
      break;
    }

    // Custom draw
    case NM_CUSTOMDRAW: {
      return OnListCustomDraw(lParam);
    }
  }

  return 0;
}

void AnimeListDialog::ListView::DrawProgressBar(HDC hdc, RECT* rc, UINT uItemState, const anime::Item* anime_item) {
  win32::Dc dc = hdc;
  win32::Rect rcBar = *rc;

  int eps_aired = anime_item->GetLastAiredEpisodeNumber(true);
  int eps_watched = anime_item->GetMyLastWatchedEpisode(true);
  int eps_estimate = anime_item->GetEpisodeCount(true);
  int eps_total = anime_item->GetEpisodeCount(false);

  if (eps_watched > eps_aired) eps_aired = -1;
  if (eps_aired == eps_total) eps_aired = -1;
  if (eps_watched == 0) eps_watched = -1;

  bool draw_text = Settings.Program.List.progress_show_eps == TRUE;
  if (draw_text)
    rcBar.right -= 50;

  // Draw border
  rcBar.Inflate(-4, -4);
  UI.list_progress.border.Draw(dc.Get(), &rcBar);
  // Draw background
  rcBar.Inflate(-1, -1);
  UI.list_progress.background.Draw(dc.Get(), &rcBar);

  win32::Rect rcAired = rcBar;
  win32::Rect rcAvail = rcBar;
  win32::Rect rcButton = rcBar;
  win32::Rect rcSeparator = rcBar;
  win32::Rect rcWatched = rcBar;

  // Draw progress
  if (eps_watched > -1 || eps_aired > -1) {
    float ratio_aired = 0.0f;
    float ratio_watched = 0.0f;
    if (eps_estimate) {
      if (eps_aired > 0) {
        ratio_aired = static_cast<float>(eps_aired) / static_cast<float>(eps_estimate);
      }
      if (eps_watched > 0) {
        ratio_watched = static_cast<float>(eps_watched) / static_cast<float>(eps_estimate);
      }
    } else {
      ratio_aired = eps_aired > -1 ? 0.8f : 0.0f;
      ratio_watched = eps_watched > 0 ? (eps_aired > -1 ? 0.75f : 0.8f) : 0.0f;
    }

    if (eps_aired > -1) {
      rcAired.right = static_cast<int>((rcAired.Width()) * ratio_aired) + rcAired.left;
    }
    if (ratio_watched > -1) {
      rcWatched.right = static_cast<int>((rcWatched.Width()) * ratio_watched) + rcWatched.left;
    }

    // Draw aired episodes
    if (Settings.Program.List.progress_show_aired && eps_aired > 0) {
      UI.list_progress.aired.Draw(dc.Get(), &rcAired);
    }

    // Draw progress
    if (anime_item->GetMyStatus() == mal::MYSTATUS_WATCHING || anime_item->GetMyRewatching()) {
      UI.list_progress.watching.Draw(dc.Get(), &rcWatched);  // Watching
    } else if (anime_item->GetMyStatus() == mal::MYSTATUS_COMPLETED) {
      UI.list_progress.completed.Draw(dc.Get(), &rcWatched); // Completed
    } else if (anime_item->GetMyStatus() == mal::MYSTATUS_DROPPED) {
      UI.list_progress.dropped.Draw(dc.Get(), &rcWatched);   // Dropped
    } else {
      UI.list_progress.completed.Draw(dc.Get(), &rcWatched); // Completed / On hold / Plan to watch
    }
  }

  // Draw episode availability
  if (Settings.Program.List.progress_show_available) {
    if (eps_total > 0) {
      float width = static_cast<float>(rcBar.Width()) / static_cast<float>(eps_total);
      int available_episode_count = static_cast<int>(anime_item->GetAvailableEpisodeCount());
      for (int i = eps_watched + 1; i <= available_episode_count; i++) {
        if (i > 0 && anime_item->IsEpisodeAvailable(i)) {
          rcAvail.left = static_cast<int>(rcBar.left + (width * (i - 1)));
          rcAvail.right = static_cast<int>(rcAvail.left + width + 1);
          UI.list_progress.available.Draw(dc.Get(), &rcAvail);
        }
      }
    } else {
      if (anime_item->IsNewEpisodeAvailable()) {
        rcAvail.left = eps_watched > -1 ? rcWatched.right : rcWatched.left;
        rcAvail.right = rcAvail.left + static_cast<int>((rcBar.Width()) * 0.05f);
        UI.list_progress.available.Draw(dc.Get(), &rcAvail);
      }
    }
  }

  // Draw separators
  if (eps_watched > -1) {
    rcSeparator.left = rcWatched.right;
    rcSeparator.right = rcWatched.right + 1;
    UI.list_progress.separator.Draw(dc.Get(), &rcSeparator);
  }
  if (eps_aired > -1) {
    rcSeparator.left = rcAired.right;
    rcSeparator.right = rcAired.right + 1;
    UI.list_progress.separator.Draw(dc.Get(), &rcSeparator);
  }

  // Draw buttons
  if (uItemState & CDIS_SELECTED) {
    // Draw decrement button
    if (button_visible[0]) {
      rcButton = rcBar;
      rcButton.right = rcButton.left + 9;
      dc.FillRect(rcButton, UI.list_progress.button.value[0]);
      rcButton.Inflate(-1, -4);
      dc.FillRect(rcButton, UI.list_progress.background.value[0]);
    }
    // Draw increment button
    if (button_visible[1]) {
      rcButton = rcBar;
      rcButton.right = rcBar.right;
      rcButton.left = rcButton.right - 9;
      dc.FillRect(rcButton, UI.list_progress.button.value[0]);
      rcButton.Inflate(-1, -4);
      dc.FillRect(rcButton, UI.list_progress.background.value[0]);
      rcButton.Inflate(-3, 3);
      dc.FillRect(rcButton, UI.list_progress.background.value[0]);
    }
  }

  // Draw text
  if (draw_text) {
    wstring text;
    win32::Rect rcText = *rc;
    COLORREF text_color = dc.GetTextColor();
    dc.SetBkMode(TRANSPARENT);

    // Separator
    rcText.left = rcBar.right;
    dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
    dc.DrawText(L"/", 1, rcText,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    dc.SetTextColor(text_color);

    // Episodes watched
    text = mal::TranslateNumber(eps_watched, L"0");
    rcText.right -= (rcText.Width() / 2) + 4;
    if (eps_watched < 1)
      dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
    dc.DrawText(text.c_str(), text.length(), rcText,
                DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    dc.SetTextColor(text_color);

    // Total episodes
    text = mal::TranslateNumber(eps_total, L"?");
    rcText.left = rcText.right + 8;
    rcText.right = rc->right;
    if (eps_total < 1)
      dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
    dc.DrawText(text.c_str(), text.length(), rcText,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    dc.SetTextColor(text_color);
  }

  // Don't destroy the DC
  dc.DetachDC();
}

LRESULT AnimeListDialog::OnListCustomDraw(LPARAM lParam) {
  LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);

  switch (pCD->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
      return CDRF_NOTIFYITEMDRAW;
    case CDDS_ITEMPREPAINT:
      return CDRF_NOTIFYSUBITEMDRAW;
    case CDDS_PREERASE:
    case CDDS_ITEMPREERASE:
      return CDRF_NOTIFYPOSTERASE;

    case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
      auto anime_item = AnimeDatabase.FindItem(static_cast<int>(pCD->nmcd.lItemlParam));
      // Alternate background color
      if ((pCD->nmcd.dwItemSpec % 2) && !listview.IsGroupViewEnabled()) {
        pCD->clrTextBk = RGB(248, 248, 248);
      }
      // Change text color
      if (!anime_item) return CDRF_NOTIFYPOSTPAINT;
      if (anime_item->IsNewEpisodeAvailable()) {
        if (Settings.Program.List.highlight) {
          pCD->clrText = GetSysColor(pCD->iSubItem == 0 ? COLOR_HIGHLIGHT : COLOR_WINDOWTEXT);
        }
      }
      // Indicate currently playing
      if (anime_item->GetPlaying()) {
        pCD->clrTextBk = theme::COLOR_LIGHTGREEN;
        static HFONT hFontDefault = ChangeDCFont(pCD->nmcd.hdc, nullptr, -1, true, -1, -1);
        static HFONT hFontBold = reinterpret_cast<HFONT>(GetCurrentObject(pCD->nmcd.hdc, OBJ_FONT));
        SelectObject(pCD->nmcd.hdc, pCD->iSubItem == 0 ? hFontBold : hFontDefault);
        return CDRF_NEWFONT | CDRF_NOTIFYPOSTPAINT;
      }
      return CDRF_NOTIFYPOSTPAINT;
    }
    
    case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM: {
      auto anime_item = AnimeDatabase.FindItem(static_cast<int>(pCD->nmcd.lItemlParam));
      if (!anime_item) return CDRF_DODEFAULT;

      // Draw progress bar
      if (pCD->iSubItem == 1) {
        win32::Rect rcItem;
        if (win32::GetWinVersion() < win32::VERSION_VISTA) {
          listview.GetSubItemRect(pCD->nmcd.dwItemSpec, pCD->iSubItem, &rcItem);
        } else {
          rcItem = pCD->nmcd.rc;
        }
        if (!rcItem.IsEmpty()) {
          listview.DrawProgressBar(pCD->nmcd.hdc, &rcItem, pCD->nmcd.uItemState, anime_item);
        }
      }
      return CDRF_DODEFAULT;
    }

    default: {
      return CDRF_DODEFAULT;
    }
  }
}

// =============================================================================

/* Tab control */

LRESULT AnimeListDialog::OnTabNotify(LPARAM lParam) {
  switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
    // Tab select
    case TCN_SELCHANGE: {
      int index = static_cast<int>(tab.GetItemParam(tab.GetCurrentlySelected()));
      RefreshList(index);
      RefreshTabs(index, false);
      break;
    }
  }

  return 0;
}

// =============================================================================

int AnimeListDialog::GetListIndex(int anime_id) {
  if (IsWindow())
    for (int i = 0; i < listview.GetItemCount(); i++)
      if (static_cast<int>(listview.GetItemParam(i)) == anime_id)
        return i;
  return -1;
}

void AnimeListDialog::RefreshList(int index) {
  if (!IsWindow()) return;
  
  // Remember last index
  static int last_index = 1;
  if (index > 0) last_index = index;
  if (index == -1) index = last_index;
  if (!MainDialog.search_bar.filters.text.empty()) index = 0;

  // Hide list to avoid visual defects and gain performance
  listview.Hide();
  listview.EnableGroupView(index == 0 && win32::GetWinVersion() > win32::VERSION_XP);
  listview.DeleteAllItems();

  // Add items
  int group_index = -1, icon_index = 0, status = 0;
  vector<int> group_count(7);
  for (auto it = AnimeDatabase.items.begin(); it != AnimeDatabase.items.end(); ++it) {
    status = it->second.GetMyStatus();
    if (status == index || index == 0 || (index == mal::MYSTATUS_WATCHING && it->second.GetMyRewatching())) {
      if (MainDialog.search_bar.filters.CheckItem(it->second)) {
        group_index = win32::GetWinVersion() > win32::VERSION_XP ? status : -1;
        icon_index = it->second.GetPlaying() ? ICON16_PLAY : StatusToIcon(it->second.GetAiringStatus());
        group_count.at(status)++;
        int i = listview.GetItemCount();
        listview.InsertItem(i, group_index, icon_index, 
          0, nullptr, LPSTR_TEXTCALLBACK, static_cast<LPARAM>(it->second.GetId()));
        listview.SetItem(i, 2, mal::TranslateNumber(it->second.GetMyScore()).c_str());
        listview.SetItem(i, 3, mal::TranslateType(it->second.GetType()).c_str());
        listview.SetItem(i, 4, mal::TranslateDateToSeason(it->second.GetDate(anime::DATE_START)).c_str());
      }
    }
  }

  // Set group headers
  for (int i = mal::MYSTATUS_NOTINLIST; i <= mal::MYSTATUS_PLANTOWATCH; i++) {
    if (index == 0 && i != mal::MYSTATUS_UNKNOWN) {
      wstring text = mal::TranslateMyStatus(i, false);
      text += group_count.at(i) > 0 ? L" (" + ToWstr(group_count.at(i)) + L")" : L"";
      listview.SetGroupText(i, text.c_str());
    }
  }

  // Sort items
  listview.Sort(listview.GetSortColumn(), listview.GetSortOrder(), 
    listview.GetSortType(listview.GetSortColumn()), ListViewCompareProc);

  // Show again
  listview.Show(SW_SHOW);
}

void AnimeListDialog::RefreshListItem(int anime_id) {
  int index = GetListIndex(anime_id);
  if (index > -1) {
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    listview.SetItem(index, 2, mal::TranslateNumber(anime_item->GetMyScore()).c_str());
    listview.RedrawItems(index, index, true);
  }
}

void AnimeListDialog::RefreshTabs(int index, bool redraw) {
  if (!IsWindow()) return;

  // Remember last index
  static int last_index = 1;
  if (index == 6) index--;
  if (index == last_index) redraw = false;
  if (index > 0) last_index = index;
  if (index == -1) index = last_index;
  if (!MainDialog.search_bar.filters.text.empty()) index = 0;
  
  if (!redraw) return;
  
  // Hide
  tab.Hide();

  // Refresh text
  for (int i = 1; i <= 6; i++) {
    if (i != 5) {
      tab.SetItemText(i == 6 ? 4 : i - 1, mal::TranslateMyStatus(i, true).c_str());
    }
  }

  // Select related tab
  tab.SetCurrentlySelected(--index);

  // Show again
  tab.Show(SW_SHOW);
}