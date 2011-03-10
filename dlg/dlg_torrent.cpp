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
#include "../gfx.h"
#include "../http.h"
#include "../resource.h"
#include "../rss.h"
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

BOOL CTorrentWindow::OnInitDialog() {
  // Set properties
  SetIconLarge(IDI_MAIN);
  SetIconSmall(IDI_MAIN);
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
  m_Toolbar.InsertButton(1, Icon16_Download, 101, 1, fsStyle, 0, L"Download selected torrents", NULL);
  m_Toolbar.InsertButton(2, 0, 0, 0, BTNS_SEP, NULL, NULL, NULL);
  m_Toolbar.InsertButton(3, Icon16_Settings, 103, 1, fsStyle, 0, L"Settings", NULL);

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

void CTorrentWindow::RefreshList() {
  if (!IsWindow()) return;
  
  // Hide list to avoid visual defects and gain performance
  m_List.Hide();
  m_List.DeleteAllItems();

  // Add items
  for (size_t i = 0; i < Torrents.Feed.Item.size(); i++) {
    if (Settings.RSS.Torrent.HideUnidentified && !Torrents.Feed.Item[i].EpisodeData.Index) {
      continue;
    }
    wstring title, number, video;
    int group = TORRENT_ANIME, icon = StatusToIcon(0);
    if (Torrents.Feed.Item[i].Category == L"Batch" || 
      !IsNumeric(Torrents.Feed.Item[i].EpisodeData.Number)) {
        group = TORRENT_BATCH;
    }
    if (Torrents.Feed.Item[i].EpisodeData.Index > 0) {
      icon = StatusToIcon(AnimeList.Item[Torrents.Feed.Item[i].EpisodeData.Index].Series_Status);
      title = AnimeList.Item[Torrents.Feed.Item[i].EpisodeData.Index].Series_Title;
    } else if (!Torrents.Feed.Item[i].EpisodeData.Title.empty()) {
      title = Torrents.Feed.Item[i].EpisodeData.Title;
    } else {
      group = TORRENT_OTHER;
      title = Torrents.Feed.Item[i].Title;
    }
    number = Torrents.Feed.Item[i].EpisodeData.Number;
    EraseLeft(number, L"0", false);
    if (!Torrents.Feed.Item[i].EpisodeData.Version.empty()) {
      number += L"v" + Torrents.Feed.Item[i].EpisodeData.Version;
    }
    video = Torrents.Feed.Item[i].EpisodeData.VideoType;
    if (!Torrents.Feed.Item[i].EpisodeData.Resolution.empty()) {
      if (!video.empty()) video += L" ";
      video += Torrents.Feed.Item[i].EpisodeData.Resolution;
    }
    int index = m_List.InsertItem(i, group, icon, 0, NULL, title.c_str(), 
      reinterpret_cast<LPARAM>(&Torrents.Feed.Item[i]));
    m_List.SetItem(index, 1, number.c_str());
    m_List.SetItem(index, 2, Torrents.Feed.Item[i].EpisodeData.Group.c_str());
    m_List.SetItem(index, 3, Torrents.Feed.Item[i].Size.c_str());
    m_List.SetItem(index, 4, video.c_str());
    m_List.SetItem(index, 5, Torrents.Feed.Item[i].Description.c_str());
    m_List.SetItem(index, 6, Torrents.Feed.Item[i].EpisodeData.File.c_str());
    m_List.SetCheckState(index, Torrents.Feed.Item[i].Download);
  }

  // Show again
  m_List.Show();

  // Set title
  wstring title = L"Torrents";
  if (!Torrents.Feed.Title.empty()) title = Torrents.Feed.Title;
  if (!Torrents.Feed.Description.empty()) title += L" - " + Torrents.Feed.Description;
  SetText(title.c_str());
}

// =============================================================================

BOOL CTorrentWindow::OnCommand(WPARAM wParam, LPARAM lParam) {
  // Toolbar
  switch (LOWORD(wParam)) {
    // Check new torrents
    case 100: {
      Torrents.Check(Settings.RSS.Torrent.Source);
      /*
      #ifdef _DEBUG
      Torrents.Read();
      Torrents.Compare();
      RefreshList();
      #endif
      */
      return TRUE;
    }
    // Download selected torrents
    case 101: {
      for (int i = 0; i < m_List.GetItemCount(); i++) {
        CRSSFeedItem* pFeed = reinterpret_cast<CRSSFeedItem*>(m_List.GetItemParam(i));
        if (pFeed) {
          pFeed->Download = m_List.GetCheckState(i);
        }
      }
      Torrents.Download(-1);
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
  // ListView control
  if (idCtrl == IDC_LIST_TORRENT) {
    switch (pnmh->code) {
      // Column click
      case LVN_COLUMNCLICK: {
        LPNMLISTVIEW lplv = (LPNMLISTVIEW)pnmh;
        int order = m_List.GetSortOrder() * -1;
        if (order == 0) order = 1;
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
          CRSSFeedItem* pFeed = reinterpret_cast<CRSSFeedItem*>(m_List.GetItemParam(lpnmitem->iItem));
          if (pFeed) {
            Torrents.Download(pFeed->Index);
          }
        }
        break;
      }

      // Right click
      case NM_RCLICK: {
        LPNMITEMACTIVATE lpnmitem = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
        if (lpnmitem->iItem == -1) break;
        CRSSFeedItem* pFeed = reinterpret_cast<CRSSFeedItem*>(m_List.GetItemParam(lpnmitem->iItem));
        if (pFeed) {
          wstring answer = UI.Menus.Show(m_hWindow, 0, 0, L"TorrentListRightClick");
          if (answer == L"DownloadTorrent") {
            Torrents.Download(pFeed->Index);
          } else if (answer == L"MoreTorrents") {
            Torrents.Check(ReplaceVariables(
              L"http://www.nyaatorrents.org/?page=rss&catid=1&subcat=37&filter=2&term=%title%", 
              pFeed->EpisodeData));
          } else if (answer == L"SearchMAL") {
            ExecuteAction(L"SearchAnime(" + pFeed->EpisodeData.Title + L")");
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
            CRSSFeedItem* pFeed = reinterpret_cast<CRSSFeedItem*>(pCD->nmcd.lItemlParam);
            if (pFeed) {
              if (pFeed->EpisodeData.Index == -1) {
                pCD->clrText = RGB(180, 180, 180);
              } else if (pFeed->NewItem) {
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