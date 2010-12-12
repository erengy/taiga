/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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
#include "dlg_test_recognition.h"
#include "../recognition.h"
#include "../resource.h"
#include "../string.h"
#include "../taiga.h"
#include "../xml.h"

CTestRecognition RecognitionTestWindow;

// =============================================================================

BOOL CTestRecognition::OnInitDialog() {
  // Initialize
  wstring file = Taiga.GetDataPath() + L"RecognitionTest.xml";
  m_EpisodeList.clear();
  m_EpisodeListTest.clear();
  m_List.DeleteAllItems();
  
  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok) {
    ::MessageBox(NULL, L"Could not read recognition test file.", file.c_str(), MB_OK | MB_ICONERROR);
    return FALSE;
  }

  // Fill episode data
  xml_node recognition = doc.child(L"recognition");
  for (xml_node file_node = recognition.child(L"file"); file_node; file_node = file_node.next_sibling(L"file")) {
    CEpisode new_episode;
    new_episode.AudioType  = XML_ReadStrValue(file_node, L"audio");
    new_episode.Checksum   = XML_ReadStrValue(file_node, L"checksum");
    new_episode.Extra      = XML_ReadStrValue(file_node, L"extra");
    new_episode.File       = XML_ReadStrValue(file_node, L"file");
    new_episode.Format     = XML_ReadStrValue(file_node, L"format");
    new_episode.Group      = XML_ReadStrValue(file_node, L"group");
    new_episode.Name       = XML_ReadStrValue(file_node, L"name");
    new_episode.Number     = XML_ReadStrValue(file_node, L"number");
    new_episode.Resolution = XML_ReadStrValue(file_node, L"resolution");
    new_episode.Title      = XML_ReadStrValue(file_node, L"title");
    new_episode.Version    = XML_ReadStrValue(file_node, L"version");
    new_episode.VideoType  = XML_ReadStrValue(file_node, L"video");
    m_EpisodeList.push_back(new_episode);
  }

  // Examine files
  DWORD tick = GetTickCount();
  for (UINT i = 0; i < m_EpisodeList.size(); i++) {
    CEpisode episode;
    Meow.ExamineTitle(m_EpisodeList[i].File, episode, true, true, true, true, false);
    m_EpisodeListTest.push_back(episode);
  }
  tick = GetTickCount() - tick;

  // Create list
  m_List.Attach(GetDlgItem(IDC_LIST_TEST_RECOGNITION));
  m_List.InsertColumn(0, 400, 400, 0, L"File name");
  m_List.InsertColumn(1, 200, 200, 0, L"Title");
  m_List.InsertColumn(2, 100, 100, 0, L"Group");
  m_List.InsertColumn(3, 55, 55, 0, L"Episode");
  m_List.InsertColumn(4, 50, 50, 0, L"Version");
  m_List.InsertColumn(5, 70, 70, 0, L"Audio");
  m_List.InsertColumn(6, 70, 70, 0, L"Video");
  m_List.InsertColumn(7, 80, 80, 0, L"Resolution");
  m_List.InsertColumn(8, 80, 80, 0, L"Checksum");
  m_List.InsertColumn(9, 100, 100, 0, L"Extra");
  m_List.InsertColumn(10, 100, 100, 0, L"Name");
  m_List.InsertColumn(11, 50, 50, 0, L"Format");

  // Fill list
  for (UINT i = 0; i < m_EpisodeList.size(); i++) {
    m_List.InsertItem(i, -1, -1, m_EpisodeListTest[i].File.c_str(), 0);
    m_List.SetItem(i, 1, m_EpisodeListTest[i].Title.c_str());
    m_List.SetItem(i, 2, m_EpisodeListTest[i].Group.c_str());
    m_List.SetItem(i, 3, m_EpisodeListTest[i].Number.c_str());
    m_List.SetItem(i, 4, m_EpisodeListTest[i].Version.c_str());
    m_List.SetItem(i, 5, m_EpisodeListTest[i].AudioType.c_str());
    m_List.SetItem(i, 6, m_EpisodeListTest[i].VideoType.c_str());
    m_List.SetItem(i, 7, m_EpisodeListTest[i].Resolution.c_str());
    m_List.SetItem(i, 8, m_EpisodeListTest[i].Checksum.c_str());
    m_List.SetItem(i, 9, m_EpisodeListTest[i].Extra.c_str());
    m_List.SetItem(i, 10, m_EpisodeListTest[i].Name.c_str());
    m_List.SetItem(i, 11, m_EpisodeListTest[i].Format.c_str());
  }

  // Set title
  int success_count = 0;
  for (UINT i = 0; i < m_EpisodeList.size(); i++) {
    if (m_EpisodeList[i].Title == m_EpisodeListTest[i].Title && 
      m_EpisodeList[i].Number == m_EpisodeListTest[i].Number) {
        success_count++;
    }
  }
  wstring title = L"Taiga Recognition Test";
  title += L" - Item count: " + ToWSTR(static_cast<int>(m_EpisodeList.size()));
  title += L" - Success rate: %" + ToWSTR(((float)success_count / (float)m_EpisodeList.size()) * 100.0f, 2);
  title += L" - Time: " + ToWSTR(tick) + L" ms";
  SetText(title.c_str());
  
  // Success
  return TRUE;
}

// =============================================================================

#define CheckSubItem(i, t) \
  m_EpisodeList[i].t == m_EpisodeListTest[i].t ? RGB(230, 255, 230) : \
  m_EpisodeListTest[i].t.empty() ? RGB(245, 255, 245) : RGB(255, 230, 230)

LRESULT CTestRecognition::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // ListView control
  if (idCtrl == IDC_LIST_TEST_RECOGNITION) {
    switch (pnmh->code) {
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
            int i = static_cast<int>(pCD->nmcd.dwItemSpec);
            switch (pCD->iSubItem) {
              // Title
              case 1: pCD->clrTextBk = CheckSubItem(i, Title); break;
              // Group
              case 2: pCD->clrTextBk = CheckSubItem(i, Group); break;
              // Episode
              case 3: pCD->clrTextBk = CheckSubItem(i, Number); break;
              // Version
              case 4: pCD->clrTextBk = CheckSubItem(i, Version); break;
              // Audio
              case 5: pCD->clrTextBk = CheckSubItem(i, AudioType); break;
              // Video
              case 6: pCD->clrTextBk = CheckSubItem(i, VideoType); break;
              // Resolution
              case 7: pCD->clrTextBk = CheckSubItem(i, Resolution); break;
              // Checksum
              case 8: pCD->clrTextBk = CheckSubItem(i, Checksum); break;
              // Extra
              case 9: pCD->clrTextBk = CheckSubItem(i, Extra); break;
              // Name
              case 10: pCD->clrTextBk = CheckSubItem(i, Name); break;
              // Format
              case 11: pCD->clrTextBk = CheckSubItem(i, Format); break;
              // Default
              default: pCD->clrTextBk = GetSysColor(COLOR_WINDOW);
            }
            return CDRF_NOTIFYPOSTPAINT;
          }
        }
      }
    }
  }

  return 0;
}

void CTestRecognition::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      m_List.SetPosition(NULL, 0, 0, size.cx, size.cy);
    }
  }
}