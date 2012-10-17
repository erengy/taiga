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
  listview.Attach(GetDlgItem(IDC_LIST_MAIN));
  listview.SetExtendedStyle(LVS_EX_AUTOSIZECOLUMNS | LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  listview.SetImageList(UI.ImgList16.GetHandle());
  listview.Sort(0, 1, 0, ListViewCompareProc);
  listview.SetTheme();

  // Insert list columns
  listview.InsertColumn(0, GetSystemMetrics(SM_CXSCREEN), 340, LVCFMT_LEFT, L"Anime title");
  listview.InsertColumn(1, 160, 160, LVCFMT_CENTER, L"Progress");
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

  listview.parent = this;

  // Success
  return TRUE;
}

// =============================================================================

INT_PTR AnimeListDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    // Drag list item
    case WM_MOUSEMOVE: {
      if (listview.dragging) {
        listview.drag_image.DragMove(LOWORD(lParam) + 8, HIWORD(lParam) + 8);
        SetCursor(LoadCursor(nullptr, tab.HitTest() > -1 ? IDC_ARROW : IDC_NO));
      }
      break;
    }
    case WM_LBUTTONUP: {
      if (listview.dragging) {
        listview.drag_image.DragLeave(g_hMain);
        listview.drag_image.EndDrag();
        listview.drag_image.Destroy();
        listview.dragging = false;
        ReleaseCapture();
        int tab_index = tab.HitTest();
        if (tab_index > -1) {
          int status = tab.GetItemParam(tab_index);
          ExecuteAction(L"EditStatus(" + ToWstr(status) + L")");
        }
      }
      break;
    }

    // Forward mouse wheel messages to the list
    case WM_MOUSEWHEEL: {
      return listview.SendMessage(uMsg, wParam, lParam);
    }

    // Back & forward buttons
    case WM_XBUTTONUP: {
      int index = tab.GetCurrentlySelected();
      int count = tab.GetItemCount();
      switch (HIWORD(wParam)) {
        case XBUTTON1: index--; break;
        case XBUTTON2: index++; break;
      }
      if (index < 0 || index > count - 1) return TRUE;
      tab.SetCurrentlySelected(index);
      index++; if (index == 5) index = 6;
      RefreshList(index);
      return TRUE;
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
      //rcWindow.Inflate(-ScaleX(WIN_CONTROL_MARGIN), -ScaleY(WIN_CONTROL_MARGIN));
      // Resize tab
      tab.SetPosition(nullptr, rcWindow);
      // Resize list
      tab.AdjustRect(nullptr, FALSE, &rcWindow);
      rcWindow.left -= 3; rcWindow.top -= 1;
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
      GetSubItemRect(GetNextItem(-1, LVIS_SELECTED), 1, &rect_item);
      win32::Rect rect_button[2];
      rect_button[0].Copy(rect_item);
      rect_button[1].Copy(rect_item);
      rect_button[0].right = rect_button[0].left + 16;
      rect_button[1].left = rect_button[1].right - 16;

      if ((button_visible[0] && rect_button[0].PtIn(pt)) || 
          (button_visible[1] && rect_button[1].PtIn(pt))) {
        ::SetCursor(reinterpret_cast<HCURSOR>(
          ::LoadImage(nullptr, IDC_HAND, IMAGE_CURSOR, 0, 0, LR_SHARED)));
        return TRUE;
      }
      break;
    }

    // Back & forward buttons
    case WM_XBUTTONUP: {
      return parent->DialogProc(hwnd, uMsg, wParam, lParam);
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
        listview.drag_image.DragEnter(GetWindowHandle(), pt.x, pt.y);
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
        if (anime_item) {
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
          win32::Rect rect_button[2];
          rect_button[0].Copy(rect_item);
          rect_button[1].Copy(rect_item);
          rect_button[0].right = rect_button[0].left + 16;
          rect_button[1].left = rect_button[1].right - 16;

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
              case 2:
                ExecuteAction(UI.Menus.Show(g_hMain, 0, 0, L"EditScore"));
                break;
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
          plvdi->item.pszText = const_cast<LPWSTR>(anime_item->GetTitle().data());
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
          if (listview.GetSelectedCount() > 0) {
            ExecuteAction(L"EditDelete()");
          }
          break;
        }
        // Check episodes
        case VK_F5: {
          ExecuteAction(L"CheckEpisodes()");
          break;
        }
        default: {
          if (GetKeyState(VK_CONTROL) & 0x8000) {
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
      if (anime_item->GetAiringStatus() == mal::STATUS_NOTYETAIRED) {
        pCD->clrText = GetSysColor(COLOR_GRAYTEXT);
      } else if (anime_item->IsNewEpisodeAvailable()) {
        if (Settings.Program.List.highlight) {
          pCD->clrText = GetSysColor(pCD->iSubItem == 0 ? COLOR_HIGHLIGHT : COLOR_WINDOWTEXT);
        }
      }
      // Indicate currently playing
      if (anime_item->GetPlaying()) {
        pCD->clrTextBk = RGB(230, 255, 230);
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
        int eps_buffer   = anime_item->GetMyLastWatchedEpisode(true);
        int eps_watched  = anime_item->GetMyLastWatchedEpisode(false);
        int eps_estimate = anime_item->GetEpisodeCount(true);
        int eps_total    = anime_item->GetEpisodeCount(false);
        if (eps_watched > eps_buffer) eps_watched = -1;
        if (eps_buffer == eps_watched) eps_buffer = -1;
        if (eps_watched == 0) eps_watched = -1;

        win32::Rect rcItem;
        if (win32::GetWinVersion() < win32::VERSION_VISTA) {
          listview.GetSubItemRect(pCD->nmcd.dwItemSpec, pCD->iSubItem, &rcItem);
        } else {
          rcItem = pCD->nmcd.rc;
        }
        if (rcItem.IsEmpty()) return CDRF_DODEFAULT;
        win32::Dc hdc = pCD->nmcd.hdc;
        win32::Rect rcText = rcItem;
        
        // Draw border
        rcItem.Inflate(-2, -2);
        UI.list_progress.border.Draw(hdc.Get(), &rcItem);
        // Draw background
        rcItem.Inflate(-1, -1);
        UI.list_progress.background.Draw(hdc.Get(), &rcItem);
        win32::Rect rcAvail = rcItem;
        win32::Rect rcBuffer = rcItem;
        win32::Rect rcButton = rcItem;
        
        // Draw progress
        if (eps_watched > -1 || eps_buffer > -1) {
          float ratio_watched = 0.0f, ratio_buffer = 0.0f;
          if (eps_estimate) {
            if (eps_watched > 0) {
              ratio_watched = static_cast<float>(eps_watched) / static_cast<float>(eps_estimate);
            }
            if (eps_buffer > 0) {
              ratio_buffer = static_cast<float>(eps_buffer) / static_cast<float>(eps_estimate);
            }
          } else {
            ratio_watched = eps_buffer > -1 ? 0.75f : 0.8f;
            ratio_buffer = eps_buffer > -1 ? 0.8f : 0.0f;
          }

          if (eps_buffer > -1) {
            rcBuffer.right = static_cast<int>((rcBuffer.right - rcBuffer.left) * ratio_buffer) + rcBuffer.left;
          }
          if (Settings.Program.List.progress_mode == LIST_PROGRESS_AVAILABLEEPS && eps_buffer > -1) {
            rcItem.right = rcBuffer.right;
          } else {
            rcItem.right = static_cast<int>((rcItem.right - rcItem.left) * ratio_watched) + rcItem.left;
          }

          // Draw buffer
          if (Settings.Program.List.progress_mode == LIST_PROGRESS_QUEUEDEPS && eps_buffer > 0) {
            UI.list_progress.buffer.Draw(hdc.Get(), &rcBuffer);
          }

          // Draw progress
          if (anime_item->GetMyStatus() == mal::MYSTATUS_WATCHING || anime_item->GetMyRewatching()) {
            UI.list_progress.watching.Draw(hdc.Get(), &rcItem);  // Watching
          } else if (anime_item->GetMyStatus() == mal::MYSTATUS_COMPLETED) {
            UI.list_progress.completed.Draw(hdc.Get(), &rcItem); // Completed
          } else if (anime_item->GetMyStatus() == mal::MYSTATUS_DROPPED) {
            UI.list_progress.dropped.Draw(hdc.Get(), &rcItem);   // Dropped
          } else {
            UI.list_progress.completed.Draw(hdc.Get(), &rcItem); // Completed / On hold / Plan to watch
          }
        }

        // Draw episode availability
        if (Settings.Program.List.progress_mode == LIST_PROGRESS_AVAILABLEEPS) {
          if (eps_total > 0) {
            float width = static_cast<float>(rcAvail.Width()) / static_cast<float>(eps_total);
            int available_episode_count = static_cast<int>(anime_item->GetAvailableEpisodeCount());
            for (int i = max(eps_buffer, eps_watched) + 1; i <= available_episode_count; i++) {
              if (i > 0 && anime_item->IsEpisodeAvailable(i)) {
                rcBuffer.left = static_cast<int>(rcAvail.left + (width * (i - 1)));
                rcBuffer.right = static_cast<int>(rcBuffer.left + width + 1);
                UI.list_progress.buffer.Draw(hdc.Get(), &rcBuffer);
              }
            }
          } else {
            if (anime_item->IsNewEpisodeAvailable()) {
              rcBuffer.left = eps_buffer > -1 ? rcBuffer.right : (eps_watched > -1 ? rcItem.right : rcItem.left);
              rcBuffer.right = rcBuffer.left + static_cast<int>((rcAvail.right - rcAvail.left) * 0.05f);
              UI.list_progress.buffer.Draw(hdc.Get(), &rcBuffer);
            }
          }
        }

        // Draw separator
        if (eps_watched > -1 || eps_buffer > -1) {
          rcBuffer.left = rcItem.right;
          rcBuffer.right = rcItem.right + 1;
          UI.list_progress.separator.Draw(hdc.Get(), &rcBuffer);
        }

        // Draw buttons
        if (pCD->nmcd.uItemState & CDIS_SELECTED) {
          rcButton.Inflate(3, 1);
          // Draw decrement button
          if (listview.button_visible[0]) {
            rcButton.left += 1;
            UI.ImgList16.Draw(ICON16_MINUS_SMALL, hdc.Get(), rcButton.left, rcButton.top);
          }
          // Draw increment button
          if (listview.button_visible[1]) {
            rcButton.left = rcButton.right - 16;
            UI.ImgList16.Draw(ICON16_PLUS_SMALL, hdc.Get(), rcButton.left, rcButton.top);
          }
        }

        // Draw text
        if (pCD->nmcd.uItemState & CDIS_SELECTED || pCD->nmcd.uItemState & CDIS_HOT || Settings.Program.List.progress_show_eps) {
          if (eps_watched == -1) eps_watched = 0;
          wstring text = mal::TranslateNumber(eps_buffer > -1 ? eps_buffer : eps_watched) + L"/" + mal::TranslateNumber(eps_total);
          if (!Settings.Program.List.progress_show_eps) text += L" episodes";
          if (anime_item->GetMyRewatching()) text += L" (rw)";
          hdc.EditFont(nullptr, 7);
          hdc.SetBkMode(TRANSPARENT);
          rcText.Offset(1, 1);
          hdc.SetTextColor(RGB(255, 255, 255));
          hdc.DrawText(text.c_str(), text.length(), rcText, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
          rcText.Offset(-1, -1);
          hdc.SetTextColor(RGB(0, 0, 0));
          hdc.DrawText(text.c_str(), text.length(), rcText, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
          DeleteObject(hdc.DetachFont());
        }

        // Return
        hdc.DetachDC();
        return CDRF_DODEFAULT;
      }
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

  // Change window title
  wstring title = APP_TITLE;
  if (!Settings.Account.MAL.user.empty()) {
    title += L" \u2013 " + Settings.Account.MAL.user;
  }
  MainDialog.SetText(title.c_str());
  
  // Remember last index
  static int last_index = 1;
  if (index > 0) last_index = index;
  if (index == -1) index = last_index;
  if (!AnimeFilters.text.empty()) index = 0;

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
      if (AnimeFilters.CheckItem(it->second)) {
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
  if (!AnimeFilters.text.empty()) index = 0;
  
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