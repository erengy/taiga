/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2011, Eren Okka
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

#ifndef DLG_FEED_FILTER_H
#define DLG_FEED_FILTER_H

#include "../std.h"
#include "../feed.h"
#include "../win32/win_control.h"
#include "../win32/win_dialog.h"

// =============================================================================

class CFeedFilterWindow : public CDialog {
public:
  CFeedFilterWindow();
  virtual ~CFeedFilterWindow();

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void OnCancel();
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);

  void ChoosePage(int index);
  
public:
  int current_page_;
  CFeedFilter filter_;
  HFONT header_font_, main_instructions_font_;
  HICON icon_;
  CWindow main_instructions_label_;

private:
  // Page
  class CDialogPage : public CDialog {
  public:
    void Create(UINT uResourceID, CFeedFilterWindow* parent, const RECT& rect);
    INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  public:
    CFeedFilterWindow* parent_;
  };
  
  // Page #0
  class CDialogPage0 : public CDialogPage {
  public:
    BOOL OnInitDialog();
    LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
    bool BuildFilter(CFeedFilter& filter);
  public:
    CListView preset_list_;
  } m_Page0;

  // Page #1
  class CDialogPage1 : public CDialogPage {
  public:
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog();
    LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
    bool BuildFilter(CFeedFilter& filter);
    void AddConditionToList(const CFeedFilterCondition& condition, int index = -1);
    void RefreshConditionList();
  public:
    CComboBox action_combo_, match_combo_;
    CEdit name_text_;
    CListView condition_list_;
    CToolbar condition_toolbar_;
  } m_Page1;

  // Page #2
  class CDialogPage2 : public CDialogPage {
  public:
    BOOL OnInitDialog();
    LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
    bool BuildFilter(CFeedFilter& filter);
  public:
    CListView anime_list_;
  } m_Page2;
};

extern CFeedFilterWindow FeedFilterWindow;

#endif // DLG_FEED_FILTER_H