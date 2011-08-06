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

#include "../std.h"
#include "../common.h"
#include "dlg_settings.h"
#include "dlg_torrent.h"
#include "../feed.h"
#include "../gfx.h"
#include "../http.h"
#include "../resource.h"
#include "../settings.h"
#include "../string.h"
#include "../taiga.h"
#include "../theme.h"
#include "../win32/win_gdi.h"

CTorrentWindow TorrentWindow;

// =============================================================================

CTorrentWindow::CTorrentWindow() {
  RegisterDlgClass(L"TaigaTorrentW");
}

CTorrentWindow::~CTorrentWindow() {
}

BOOL CTorrentWindow::OnInitDialog() {
  // Set properties
  SetSizeMin(470, 260);

  // Create list
  m_List.Attach(GetDlgItem(IDC_LIST_TORRENT));
  m_List.EnableGroupView(true);
  m_List.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER | 
    LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP);
  m_List.SetImageList(UI.ImgList16.GetHandle());
  m_List.SetTheme();
  
  // Insert list columns
  m_List.InsertColumn(0, 240, 240, LVCFMT_LEFT,  L"Anime title");
  m_List.InsertColumn(1,  60,  60, LVCFMT_RIGHT, L"Episode");
  m_List.InsertColumn(2, 120, 120, LVCFMT_LEFT,  L"Group");
  m_List.InsertColumn(3,  70,  70, LVCFMT_RIGHT, L"Size");
  m_List.InsertColumn(4, 100, 100, LVCFMT_LEFT,  L"Video");
  m_List.InsertColumn(5, 250, 250, LVCFMT_LEFT,  L"Description");
  m_List.InsertColumn(6, 250, 250, LVCFMT_LEFT,  L"File name");
  // Insert list groups
  m_List.InsertGroup(0, L"Anime");
  m_List.InsertGroup(1, L"Batch");
  m_List.InsertGroup(2, L"Other");

  // Create main toolbar
  m_Toolbar.Attach(GetDlgItem(IDC_TOOLBAR_TORRENT));
  m_Toolbar.SetImageList(UI.ImgList16.GetHandle(), 16, 16);
  m_Toolbar.SendMessage(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS | TBSTYLE_EX_MIXEDBUTTONS);
  // Insert toolbar buttons
  BYTE fsStyle = BTNS_AUTOSIZE | BTNS_SHOWTEXT;
  m_Toolbar.InsertButton(0, Icon16_Refresh,  100, 1, fsStyle, 0, L"Check new torrents", NULL);
  m_Toolbar.InsertButton(1, 0, 0, 0, BTNS_SEP, NULL, NULL, NULL);
  m_Toolbar.InsertButton(2, Icon16_Download, 101, 1, fsStyle, 0, L"Download marked torrents", NULL);
  m_Toolbar.InsertButton(3, Icon16_Cross,    102, 1, fsStyle, 0, L"Discard all", NULL);
  m_Toolbar.InsertButton(4, 0, 0, 0, BTNS_SEP, NULL, NULL, NULL);
  m_Toolbar.InsertButton(5, Icon16_Settings, 103, 1, fsStyle, 0, L"Settings", NULL);

  // Create rebar
  m_Rebar.Attach(GetDlgItem(IDC_REBAR_TORRENT));
  // Insert rebar bands
  m_Rebar.InsertBand(NULL, 0, 0, 0, 0, 0, 0, 0, 0, 
    RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE, RBBS_NOGRIPPER);
  m_Rebar.InsertBand(m_Toolbar.GetWindowHandle(), GetSystemMetrics(SM_CXSCREEN), 0, 0, 0, 0, 0, 0, 
    HIWORD(m_Toolbar.GetButtonSize()) + (HIWORD(m_Toolbar.GetPadding()) / 2), 
    RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_SIZE | RBBIM_STYLE, RBBS_NOGRIPPER);

  // Create status bar
  m_Status.Attach(GetDlgItem(IDC_STATUSBAR_TORRENT));
  m_Status.SetImageList(UI.ImgList16.GetHandle());
  //m_Status.InsertPart(-1, 0, 0, 700, NULL, NULL);
  //m_Status.InsertPart(-1, 0, 0, 860, NULL, NULL);

  // Refresh list
  RefreshList();

  return TRUE;
}

// =============================================================================

void CTorrentWindow::ChangeStatus(wstring str, int panel_index) {
  // Change status text
  if (panel_index == 0) str = L"  " + str;
  m_Status.SetPanelText(panel_index, str.c_str());
}

void CTorrentWindow::EnableInput(bool enable) {
  // Enable/disable toolbar buttons
  m_Toolbar.EnableButton(0, enable);
  m_Toolbar.EnableButton(2, enable);
  // Enable/disable list
  m_List.Enable(enable);
}

