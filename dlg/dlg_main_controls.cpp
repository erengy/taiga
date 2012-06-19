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

#include "dlg_main.h"

#include "../anime_db.h"
#include "../anime_filter.h"
#include "../common.h"
#include "../debug.h"
#include "../event.h"
#include "../gfx.h"
#include "../myanimelist.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"

#include "../win32/win_gdi.h"

// =============================================================================

/* TreeView control */

void MainDialog::CMainTree::RefreshItems() {
  // Clear items
  DeleteAllItems();
  for (int i = 0; i < 7; i++) {
    htItem[i] = nullptr;
  }
  
  // My tralala
  htItem[0] = InsertItem(L"My Panel", -1, 0, nullptr);
  htItem[1] = InsertItem(L"My Profile", -1, 0, nullptr);
  htItem[2] = InsertItem(L"My History", -1, 0, nullptr);

  // Separator
  htItem[3] = InsertItem(nullptr, -1, -1, nullptr);
  
  // My Anime List
  htItem[4] = InsertItem(L"My Anime List", -1, 0, nullptr);
  InsertItem(mal::TranslateMyStatus(mal::MYSTATUS_WATCHING, true).c_str(), -1, 0, htItem[4]);
  InsertItem(mal::TranslateMyStatus(mal::MYSTATUS_COMPLETED, true).c_str(), -1, 0, htItem[4]);
  InsertItem(mal::TranslateMyStatus(mal::MYSTATUS_ONHOLD, true).c_str(), -1, 0, htItem[4]);
  InsertItem(mal::TranslateMyStatus(mal::MYSTATUS_DROPPED, true).c_str(), -1, 0, htItem[4]);
  InsertItem(mal::TranslateMyStatus(mal::MYSTATUS_PLANTOWATCH, true).c_str(), -1, 0, htItem[4]);
  Expand(htItem[4]);

  // Separator
  htItem[5] = InsertItem(nullptr, -1, -1, nullptr);

  // Foobar
  htItem[6] = InsertItem(L"Foo", -1, 0, nullptr);
  InsertItem(L"Foofoo", -1, 0, htItem[6]);
  InsertItem(L"Foobar", -1, 0, htItem[6]);
  InsertItem(L"Foobaz", -1, 0, htItem[6]);
  Expand(htItem[6]);
}

LRESULT MainDialog::OnTreeNotify(LPARAM lParam) {
  LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);

  switch (pnmh->code) {
    // Custom draw
    case NM_CUSTOMDRAW: {
      LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam);
      switch (pCD->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
          return CDRF_NOTIFYITEMDRAW;
        case CDDS_ITEMPREPAINT:
          return CDRF_NOTIFYPOSTPAINT;
        case CDDS_ITEMPOSTPAINT: {
          // Draw separator
          if (pCD->nmcd.lItemlParam == -1) {
            win32::Rect rcItem = pCD->nmcd.rc;
            win32::Dc hdc = pCD->nmcd.hdc;
            hdc.FillRect(rcItem, RGB(255, 255, 255));
            rcItem.Inflate(-8, 0);
            rcItem.top += (rcItem.bottom - rcItem.top) / 2;
            GradientRect(hdc.Get(), &rcItem, RGB(245, 245 ,245), RGB(255, 255, 255), true);
            rcItem.bottom = rcItem.top + 2;
            hdc.FillRect(rcItem, RGB(255, 255, 255));
            rcItem.bottom -= 1;
            hdc.FillRect(rcItem, RGB(230, 230, 230));
            hdc.DetachDC();
          }
          return CDRF_DODEFAULT;
        }
      }
    }
  }

  return 0;
}

// =============================================================================

/* ListView control */

