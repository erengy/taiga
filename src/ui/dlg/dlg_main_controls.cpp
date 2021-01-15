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

#include <windowsx.h>

#include "base/string.h"
#include "media/library/queue.h"
#include "sync/sync.h"
#include "taiga/debug.h"
#include "taiga/resource.h"
#include "ui/dlg/dlg_anime_list.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dlg/dlg_season.h"
#include "ui/command.h"
#include "ui/dialog.h"
#include "ui/menu.h"
#include "ui/theme.h"
#include "ui/ui.h"

namespace ui {

////////////////////////////////////////////////////////////////////////////////

/* TreeView control */

LRESULT MainDialog::MainTree::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_SETCURSOR: {
      TVHITTESTINFO ht = {0};
      HitTest(&ht, true);
      if (IsSeparator(GetItemData(ht.hItem))) {
        SetSharedCursor(IDC_ARROW);
        return TRUE;
      }
      break;
    }
  }

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

void MainDialog::MainTree::RefreshHistoryCounter() {
  std::wstring text = L"History";
  int count = library::queue.GetItemCount();
  if (count > 0) text += L" (" + ToWstr(count) + L")";
  SetItem(hti.at(kSidebarItemHistory), text.c_str());
}

bool MainDialog::MainTree::IsSeparator(int page) {
  switch (page) {
    case kSidebarItemSeparator1:
    case kSidebarItemSeparator2:
      return true;
    default:
      return false;
  }
}

BOOL MainDialog::MainTree::IsVisible() {
  // This hack ensures that the sidebar is considered visible on startup
  if (!DlgMain.IsVisible())
    return TRUE;

  return TreeView::IsVisible();
}