void CTorrentWindow::RefreshList() {
  if (!IsWindow()) return;
  CFeed* pFeed = Aggregator.Get(FEED_CATEGORY_LINK);
  if (!pFeed) return;
  
  // Hide list to avoid visual defects and gain performance
  m_List.Hide();
  m_List.DeleteAllItems();

  // Add items
  for (auto it = pFeed->Items.begin(); it != pFeed->Items.end(); ++it) {
    if (Settings.RSS.Torrent.HideUnidentified && it->EpisodeData.AnimeId == ANIMEID_NOTINLIST) {
      continue;
    }
    wstring title, number, video;
    int group = TORRENT_ANIME, icon = StatusToIcon(0);
    if (it->Category == L"Batch" || 
      !IsNumeric(it->EpisodeData.Number)) {
        group = TORRENT_BATCH;
    }
    CAnime* anime = AnimeList.FindItem(it->EpisodeData.AnimeId);
    if (anime) {
      icon = StatusToIcon(anime->GetAiringStatus());
      title = anime->Series_Title;
    } else if (!it->EpisodeData.Title.empty()) {
      title = it->EpisodeData.Title;
    } else {
      group = TORRENT_OTHER;
      title = it->Title;
    }
    vector<int> numbers;
    SplitEpisodeNumbers(it->EpisodeData.Number, numbers);
    number = JoinEpisodeNumbers(numbers);
    if (!it->EpisodeData.Version.empty()) {
      number += L"v" + it->EpisodeData.Version;
    }
    video = it->EpisodeData.VideoType;
    if (!it->EpisodeData.Resolution.empty()) {
      if (!video.empty()) video += L" ";
      video += it->EpisodeData.Resolution;
    }
    int index = m_List.InsertItem(it - pFeed->Items.begin(), 
      group, icon, 0, NULL, title.c_str(), reinterpret_cast<LPARAM>(&(*it)));
    m_List.SetItem(index, 1, number.c_str());
    m_List.SetItem(index, 2, it->EpisodeData.Group.c_str());
    m_List.SetItem(index, 3, it->EpisodeData.FileSize.c_str());
    m_List.SetItem(index, 4, video.c_str());
    m_List.SetItem(index, 5, it->Description.c_str());
    m_List.SetItem(index, 6, it->EpisodeData.File.c_str());
    m_List.SetCheckState(index, it->Download);
  }

  // Show again
  m_List.Show();

  // Set icon
  HICON hIcon = pFeed->GetIcon();
  if (hIcon) hIcon = CopyIcon(hIcon);
  SetIconSmall(hIcon);

  // Set title
  wstring title = L"Torrents";
  if (!pFeed->Title.empty()) {
    title = pFeed->Title;
  } else if (!pFeed->Link.empty()) {
    CUrl url(pFeed->Link);
    title += L" (" + url.Host + L")";
  }
  if (!pFeed->Description.empty()) {
    title += L" - " + pFeed->Description;
  }
  SetText(title.c_str());
}

// =============================================================================

BOOL CTorrentWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
  CFeed* pFeed = Aggregator.Get(FEED_CATEGORY_LINK);
  if (!pFeed) return 0;
  
  // Toolbar
  switch (LOWORD(wParam)) {
    // Check new torrents
    case 100: {
      pFeed->Check(Settings.RSS.Torrent.Source);
      /**
      #ifdef _DEBUG
      pFeed->Read();
      pFeed->ExamineData();
      RefreshList();
      #endif
      /**/
      return TRUE;
    }
    // Download marked torrents
    case 101: {
      for (int i = 0; i < m_List.GetItemCount(); i++) {
        CFeedItem* pItem = reinterpret_cast<CFeedItem*>(m_List.GetItemParam(i));
        if (pItem) {
          bool check_state = m_List.GetCheckState(i) == TRUE;
          if (pItem->Download && !check_state) {
            // Discard items that have passed all filters but are unchecked by the user
            Aggregator.FileArchive.push_back(pItem->Title);
          }
          pItem->Download = check_state;
        }
      }
      pFeed->Download(-1);
      return TRUE;
    }
    // Discard marked torrents
    case 102: {
      for (int i = 0; i < m_List.GetItemCount(); i++) {
        if (m_List.GetCheckState(i) == TRUE) {
          CFeedItem* pItem = reinterpret_cast<CFeedItem*>(m_List.GetItemParam(i));
          if (pItem) {
            pItem->Download = false;
            m_List.SetCheckState(i, FALSE);
            Aggregator.FileArchive.push_back(pItem->Title);
          }
        }
      }
      return TRUE;
    }
    // Settings
    case 103: {
      ExecuteAction(L"Settings", 0, PAGE_TORRENT1);
      return TRUE;
    }
  }

  return FALSE;
}

