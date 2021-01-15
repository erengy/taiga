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

#pragma once

#include <windows/win/common_controls.h>
#include <windows/win/dialog.h>

#include "track/feed_filter.h"

namespace ui {

class FeedFilterDialog : public win::Dialog {
public:
  FeedFilterDialog();
  ~FeedFilterDialog();

  INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  void OnCancel();
  BOOL OnCommand(WPARAM wParam, LPARAM lParam);
  BOOL OnInitDialog();
  void OnPaint(HDC hdc, LPPAINTSTRUCT lpps);

  void ChoosePage(int index);

  track::FeedFilter filter;

private:
  int current_page_;
  HICON icon_;
  win::Window main_instructions_label_;

  // Page
  class DialogPage : public win::Dialog {
  public:
    void Create(UINT uResourceID, FeedFilterDialog* parent, const RECT& rect);
    INT_PTR DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  public:
    FeedFilterDialog* parent;
  };

  // Page #0
  class DialogPage0 : public DialogPage {
  public:
    BOOL OnInitDialog();
    LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
    bool BuildFilter(track::FeedFilter& filter);
  public:
    win::ListView preset_list;
  } page_0_;

  // Page #1
  class DialogPage1 : public DialogPage {
  public:
    BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog();
    LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
    bool BuildFilter(track::FeedFilter& filter);
    void AddConditionToList(const track::FeedFilterCondition& condition, int index = -1);
    void RefreshConditionList();
    void ChangeAction();
  public:
    win::ComboBox action_combo, match_combo, option_combo;
    win::Edit name_text;
    win::ListView condition_list;
    win::Toolbar condition_toolbar;
  } page_1_;

  // Page #2
  class DialogPage2 : public DialogPage {
  public:
    BOOL OnInitDialog();
    LRESULT OnNotify(int idCtrl, LPNMHDR pnmh);
    bool BuildFilter(track::FeedFilter& filter);
  public:
    win::ListView anime_list;
  } page_2_;
};

extern FeedFilterDialog DlgFeedFilter;

}  // namespace ui
