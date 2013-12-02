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

#include "dlg_anime_list.h"

#include "dlg_main.h"
#include "dlg_torrent.h"

#include "library/anime_db.h"
#include "library/anime_filter.h"
#include "library/anime_util.h"
#include "base/common.h"
#include "base/gfx.h"
#include "sync/myanimelist.h"
#include "taiga/resource.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "taiga/taiga.h"
#include "ui/menu.h"
#include "ui/theme.h"

#include "win/win_gdi.h"

class AnimeListDialog AnimeListDialog;

// =============================================================================

AnimeListDialog::AnimeListDialog()
    : current_id_(anime::ID_UNKNOWN),
      current_status_(anime::kWatching) {
}

BOOL AnimeListDialog::OnInitDialog() {
  // Create tab control
  tab.Attach(GetDlgItem(IDC_TAB_MAIN));

  // Create main list
  listview.parent = this;
  listview.Attach(GetDlgItem(IDC_LIST_MAIN));
  listview.SetExtendedStyle(LVS_EX_AUTOSIZECOLUMNS |
                            LVS_EX_DOUBLEBUFFER |
                            LVS_EX_FULLROWSELECT |
                            LVS_EX_INFOTIP |
                            LVS_EX_LABELTIP |
                            LVS_EX_TRACKSELECT);
  listview.SetBkImage(UI.list_background.bitmap, UI.list_background.flags,
                      UI.list_background.offset_x, UI.list_background.offset_y);
  listview.SetHoverTime(60 * 1000);
  listview.SetImageList(UI.ImgList16.GetHandle());
  listview.Sort(0, 1, 0, ListViewCompareProc);
  listview.SetTheme();

  // Create list tooltips
  listview.tooltips.Create(listview.GetWindowHandle());
  listview.tooltips.SetDelayTime(30000, -1, 0);

  // Insert list columns
  listview.InsertColumn(0, GetSystemMetrics(SM_CXSCREEN), 340, LVCFMT_LEFT, L"Anime title");
  listview.InsertColumn(1, 200, 200, LVCFMT_CENTER, L"Progress");
  listview.InsertColumn(2,  62,  62, LVCFMT_CENTER, L"Score");
  listview.InsertColumn(3,  62,  62, LVCFMT_CENTER, L"Type");
  listview.InsertColumn(4, 105, 105, LVCFMT_RIGHT,  L"Season");

  // Insert tabs and list groups
  listview.InsertGroup(anime::kNotInList, anime::TranslateMyStatus(anime::kNotInList, false).c_str());
  for (int i = anime::kWatching; i <= anime::kPlanToWatch; i++) {
    if (i != anime::kUnknownMyStatus) {
      tab.InsertItem(i - 1, anime::TranslateMyStatus(i, true).c_str(), (LPARAM)i);
      listview.InsertGroup(i, anime::TranslateMyStatus(i, false).c_str());
    }
  }

  // Track mouse leave event for the list view
  TRACKMOUSEEVENT tme = {0};
  tme.cbSize = sizeof(TRACKMOUSEEVENT);
  tme.dwFlags = TME_LEAVE;
  tme.hwndTrack = listview.GetWindowHandle();
  TrackMouseEvent(&tme);

  // Refresh
  RefreshList(anime::kWatching);
  RefreshTabs(anime::kWatching);

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
          win::Rect rect_edit;
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
        SetSharedCursor(allow_drop ? IDC_ARROW : IDC_NO);
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
        
        int anime_id = GetCurrentId();
        auto anime_item = GetCurrentItem();
        if (!anime_item)
          break;

        int tab_index = tab.HitTest();
        if (tab_index > -1) {
          int status = tab.GetItemParam(tab_index);
          if (anime_item->IsInList()) {
            ExecuteAction(L"EditStatus(" + ToWstr(status) + L")", 0, anime_id);
          } else {
            ExecuteAction(L"AddToListAs(" + ToWstr(status) + L")", 0, anime_id);
          }
          break;
        }

        wstring text = Settings.Program.List.english_titles ? 
          anime_item->GetEnglishTitle(true) : anime_item->GetTitle();

        POINT pt;
        GetCursorPos(&pt);
        win::Rect rect_edit;
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
              TorrentDialog.Search(Settings.RSS.Torrent.search_url, anime_id);
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
        win::Dc dc = dis->hDC;
        win::Rect rect = dis->rcItem;

        int anime_id = dis->itemData;
        auto anime_item = AnimeDatabase.FindItem(anime_id);
        if (!anime_item) return TRUE;

        if ((dis->itemState & ODS_SELECTED) == ODS_SELECTED) {
          dc.FillRect(rect, theme::COLOR_LIGHTBLUE);
        }
        rect.Inflate(-2, -2);
        dc.FillRect(rect, theme::COLOR_LIGHTGRAY);

        // Draw image
        win::Rect rect_image = rect;
        rect_image.right = rect_image.left + static_cast<int>(rect_image.Height() / 1.4);
        dc.FillRect(rect_image, theme::COLOR_GRAY);
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
        COLORREF text_color = dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
        wstring text = ToWstr(anime_item->GetMyLastWatchedEpisode()) + L"/" +
                       ToWstr(anime_item->GetEpisodeCount());
        dc.DrawText(text.c_str(), -1, rect, 
                    DT_END_ELLIPSIS | DT_NOPREFIX | DT_SINGLELINE);
        dc.SetTextColor(text_color);
        dc.SetBkMode(bk_mode);

        // Draw progress bar
        rect.left -= 2;
        rect.top += 12;
        rect.bottom = rect.top + 12;
        rect.right -= 8;
        listview.DrawProgressBar(dc.Get(), &rect, dis->itemID, 0, *anime_item);

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
      win::Rect rcWindow(0, 0, size.cx, size.cy);
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

AnimeListDialog::ListView::ListView()
    : dragging(false), hot_item(-1) {
  button_visible[0] = false;
  button_visible[1] = false;
  button_visible[2] = false;
}

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

void AnimeListDialog::ListView::RefreshItem(int index) {
  for (int i = 0; i < 3; i++) {
    button_rect[i].SetEmpty();
    button_visible[i] = false;
  }

  hot_item = index;

  if (index < 0) {
    tooltips.DeleteTip(0);
    tooltips.DeleteTip(1);
    tooltips.DeleteTip(2);
    return;
  }

  int anime_id = GetItemParam(index);
  auto anime_item = AnimeDatabase.FindItem(anime_id);

  if (!anime_item || !anime_item->IsInList())
    return;

  if (anime_item->GetMyStatus() != anime::kDropped) {
    if (anime_item->GetMyStatus() != anime::kCompleted || anime_item->GetMyRewatching()) {
      if (anime_item->GetMyLastWatchedEpisode() > 0)
        button_visible[0] = true;
      if (anime_item->GetEpisodeCount() > anime_item->GetMyLastWatchedEpisode() ||
          anime_item->GetEpisodeCount() == 0)
        button_visible[1] = true;

      win::Rect rect_item;
      GetSubItemRect(index, 1, &rect_item);
      rect_item.right -= 50;
      rect_item.Inflate(-5, -5);
      button_rect[0].Copy(rect_item);
      button_rect[0].right = button_rect[0].left + 9;
      button_rect[1].Copy(rect_item);
      button_rect[1].left = button_rect[1].right - 9;

      POINT pt;
      ::GetCursorPos(&pt);
      ::ScreenToClient(GetWindowHandle(), &pt);
      if (rect_item.PtIn(pt)) {
        if (anime_item->IsInList()) {
          wstring text;
          if (anime_item->IsNewEpisodeAvailable())
            AppendString(text, L"#" + ToWstr(anime_item->GetMyLastWatchedEpisode() + 1) + L" is on computer");
          if (anime_item->GetLastAiredEpisodeNumber() > anime_item->GetMyLastWatchedEpisode())
            AppendString(text, L"#" + ToWstr(anime_item->GetLastAiredEpisodeNumber()) + L" is available for download");
          if (!text.empty()) {
            tooltips.AddTip(2, text.c_str(), nullptr, &rect_item, false);
          } else {
            tooltips.DeleteTip(2);
          }
        }
      } else {
        tooltips.DeleteTip(2);
      }
      if ((button_visible[0] && button_rect[0].PtIn(pt)) || 
          (button_visible[1] && button_rect[1].PtIn(pt))) {
        tooltips.AddTip(0, L"-1 episode", nullptr, &button_rect[0], false);
        tooltips.AddTip(1, L"+1 episode", nullptr, &button_rect[1], false);
      } else {
        tooltips.DeleteTip(0);
        tooltips.DeleteTip(1);
      }
    }
  }

  button_visible[2] = true;

  win::Rect rect_item;
  GetSubItemRect(index, 2, &rect_item);
  rect_item.Inflate(-8, -2);
  button_rect[2].Copy(rect_item);
}

LRESULT AnimeListDialog::ListView::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Middle mouse button
    case WM_MBUTTONDOWN: {
      int item_index = HitTest();
      if (item_index > -1) {
        SetSelectedItem(item_index);
        int anime_id = parent->GetCurrentId();
        switch (Settings.Program.List.middle_click) {
          case 1:
            ExecuteAction(L"EditAll", 0, anime_id);
            break;
          case 2:
            ExecuteAction(L"OpenFolder", 0, anime_id);
            break;
          case 3:
            ExecuteAction(L"PlayNext", 0, anime_id);
            break;
          case 4:
            ExecuteAction(L"Info", 0, anime_id);
            break;
        }
      }
      break;
    }

    // Mouse leave
    case WM_MOUSELEAVE: {
      int item_index = GetNextItem(-1, LVIS_SELECTED);
      if (item_index != hot_item)
        RefreshItem(-1);
      break;
    }

    // Set cursor
    case WM_SETCURSOR: {
      POINT pt;
      ::GetCursorPos(&pt);
      ::ScreenToClient(GetWindowHandle(), &pt);
      if ((button_visible[0] && button_rect[0].PtIn(pt)) || 
          (button_visible[1] && button_rect[1].PtIn(pt)) ||
          (button_visible[2] && button_rect[2].PtIn(pt))) {
        SetSharedCursor(IDC_HAND);
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

    // Delete all items
    case LVN_DELETEALLITEMS: {
      SetCurrentId(anime::ID_UNKNOWN);
      listview.button_visible[0] = false;
      listview.button_visible[1] = false;
      break;
    }

    // Item select
    case LVN_ITEMCHANGED: {
      auto lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      auto anime_id = static_cast<int>(lplv->lParam);
      SetCurrentId(anime_id);
      if (lplv->uNewState)
        listview.RefreshItem(lplv->iItem);
      break;
    }

    // Item hover
    case LVN_HOTTRACK: {
      auto lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      listview.RefreshItem(lplv->iItem);
      break;
    }

    // Double click
    case NM_DBLCLK: {
      if (listview.GetSelectedCount() > 0) {
        bool on_button = false;
        int anime_id = GetCurrentId();
        auto lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
        if (listview.button_visible[0] && listview.button_rect[0].PtIn(lpnmitem->ptAction)) {
          ExecuteAction(L"DecrementEpisode", 0, anime_id);
          on_button = true;
        } else if (listview.button_visible[1] && listview.button_rect[1].PtIn(lpnmitem->ptAction)) {
          ExecuteAction(L"IncrementEpisode", 0, anime_id);
          on_button = true;
        }
        if (on_button) {
          int list_index = GetListIndex(GetCurrentId());
          listview.RefreshItem(list_index);
          listview.RedrawItems(list_index, list_index, true);
        } else {
          switch (Settings.Program.List.double_click) {
            case 1:
              ExecuteAction(L"EditAll", 0, anime_id);
              break;
            case 2:
              ExecuteAction(L"OpenFolder", 0, anime_id);
              break;
            case 3:
              ExecuteAction(L"PlayNext", 0, anime_id);
              break;
            case 4:
              ExecuteAction(L"Info", 0, anime_id);
              break;
          }
        }
      }
      break;
    }

    // Left click
    case NM_CLICK: {
      if (pnmh->hwndFrom == listview.GetWindowHandle()) {
        if (listview.GetSelectedCount() > 0) {
          int anime_id = GetCurrentId();
          auto lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(lParam);
          if (listview.button_visible[0] && listview.button_rect[0].PtIn(lpnmitem->ptAction)) {
            ExecuteAction(L"DecrementEpisode", 0, anime_id);
          } else if (listview.button_visible[1] && listview.button_rect[1].PtIn(lpnmitem->ptAction)) {
            ExecuteAction(L"IncrementEpisode", 0, anime_id);
          } else if (listview.button_visible[2] && listview.button_rect[2].PtIn(lpnmitem->ptAction)) {
            POINT pt = {listview.button_rect[2].left, listview.button_rect[2].bottom};
            ClientToScreen(listview.GetWindowHandle(), &pt);
            UpdateAnimeMenu(GetCurrentItem());
            ExecuteAction(UI.Menus.Show(GetWindowHandle(), pt.x, pt.y, L"EditScore"), 0, anime_id);
          }
          int list_index = GetListIndex(GetCurrentId());
          listview.RefreshItem(list_index);
          listview.RedrawItems(list_index, list_index, true);
        }
      }
      break;
    }

    // Right click
    case NM_RCLICK: {
      if (pnmh->hwndFrom == listview.GetWindowHandle()) {
        if (listview.GetSelectedCount() > 0) {
          int anime_id = GetCurrentId();
          auto anime_item = GetCurrentItem();
          UpdateAllMenus(anime_item);
          int index = listview.HitTest(true);
          if (anime_item->IsInList()) {
            switch (index) {
              // Score
              case 2:
                ExecuteAction(UI.Menus.Show(g_hMain, 0, 0, L"EditScore"), 0, anime_id);
                break;
              // Other
              default:
                ExecuteAction(UI.Menus.Show(g_hMain, 0, 0, L"RightClick"), 0, anime_id);
                break;
            }
            UpdateAllMenus(anime_item);
          } else {
            UpdateSearchListMenu(true);
            ExecuteAction(UI.Menus.Show(g_hMain, 0, 0, L"SearchList"), 0, anime_id);
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
      int anime_id = GetCurrentId();
      auto anime_item = GetCurrentItem();
      switch (pnkd->wVKey) {
        // Delete item
        case VK_DELETE: {
          if (listview.GetSelectedCount() > 0)
            ExecuteAction(L"EditDelete()", 0, anime_id);
          break;
        }
        default: {
          if (listview.GetSelectedCount() > 0 &&
              GetKeyState(VK_CONTROL) & 0x8000) {
            // Edit episode
            if (pnkd->wVKey == VK_ADD) {
              if (anime_item) {
                int value = anime_item->GetMyLastWatchedEpisode();
                ExecuteAction(L"EditEpisode(" + ToWstr(value + 1) + L")", 0, anime_id);
              }
            } else if (pnkd->wVKey == VK_SUBTRACT) {
              if (anime_item) {
                int value = anime_item->GetMyLastWatchedEpisode();
                ExecuteAction(L"EditEpisode(" + ToWstr(value - 1) + L")", 0, anime_id);
              }
            // Edit score
            } else if (pnkd->wVKey >= '0' && pnkd->wVKey <= '9') {
              ExecuteAction(L"EditScore(" + ToWstr(pnkd->wVKey - '0') + L")", 0, anime_id);
            } else if (pnkd->wVKey >= VK_NUMPAD0 && pnkd->wVKey <= VK_NUMPAD9) {
              ExecuteAction(L"EditScore(" + ToWstr(pnkd->wVKey - VK_NUMPAD0) + L")", 0, anime_id);
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

void AnimeListDialog::ListView::DrawProgressBar(HDC hdc, RECT* rc, int index,
                                                UINT uItemState, anime::Item& anime_item) {
  win::Dc dc = hdc;
  win::Rect rcBar = *rc;

  int eps_aired = anime_item.GetLastAiredEpisodeNumber(true);
  int eps_watched = anime_item.GetMyLastWatchedEpisode(true);
  int eps_estimate = anime_item.GetEpisodeCount(true);
  int eps_total = anime_item.GetEpisodeCount(false);

  if (eps_watched > eps_aired) eps_aired = -1;
  if (eps_watched == 0) eps_watched = -1;

  rcBar.right -= 50;

  // Draw border
  rcBar.Inflate(-4, -4);
  UI.list_progress.border.Draw(dc.Get(), &rcBar);
  // Draw background
  rcBar.Inflate(-1, -1);
  UI.list_progress.background.Draw(dc.Get(), &rcBar);

  win::Rect rcAired = rcBar;
  win::Rect rcAvail = rcBar;
  win::Rect rcButton = rcBar;
  win::Rect rcSeparator = rcBar;
  win::Rect rcWatched = rcBar;

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
      ratio_aired = eps_aired > -1 ? 0.85f : 0.0f;
      ratio_watched = eps_watched > 0 ? 0.8f : 0.0f;
    }
    if (ratio_watched > 1.0f) {
      // The number of watched episodes is greater than the number of total episodes
      ratio_watched = 1.0f;
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
    if (anime_item.GetMyStatus() == anime::kWatching || anime_item.GetMyRewatching()) {
      UI.list_progress.watching.Draw(dc.Get(), &rcWatched);  // Watching
    } else if (anime_item.GetMyStatus() == anime::kCompleted) {
      UI.list_progress.completed.Draw(dc.Get(), &rcWatched); // Completed
    } else if (anime_item.GetMyStatus() == anime::kDropped) {
      UI.list_progress.dropped.Draw(dc.Get(), &rcWatched);   // Dropped
    } else {
      UI.list_progress.completed.Draw(dc.Get(), &rcWatched); // Completed / On hold / Plan to watch
    }
  }

  // Draw episode availability
  if (Settings.Program.List.progress_show_available) {
    if (eps_estimate > 0) {
      float width = static_cast<float>(rcBar.Width()) / static_cast<float>(eps_estimate);
      int available_episode_count = static_cast<int>(anime_item.GetAvailableEpisodeCount());
      for (int i = eps_watched + 1; i <= available_episode_count; i++) {
        if (i > 0 && anime_item.IsEpisodeAvailable(i)) {
          rcAvail.left = static_cast<int>(rcBar.left + (width * (i - 1)));
          rcAvail.right = static_cast<int>(rcAvail.left + width + 1);
          UI.list_progress.available.Draw(dc.Get(), &rcAvail);
        }
      }
    } else {
      if (anime_item.IsNewEpisodeAvailable()) {
        float ratio_avail = anime_item.IsEpisodeAvailable(eps_aired) ? 0.85f : 0.83f;
        rcAvail.right = rcAvail.left + static_cast<int>((rcAvail.Width()) * ratio_avail);
        rcAvail.left = rcWatched.right;
        UI.list_progress.available.Draw(dc.Get(), &rcAvail);
      }
    }
  }

  // Draw separators
  if (eps_watched > 0 && (eps_watched < eps_total || eps_total == 0)) {
    rcSeparator.left = rcWatched.right;
    rcSeparator.right = rcWatched.right + 1;
    UI.list_progress.separator.Draw(dc.Get(), &rcSeparator);
  }
  if (eps_aired > 0 && (eps_aired < eps_total || eps_total == 0)) {
    rcSeparator.left = rcAired.right;
    rcSeparator.right = rcAired.right + 1;
    UI.list_progress.separator.Draw(dc.Get(), &rcSeparator);
  }

  // Draw buttons
  if (index > -1 && index == hot_item) {
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
  wstring text;
  win::Rect rcText = *rc;
  COLORREF text_color = dc.GetTextColor();
  dc.SetBkMode(TRANSPARENT);

  // Separator
  rcText.left = rcBar.right;
  dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
  dc.DrawText(L"/", 1, rcText,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  dc.SetTextColor(text_color);

  // Episodes watched
  text = anime::TranslateNumber(eps_watched, L"0");
  rcText.right -= (rcText.Width() / 2) + 4;
  if (eps_watched < 1) {
    dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
  } else if (eps_watched > eps_total && eps_total) {
    dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHT));
  } else if (eps_watched < eps_total && anime_item.GetMyStatus() == anime::kCompleted) {
    dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHT));
  }
  dc.DrawText(text.c_str(), text.length(), rcText,
              DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
  dc.SetTextColor(text_color);

  // Total episodes
  text = anime::TranslateNumber(eps_total, L"?");
  rcText.left = rcText.right + 8;
  rcText.right = rc->right;
  if (eps_total < 1)
    dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
  dc.DrawText(text.c_str(), text.length(), rcText,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  dc.SetTextColor(text_color);

  // Don't destroy the DC
  dc.DetachDC();
}

void AnimeListDialog::ListView::DrawScoreBox(HDC hdc, RECT* rc, int index,
                                             UINT uItemState, anime::Item& anime_item) {
  win::Dc dc = hdc;
  win::Rect rcBox = *rc;

  if (index > -1 && index == hot_item) {
    rcBox.Inflate(-8, -2);
    UI.list_progress.border.Draw(dc.Get(), &rcBox);
    rcBox.Inflate(-1, -1);
    UI.list_progress.background.Draw(dc.Get(), &rcBox);
    rcBox.Inflate(-4, 0);

    COLORREF text_color = dc.GetTextColor();
    dc.SetBkMode(TRANSPARENT);

    wstring text = anime::TranslateNumber(anime_item.GetMyScore());
    dc.SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
    dc.DrawText(text.c_str(), text.length(), rcBox, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    dc.EditFont(nullptr, 5);
    dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
    dc.DrawText(L"\u25BC", 1, rcBox, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    dc.SetTextColor(text_color);
  }

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
      if ((pCD->nmcd.dwItemSpec % 2) && !listview.IsGroupViewEnabled())
        pCD->clrTextBk = ChangeColorBrightness(GetSysColor(COLOR_WINDOW), -0.03f);
      // Change text color
      if (!anime_item) return CDRF_NOTIFYPOSTPAINT;
      pCD->clrText = GetSysColor(COLOR_WINDOWTEXT);
      switch (pCD->iSubItem) {
        case 0:
          if (anime_item->IsNewEpisodeAvailable() && Settings.Program.List.highlight)
            pCD->clrText = GetSysColor(COLOR_HIGHLIGHT);
          break;
        case 2:
          if (!anime_item->GetMyScore())
            pCD->clrText = GetSysColor(COLOR_GRAYTEXT);
          break;
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
      if (pCD->iSubItem == 1 || pCD->iSubItem == 2) {
        win::Rect rcItem;
        if (win::GetWinVersion() < win::VERSION_VISTA) {
          listview.GetSubItemRect(pCD->nmcd.dwItemSpec, pCD->iSubItem, &rcItem);
        } else {
          rcItem = pCD->nmcd.rc;
        }
        if (!rcItem.IsEmpty()) {
          if (pCD->iSubItem == 1) {
            listview.DrawProgressBar(pCD->nmcd.hdc, &rcItem, pCD->nmcd.dwItemSpec,
                                     pCD->nmcd.uItemState, *anime_item);
          } else if (pCD->iSubItem == 2) {
            listview.DrawScoreBox(pCD->nmcd.hdc, &rcItem, pCD->nmcd.dwItemSpec,
                                  pCD->nmcd.uItemState, *anime_item);
          }
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
      int tab_index = tab.GetCurrentlySelected();
      int index = static_cast<int>(tab.GetItemParam(tab_index));
      RefreshList(index);
      break;
    }
  }

  return 0;
}

// =============================================================================

int AnimeListDialog::GetCurrentId() {
  if (current_id_ > anime::ID_UNKNOWN)
    if (!AnimeDatabase.FindItem(current_id_))
      current_id_ = anime::ID_UNKNOWN;

  return current_id_;
}

anime::Item* AnimeListDialog::GetCurrentItem() {
  anime::Item* item = nullptr;

  if (current_id_ > anime::ID_UNKNOWN) {
    item = AnimeDatabase.FindItem(current_id_);
    if (!item) current_id_ = anime::ID_UNKNOWN;
  }

  return item;
}

void AnimeListDialog::SetCurrentId(int anime_id) {
  if (anime_id > anime::ID_UNKNOWN)
    if (!AnimeDatabase.FindItem(anime_id))
      anime_id = anime::ID_UNKNOWN;

  current_id_ = anime_id;
}

int AnimeListDialog::GetListIndex(int anime_id) {
  if (IsWindow())
    for (int i = 0; i < listview.GetItemCount(); i++)
      if (static_cast<int>(listview.GetItemParam(i)) == anime_id)
        return i;
  
  return -1;
}

void AnimeListDialog::RefreshList(int index) {
  if (!IsWindow()) return;
  
  // Remember current status
  if (index > anime::kNotInList)
    current_status_ = index;

  // Hide list to avoid visual defects and gain performance
  listview.Hide();
  listview.DeleteAllItems();
  listview.RefreshItem(-1);

  // Enable group view
  bool group_view = !MainDialog.search_bar.filters.text.empty() &&
                    win::GetWinVersion() > win::VERSION_XP;
  listview.EnableGroupView(group_view);

  // Add items to list
  vector<int> group_count(7);
  int group_index = -1;
  int icon_index = 0;
  int i = 0;
  for (auto it = AnimeDatabase.items.begin(); it != AnimeDatabase.items.end(); ++it) {
    anime::Item& anime_item = it->second;
    
    if (!anime_item.IsInList())
      continue;
    if (!group_view)
      if (current_status_ != anime_item.GetMyStatus())
        if (current_status_ != anime::kWatching || !it->second.GetMyRewatching())
          continue;
    if (!MainDialog.search_bar.filters.CheckItem(anime_item))
      continue;
    
    group_count.at(anime_item.GetMyStatus())++;
    group_index = group_view ? anime_item.GetMyStatus() : -1;
    icon_index = anime_item.GetPlaying() ? ICON16_PLAY : StatusToIcon(anime_item.GetAiringStatus());
    i = listview.GetItemCount();
    
    listview.InsertItem(i, group_index, icon_index, 
                        0, nullptr, LPSTR_TEXTCALLBACK,
                        static_cast<LPARAM>(anime_item.GetId()));
    listview.SetItem(i, 2, anime::TranslateNumber(anime_item.GetMyScore()).c_str());
    listview.SetItem(i, 3, anime::TranslateType(anime_item.GetType()).c_str());
    listview.SetItem(i, 4, anime::TranslateDateToSeason(anime_item.GetDate(anime::DATE_START)).c_str());
  }

  // Set group headers
  if (group_view) {
    for (int i = anime::kNotInList; i <= anime::kPlanToWatch; i++) {
      if (i != anime::kUnknownMyStatus) {
        wstring text = anime::TranslateMyStatus(i, false);
        text += group_count.at(i) > 0 ? L" (" + ToWstr(group_count.at(i)) + L")" : L"";
        listview.SetGroupText(i, text.c_str());
      }
    }
  }

  // Sort items
  listview.Sort(listview.GetSortColumn(),
                listview.GetSortOrder(),
                listview.GetSortType(listview.GetSortColumn()),
                ListViewCompareProc);

  // Show again
  listview.Show(SW_SHOW);
}

void AnimeListDialog::RefreshListItem(int anime_id) {
  int index = GetListIndex(anime_id);
  if (index > -1) {
    auto anime_item = AnimeDatabase.FindItem(anime_id);
    listview.SetItem(index, 2, anime::TranslateNumber(anime_item->GetMyScore()).c_str());
    listview.RedrawItems(index, index, true);
  }
}

void AnimeListDialog::RefreshTabs(int index) {
  if (!IsWindow()) return;

  // Remember last index
  if (index > anime::kNotInList)
    current_status_ = index;
  
  // Hide
  tab.Hide();

  // Refresh text
  for (int i = 1; i <= 6; i++)
    if (i != 5)
      tab.SetItemText(i == 6 ? 4 : i - 1, anime::TranslateMyStatus(i, true).c_str());

  // Select related tab
  bool group_view = !MainDialog.search_bar.filters.text.empty();
  int tab_index = current_status_;
  if (group_view) {
    tab_index = -1;
  } else if (tab_index == 6) {
    tab_index = 4;
  } else {
    tab_index--;
  }
  tab.SetCurrentlySelected(tab_index);

  // Show again
  tab.Show(SW_SHOW);
}