LRESULT CTorrentWindow::OnNotify(int idCtrl, LPNMHDR pnmh) {
  CFeed* pFeed = Aggregator.Get(FEED_CATEGORY_LINK);
  if (!pFeed) return 0;

  // ListView control
  if (idCtrl == IDC_LIST_TORRENT) {
    switch (pnmh->code) {
      // Column click
      case LVN_COLUMNCLICK: {
        LPNMLISTVIEW lplv = (LPNMLISTVIEW)pnmh;
        int order = 1;
        if (lplv->iSubItem == m_List.GetSortColumn()) order = m_List.GetSortOrder() * -1;
        switch (lplv->iSubItem) {
          // Episode
          case 1:
            m_List.Sort(lplv->iSubItem, order, LISTSORTTYPE_NUMBER, ListViewCompareProc);
            break;
          // File size
          case 3:
            m_List.Sort(lplv->iSubItem, order, LISTSORTTYPE_FILESIZE, ListViewCompareProc);
            break;
          // Other columns
          default:
            m_List.Sort(lplv->iSubItem, order, LISTSORTTYPE_DEFAULT, ListViewCompareProc);
            break;
        }
        break;
      }

      // Double click
      case NM_DBLCLK: {
        if (m_List.GetSelectedCount() > 0) {
          LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
          if (lpnmitem->iItem == -1) break;
          CFeedItem* pItem = reinterpret_cast<CFeedItem*>(m_List.GetItemParam(lpnmitem->iItem));
          if (pItem) {
            pFeed->Download(pItem->Index);
          }
        }
        break;
      }

      // Right click
      case NM_RCLICK: {
        LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
        if (lpnmitem->iItem == -1) break;
        CFeedItem* pItem = reinterpret_cast<CFeedItem*>(m_List.GetItemParam(lpnmitem->iItem));
        if (pItem) {
          wstring answer = UI.Menus.Show(m_hWindow, 0, 0, L"TorrentListRightClick");
          if (answer == L"DownloadTorrent") {
            pFeed->Download(pItem->Index);
          } else if (answer == L"DiscardTorrent") {
            pItem->Download = false;
            m_List.SetCheckState(lpnmitem->iItem, FALSE);
            Aggregator.FileArchive.push_back(pItem->Title);
          } else if (answer == L"DiscardTorrents") {
            CAnime* anime = AnimeList.FindItem(pItem->EpisodeData.AnimeId);
            if (anime) {
              for (int i = 0; i < m_List.GetItemCount(); i++) {
                pItem = reinterpret_cast<CFeedItem*>(m_List.GetItemParam(i));
                if (pItem && pItem->EpisodeData.AnimeId == anime->Series_ID) {
                  pItem->Download = false;
                  m_List.SetCheckState(i, FALSE);
                }
              }
              Aggregator.FilterManager.AddFilter(
                FEED_FILTER_ACTION_DISCARD, FEED_FILTER_MATCH_ALL, true, 
                L"Discard \"" + anime->Series_Title + L"\"");
              Aggregator.FilterManager.Filters.back().AddCondition(
                FEED_FILTER_ELEMENT_ANIME_ID, FEED_FILTER_OPERATOR_IS, 
                ToWSTR(anime->Series_ID));
            }
          } else if (answer == L"MoreTorrents") {
            pFeed->Check(ReplaceVariables(
              L"http://www.nyaa.eu/?page=rss&cats=1_37&filter=2&term=%title%", // TEMP
              pItem->EpisodeData));
          } else if (answer == L"SearchMAL") {
            ExecuteAction(L"SearchAnime(" + pItem->EpisodeData.Title + L")");
          }
        }
        break;
      }

      // Custom draw
      case NM_CUSTOMDRAW: {
        LPNMLVCUSTOMDRAW pCD = reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmh);
        switch (pCD->nmcd.dwDrawStage) {
          case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;
          case CDDS_ITEMPREPAINT:
            return CDRF_NOTIFYSUBITEMDRAW;
          case CDDS_PREERASE:
          case CDDS_ITEMPREERASE:
            return CDRF_NOTIFYPOSTERASE;

          case CDDS_ITEMPREPAINT | CDDS_SUBITEM: {
            // Alternate background color
            if ((pCD->nmcd.dwItemSpec % 2) && !m_List.IsGroupViewEnabled()) {
              pCD->clrTextBk = RGB(248, 248, 248);
            }
            // Change text color
            CFeedItem* pItem = reinterpret_cast<CFeedItem*>(pCD->nmcd.lItemlParam);
            if (pItem) {
              if (pItem->EpisodeData.AnimeId == ANIMEID_NOTINLIST) {
                pCD->clrText = RGB(180, 180, 180);
              } else if (pItem->EpisodeData.NewEpisode) {
                pCD->clrText = GetSysColor(pCD->iSubItem == 1 ? COLOR_HIGHLIGHT : COLOR_WINDOWTEXT);
              }
            }
            return CDRF_NOTIFYPOSTPAINT;
          }
        }
      }
    }
  }

  return 0;
}

void CTorrentWindow::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      CRect rcWindow;
      rcWindow.Set(0, 0, size.cx, size.cy);
      rcWindow.Inflate(-ScaleX(WIN_CONTROL_MARGIN), -ScaleY(WIN_CONTROL_MARGIN));
      // Resize rebar
      m_Rebar.SendMessage(WM_SIZE, 0, 0);
      rcWindow.top += m_Rebar.GetBarHeight() + ScaleY(WIN_CONTROL_MARGIN / 2);
      // Resize status bar
      CRect rcStatus;
      m_Status.GetClientRect(&rcStatus);
      m_Status.SendMessage(WM_SIZE, 0, 0);
      rcWindow.bottom -= rcStatus.Height();
      // Resize list
      m_List.SetPosition(NULL, rcWindow);
    }
  }
}