LRESULT MainDialog::OnTreeNotify(LPARAM lParam) {
  LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);

  switch (pnmh->code) {
    // Custom draw
    case NM_CUSTOMDRAW: {
      LPNMTVCUSTOMDRAW pCD = reinterpret_cast<LPNMTVCUSTOMDRAW>(lParam);
      switch (pCD->nmcd.dwDrawStage) {
        case CDDS_PREPAINT:
          return CDRF_NOTIFYITEMDRAW;
        case CDDS_ITEMPREPAINT:
          return CDRF_NOTIFYPOSTPAINT;
        case CDDS_ITEMPOSTPAINT: {
          // Draw separator
          if (treeview.IsSeparator(pCD->nmcd.lItemlParam)) {
            win::Rect rcItem = pCD->nmcd.rc;
            win::Dc hdc = pCD->nmcd.hdc;
            hdc.FillRect(rcItem, ::GetSysColor(COLOR_3DFACE));
            rcItem.top += (rcItem.bottom - rcItem.top) / 2;
            //GradientRect(hdc.Get(), &rcItem, ::GetSysColor(COLOR_3DLIGHT), ::GetSysColor(COLOR_3DFACE), true);
            rcItem.bottom = rcItem.top + 2;
            hdc.FillRect(rcItem, ::GetSysColor(COLOR_3DHIGHLIGHT));
            rcItem.bottom -= 1;
            hdc.FillRect(rcItem, ::GetSysColor(COLOR_3DLIGHT));
            hdc.DetachDc();
          }
          return CDRF_DODEFAULT;
        }
      }
      break;
    }
    // Item select
    case TVN_SELCHANGING: {
      LPNMTREEVIEW pnmtv = reinterpret_cast<LPNMTREEVIEW>(lParam);
      switch (pnmtv->action) {
        case TVC_UNKNOWN:
          break;
        case TVC_BYMOUSE:
        case TVC_BYKEYBOARD: {
          int old_page = pnmtv->itemOld.lParam;
          int new_page = pnmtv->itemNew.lParam;
          // Prevent selection of separators
          if (treeview.IsSeparator(new_page) &&
              pnmtv->action == TVC_BYKEYBOARD) {
            UINT flag = new_page > old_page ? TVGN_NEXT : TVGN_PREVIOUS;
            HTREEITEM hti = treeview.GetNextItem(pnmtv->itemNew.hItem, flag);
            if (hti)
              new_page = treeview.GetItemData(hti);
          }
          if (!treeview.IsSeparator(new_page))
            navigation.SetCurrentPage(new_page);
          return TRUE;
        }
      }
      break;
    }
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////

/* Button control */

LRESULT MainDialog::CancelButton::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_SETCURSOR: {
      SetSharedCursor(IDC_HAND);
      return TRUE;
    }
  }

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT MainDialog::CancelButton::OnCustomDraw(LPARAM lParam) {
  LPNMCUSTOMDRAW pCD = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);

  switch (pCD->dwDrawStage) {
    case CDDS_PREPAINT: {
      win::Dc dc = pCD->hdc;
      dc.FillRect(pCD->rc, ::GetSysColor(COLOR_WINDOW));
      ui::Theme.GetImageList16().Draw(ui::kIcon16_Cross, dc.Get(), 0, 0);
      dc.DetachDc();
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

////////////////////////////////////////////////////////////////////////////////

/* Toolbar */

BOOL MainDialog::OnCommand(WPARAM wParam, LPARAM lParam) {
  // Toolbar
  switch (LOWORD(wParam)) {
    // Synchronize
    case kToolbarButtonSync:
      sync::Synchronize();
      return TRUE;
    // Settings
    case kToolbarButtonSettings:
      ShowDialog(ui::Dialog::Settings);
      return TRUE;
    // Debug
    case kToolbarButtonDebug:
      taiga::debug::Test();
      return TRUE;
  }

  // Search text
  if (HIWORD(wParam) == EN_CHANGE) {
    if (LOWORD(wParam) == IDC_EDIT_SEARCH) {
      int current_page = navigation.GetCurrentPage();
      auto& text = search_bar.text[current_page];
      edit.GetText(text);
      cancel_button.Show(text.empty() ? SW_HIDE : SW_SHOWNORMAL);
      switch (current_page) {
        case kSidebarItemAnimeList:
          if (search_bar.filters.text[current_page] != text) {
            if (text.empty() || text.size() > 1) {
              search_bar.filters.text[current_page] = text;
              DlgAnimeList.RefreshList();
              DlgAnimeList.RefreshTabs();
              return TRUE;
            }
          }
          break;
        case kSidebarItemSeasons:
          if (search_bar.filters.text[current_page] != text) {
            search_bar.filters.text[current_page] = text;
            DlgSeason.RefreshList();
            return TRUE;
          }
          break;
      }
    }
  }

  return FALSE;
}

LRESULT CALLBACK MainDialog::ToolbarWithMenu::HookProc(int code, WPARAM wParam, LPARAM lParam) {
  if (code == MSGF_MENU) {
    auto msg = reinterpret_cast<MSG*>(lParam);

    switch (msg->message) {
      case WM_MOUSEMOVE: {
        POINT pt = {GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
        ScreenToClient(DlgMain.toolbar_wm.toolbar->GetWindowHandle(), &pt);

        int button_index = DlgMain.toolbar_wm.toolbar->HitTest(pt);
        int button_count = DlgMain.toolbar_wm.toolbar->GetButtonCount();
        DWORD button_style = DlgMain.toolbar_wm.toolbar->GetButtonStyle(button_index);

        if (button_index > -1 &&
            button_index < button_count &&
            button_index != DlgMain.toolbar_wm.button_index) {
          if (button_style & BTNS_DROPDOWN || button_style & BTNS_WHOLEDROPDOWN) {
            DlgMain.toolbar_wm.toolbar->SetHotItem(button_index, HICF_MOUSE);
            return 0L;
          }
        }

        break;
      }
    }
  }

  return CallNextHookEx(DlgMain.toolbar_wm.hook, code, wParam, lParam);
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
      auto lpnmhi = reinterpret_cast<LPNMTBHOTITEM>(lParam);
      if (toolbar_wm.hook && lpnmhi->idNew > 0) {
        toolbar_wm.button_index = -1;
        PostMessage(WM_CANCELMODE);
        PostMessage(WM_TAIGA_SHOWMENU);
      }
      break;
    }
  }

  return 0L;
}

void MainDialog::ToolbarWithMenu::ShowMenu() {
  button_index = toolbar->GetHotItem();

  TBBUTTON tbb;
  toolbar->GetButton(button_index, tbb);
  toolbar->PressButton(tbb.idCommand, TRUE);

  // Calculate point
  RECT rect;
  toolbar->SendMessage(TB_GETITEMRECT, button_index, reinterpret_cast<LPARAM>(&rect));
  POINT pt = {rect.left, rect.bottom};
  ClientToScreen(toolbar->GetWindowHandle(), &pt);

  // Hook
  if (hook)
    UnhookWindowsHookEx(hook);
  hook = SetWindowsHookEx(WH_MSGFILTER, &HookProc, NULL, GetCurrentThreadId());

  // Display menu
  ui::Menus.UpdateAll();
  std::wstring command;
  HWND hwnd = DlgMain.GetWindowHandle();
  #define SHOWUIMENU(id, name) \
    case id: command = ui::Menus.Show(hwnd, pt.x, pt.y, name); break;
  switch (tbb.idCommand) {
    SHOWUIMENU(100, L"File");
    SHOWUIMENU(101, L"Services");
    SHOWUIMENU(102, L"Tools");
    SHOWUIMENU(103, L"View");
    SHOWUIMENU(104, L"Help");
    SHOWUIMENU(kToolbarButtonFolders, L"Folders");
    SHOWUIMENU(kToolbarButtonTools, L"ExternalLinks");
  }
  #undef SHOWUIMENU

  // Unhook
  if (hook) {
    UnhookWindowsHookEx(hook);
    hook = nullptr;
  }

  toolbar->PressButton(tbb.idCommand, FALSE);
  if (button_index > -1)
    toolbar->SetHotItem(-1);

  if (!command.empty())
    ExecuteCommand(command);
}

////////////////////////////////////////////////////////////////////////////////

/* Statusbar */

LRESULT MainDialog::StatusBar::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  switch (uMsg) {
    case WM_SETCURSOR: {
      POINT pt;
      GetCursorPos(&pt);
      ScreenToClient(GetWindowHandle(), &pt);
      win::Rect rect;
      GetRect(2, &rect);
      if (rect.PtIn(pt)) {
        SetSharedCursor(IDC_HAND);
        return TRUE;
      }
      break;
    }
  }

  return WindowProcDefault(hwnd, uMsg, wParam, lParam);
}

LRESULT MainDialog::OnStatusbarNotify(LPARAM lParam) {
  auto pnmh = reinterpret_cast<LPNMHDR>(lParam);

  switch (pnmh->code) {
    case NM_CLICK: {
      auto pnm = reinterpret_cast<LPNMMOUSE>(lParam);
      // Handle click on timer
      if (pnm->dwItemSpec == 2) {
        if (ui::OnRecognitionCancelConfirm()) {
          CurrentEpisode.processed = true;
        }
        return TRUE;
      }
      break;
    }
  }

  return 0L;
}

}  // namespace ui