int MainDialog::ListView::GetSortType(int column) {
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

LRESULT MainDialog::ListView::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

    // Back & forward buttons
    case WM_XBUTTONUP: {
      return parent->DialogProc(hwnd, uMsg, wParam, lParam);
    }
  }
  
  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT MainDialog::OnListNotify(LPARAM lParam) {
  LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
  switch (pnmh->code) {
    // Item drag
    case LVN_BEGINDRAG: {
      POINT pt = {0};
      LPNMLISTVIEW lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      listview.drag_image = listview.CreateDragImage(lplv->iItem, &pt);
      if (listview.drag_image.GetHandle()) {
        pt = lplv->ptAction;
        listview.drag_image.BeginDrag(0, 0, 0);
        listview.drag_image.DragEnter(g_hMain, pt.x, pt.y);
        listview.dragging = true;
        SetCapture();
      }
      break;
    }
    
    // Column click
    case LVN_COLUMNCLICK: {
      LPNMLISTVIEW lplv = (LPNMLISTVIEW)lParam;
      int order = 1;
      if (lplv->iSubItem == listview.GetSortColumn()) order = listview.GetSortOrder() * -1;
      listview.Sort(lplv->iSubItem, order, listview.GetSortType(lplv->iSubItem), ListViewCompareProc);
      break;
    }

    // Item select
    case LVN_ITEMCHANGED: {
      LPNMLISTVIEW lplv = reinterpret_cast<LPNMLISTVIEW>(lParam);
      auto anime_id = static_cast<int>(lplv->lParam);
      AnimeDatabase.SetCurrentId(anime_id);
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

    // Right click
    case NM_RCLICK: {
      if (pnmh->hwndFrom == listview.GetWindowHandle()) {
        if (listview.GetSelectedCount() > 0) {
          auto anime_item = AnimeDatabase.GetCurrentItem();
          UpdateAllMenus(anime_item);
          int index = listview.HitTest(true);
          if (anime_item->IsInList()) {
            ExecuteAction(UI.Menus.Show(g_hMain, 0, 0, index == 2 ? L"EditScore" : L"RightClick"));
            UpdateAllMenus(anime_item);
          } else {
            UpdateSearchListMenu(true);
            ExecuteAction(UI.Menus.Show(g_hMain, 0, 0, L"SearchList"), 
              0, static_cast<LPARAM>(anime_item->GetId()));
          }
        }
      } else if (pnmh->hwndFrom == listview.GetHeader()) {
        HDHITTESTINFO hdhti;
        ::GetCursorPos(&hdhti.pt);
        ::ScreenToClient(listview.GetHeader(), &hdhti.pt);
        if (::SendMessage(listview.GetHeader(), HDM_HITTEST, 0, reinterpret_cast<LPARAM>(&hdhti))) {
          if (hdhti.iItem == 3) {
            ExecuteAction(UI.Menus.Show(m_hWindow, 0, 0, L"FilterType"));
            UpdateAllMenus(AnimeDatabase.GetCurrentItem());
            return TRUE;
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

LRESULT MainDialog::OnListCustomDraw(LPARAM lParam) {
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

/* Button control */

LRESULT MainDialog::OnButtonCustomDraw(LPARAM lParam) {
  LPNMCUSTOMDRAW pCD = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);

  switch (pCD->dwDrawStage) {
    case CDDS_PREPAINT: {
      win32::Dc dc = pCD->hdc;
      dc.FillRect(pCD->rc, ::GetSysColor(COLOR_WINDOW));
      UI.ImgList16.Draw(ICON16_CROSS, dc.Get(), 0, 0);
      dc.DetachDC();
      return CDRF_SKIPDEFAULT;
    }
  }

  return 0;
}

/* Edit control */

LRESULT MainDialog::EditSearch::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_COMMAND: {
      if (HIWORD(wParam) == BN_CLICKED) {
        // Clear search text
        if (LOWORD(wParam) == IDC_BUTTON_CANCELSEARCH) {
          SetText(L"");
          return TRUE;
        }
      }
      break;
    }
  }
  
  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

// =============================================================================

/* Tab control */

LRESULT MainDialog::OnTabNotify(LPARAM lParam) {
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

/* Toolbar */

BOOL MainDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  // Toolbar
  switch (LOWORD(wParam)) {
    // Login
    case 200:
      ExecuteAction(L"LoginLogout");
      return TRUE;
    // Synchronize
    case 201:
      ExecuteAction(L"Synchronize");
      return TRUE;
    // MyAnimeList
    case 202:
      ExecuteAction(L"ViewPanel");
      return TRUE;
    // Season browser
    case 205:
      ExecuteAction(L"SeasonBrowser");
      return TRUE;
    // Torrents
    case 207:
      ExecuteAction(L"Torrents");
      return TRUE;
    // Filter
    case 209:
      ExecuteAction(L"Filter");
      return TRUE;
    // Settings
    case 210:
      ExecuteAction(L"Settings");
      return TRUE;
    // Debug
    case 212:
      debug::Test();
      return TRUE;
  }
  
  // Search text
  if (HIWORD(wParam) == EN_CHANGE) {
    if (LOWORD(wParam) == IDC_EDIT_SEARCH) {
      wstring text;
      edit.GetText(text);
      cancel_button.Show(text.empty() ? SW_HIDE : SW_SHOWNORMAL);
      if (search_bar.filter_list) {
        AnimeFilters.text = text;
        RefreshList();
        RefreshTabs();
        return TRUE;
      }
    }
  }

  return FALSE;
}

LRESULT CALLBACK MainDialog::ToolbarWithMenu::HookProc(int code, WPARAM wParam, LPARAM lParam) {
  if (code == MSGF_MENU) {
    MSG* msg = reinterpret_cast<MSG*>(lParam);

    switch (msg->message) {
      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN: {
        break;
      }

      case WM_MOUSEMOVE: {
        POINT pt = {LOWORD(msg->lParam), HIWORD(msg->lParam)};
        ScreenToClient(::MainDialog.toolbar_wm.toolbar->GetWindowHandle(), &pt);

        int button_index = ::MainDialog.toolbar_wm.toolbar->HitTest(pt);
        int button_count = ::MainDialog.toolbar_wm.toolbar->GetButtonCount();
        DWORD button_style = ::MainDialog.toolbar_wm.toolbar->GetButtonStyle(button_index);
        
        if (button_index > -1 && 
            button_index < button_count && 
            button_index != ::MainDialog.toolbar_wm.button_index) {
          if (button_style & BTNS_DROPDOWN || button_style & BTNS_WHOLEDROPDOWN) {
            ::MainDialog.toolbar_wm.toolbar->SendMessage(TB_SETHOTITEM, button_index, 0);
            return 0L;
          }
        }

        break;
      }
    }
  }
  
  return CallNextHookEx(::MainDialog.toolbar_wm.hook, code, wParam, lParam);
}

LRESULT MainDialog::OnToolbarNotify(LPARAM lParam) {
  switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
    // Dropdown button click
    case TBN_DROPDOWN: {
      LPNMTOOLBAR nmt = reinterpret_cast<LPNMTOOLBAR>(lParam);
      int toolbar_id = nmt->hdr.idFrom;
      switch (toolbar_id) {
        case IDC_TOOLBAR_MENU:
          toolbar_wm.toolbar = &toolbar_menu;
          break;
        case IDC_TOOLBAR_MAIN:
          toolbar_wm.toolbar = &toolbar_main;
          break;
        case IDC_TOOLBAR_SEARCH:
          toolbar_wm.toolbar = &toolbar_search;
          break;
      }
      toolbar_wm.ShowMenu();
      break;
    }

    // Show tooltips
    case TBN_GETINFOTIP: {
      NMTBGETINFOTIP* git = reinterpret_cast<NMTBGETINFOTIP*>(lParam);
      git->cchTextMax = INFOTIPSIZE;
      // Main toolbar
      if (git->hdr.hwndFrom == toolbar_main.GetWindowHandle()) {
        git->pszText = (LPWSTR)(toolbar_main.GetButtonTooltip(git->lParam));
      // Search toolbar
      } else if (git->hdr.hwndFrom == toolbar_search.GetWindowHandle()) {
        git->pszText = (LPWSTR)(toolbar_search.GetButtonTooltip(git->lParam));
      }
      break;
    }

    // Hot-tracking
    case TBN_HOTITEMCHANGE: {
      LPNMTBHOTITEM lpnmhi = reinterpret_cast<LPNMTBHOTITEM>(lParam);
      if (toolbar_wm.hook && lpnmhi->idNew > 0) {
        debug::Print(L"Old: " + ToWstr(lpnmhi->idOld) + L" | New: " + ToWstr(lpnmhi->idNew) + L"\n");
        toolbar_wm.toolbar->PressButton(lpnmhi->idOld, FALSE);
        toolbar_wm.toolbar->SendMessage(lpnmhi->idNew, TRUE);
        SendMessage(WM_CANCELMODE);
        PostMessage(WM_TAIGA_SHOWMENU);
      }
      break;
    }
  }

  return 0L;
}

void MainDialog::ToolbarWithMenu::ShowMenu() {
  button_index = toolbar->SendMessage(TB_GETHOTITEM);

  TBBUTTON tbb;
  toolbar->GetButton(button_index, tbb);
  toolbar->PressButton(tbb.idCommand, TRUE);
  
  // Calculate point
  RECT rect;
  toolbar->SendMessage(TB_GETITEMRECT, button_index, reinterpret_cast<LPARAM>(&rect));
  POINT pt = {rect.left, rect.bottom};
  ClientToScreen(toolbar->GetWindowHandle(), &pt);

  // Hook
  if (hook) UnhookWindowsHookEx(hook);
  hook = SetWindowsHookEx(WH_MSGFILTER, &HookProc, NULL, GetCurrentThreadId());

  // Display menu
  wstring action;
  HWND hwnd = ::MainDialog.GetWindowHandle();
  #define SHOWUIMENU(id, name) \
    case id: action = UI.Menus.Show(hwnd, pt.x, pt.y, name); break;
  switch (tbb.idCommand) {
    SHOWUIMENU(100, L"File");
    SHOWUIMENU(101, L"Account");
    SHOWUIMENU(102, L"List");
    SHOWUIMENU(103, L"Help");
    SHOWUIMENU(204, L"Folders");
    SHOWUIMENU(206, L"Tools");
    SHOWUIMENU(300, L"SearchBar");
  }
  #undef SHOWUIMENU

  // Unhook
  if (hook) {
    UnhookWindowsHookEx(hook);
    hook = nullptr;
  }

  toolbar->PressButton(tbb.idCommand, FALSE);

  if (!action.empty()) {
    ExecuteAction(action);
    UpdateAllMenus(AnimeDatabase.GetCurrentItem());
  }
}