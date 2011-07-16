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
  
  BOOL DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void OnCancel();
  BOOL OnInitDialog();
  LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  void OnOK();
  
public:
  CFeedFilter m_Filter;
  HFONT m_hfHeader;

private:
  CTab m_Tab;

private:
  // Page: Basic
  class CDialogPageBasic : public CDialog {
  public:
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog();
  public:
    bool BuildFilter(CFeedFilter& filter);
    void BuildOptions(int mode);
    void BuildOptions2(int mode);
  public:
    CComboBox m_ComboOption1, m_ComboOption2;
  public:
    CFeedFilterWindow* m_Parent;
  } m_PageBasic;
  // Page: Advanced
  class CDialogPageAdvanced : public CDialog {
  public:
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog();
    LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
  public:
    bool BuildFilter(CFeedFilter& filter);
  private:
    void AddConditionToList(const CFeedFilterCondition& condition, int index = -1);
    void RefreshConditionsList();
  public:
    CComboBox m_ComboAction, m_ComboMatch;
    CEdit m_Edit;
    CListView m_ListCondition, m_ListAnime;
    CToolbar m_Toolbar;
  public:
    CFeedFilterWindow* m_Parent;
  } m_PageAdvanced;
};

extern CFeedFilterWindow FeedFilterWindow;

#endif // DLG_FEED_FILTER_H