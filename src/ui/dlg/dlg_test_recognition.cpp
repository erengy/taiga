/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include "dlg_test_recognition.h"

#include "base/common.h"
#include "track/recognition.h"
#include "taiga/resource.h"
#include "base/string.h"
#include "taiga/path.h"
#include "taiga/taiga.h"
#include "base/xml.h"
#include "ui/list.h"

RecognitionTestDialog RecognitionTest;

// =============================================================================

BOOL RecognitionTestDialog::OnInitDialog() {
  episodes_.clear();
  test_episodes_.clear();
  list_.DeleteAllItems();

  xml_document document;
  wstring path = taiga::GetPath(taiga::kPathTestRecognition);
  xml_parse_result parse_result = document.load_file(path.c_str());

  if (parse_result.status != pugi::status_ok) {
    ::MessageBox(nullptr, L"Could not read recognition test file.", path.c_str(),
                 MB_OK | MB_ICONERROR);
    return FALSE;
  }

  // Fill episode data
  xml_node recognition = document.child(L"recognition");
  for (xml_node file_node = recognition.child(L"file"); file_node; file_node = file_node.next_sibling(L"file")) {
    EpisodeTest new_episode;
    new_episode.audio_type = XmlReadStrValue(file_node, L"audio");
    new_episode.checksum   = XmlReadStrValue(file_node, L"checksum");
    new_episode.extras     = XmlReadStrValue(file_node, L"extra");
    new_episode.file       = XmlReadStrValue(file_node, L"file");
    new_episode.format     = XmlReadStrValue(file_node, L"format");
    new_episode.group      = XmlReadStrValue(file_node, L"group");
    new_episode.name       = XmlReadStrValue(file_node, L"name");
    new_episode.number     = XmlReadStrValue(file_node, L"number");
    new_episode.priority   = XmlReadIntValue(file_node, L"priority");
    new_episode.resolution = XmlReadStrValue(file_node, L"resolution");
    new_episode.title      = XmlReadStrValue(file_node, L"title");
    new_episode.version    = XmlReadStrValue(file_node, L"version");
    new_episode.video_type = XmlReadStrValue(file_node, L"video");
    episodes_.push_back(new_episode);
  }

  // Examine files
  DWORD tick = GetTickCount();
  for (UINT i = 0; i < episodes_.size(); i++) {
    EpisodeTest episode;
    Meow.ExamineTitle(episodes_[i].file, episode, true, true, true, true, false);
    episode.anime_id = i;
    episode.priority = episodes_[i].priority;
    test_episodes_.push_back(episode);
  }
  tick = GetTickCount() - tick;

  // Create list
  list_.Attach(GetDlgItem(IDC_LIST_TEST_RECOGNITION));
  list_.InsertColumn(0, 400, 400, 0, L"File name");
  list_.InsertColumn(1, 200, 200, 0, L"Title");
  list_.InsertColumn(2, 100, 100, 0, L"Group");
  list_.InsertColumn(3, 55, 55, 0, L"Episode");
  list_.InsertColumn(4, 50, 50, 0, L"Version");
  list_.InsertColumn(5, 70, 70, 0, L"Audio");
  list_.InsertColumn(6, 70, 70, 0, L"Video");
  list_.InsertColumn(7, 80, 80, 0, L"Resolution");
  list_.InsertColumn(8, 80, 80, 0, L"Checksum");
  list_.InsertColumn(9, 100, 100, 0, L"Extra");
  list_.InsertColumn(10, 100, 100, 0, L"Name");
  list_.InsertColumn(11, 50, 50, 0, L"Format");

  // Fill list
  for (UINT i = 0; i < episodes_.size(); i++) {
    list_.InsertItem(i, -1, -1, 0, NULL, test_episodes_[i].file.c_str(), 
      reinterpret_cast<LPARAM>(&test_episodes_[i]));
    list_.SetItem(i, 1, test_episodes_[i].title.c_str());
    list_.SetItem(i, 2, test_episodes_[i].group.c_str());
    list_.SetItem(i, 3, test_episodes_[i].number.c_str());
    list_.SetItem(i, 4, test_episodes_[i].version.c_str());
    list_.SetItem(i, 5, test_episodes_[i].audio_type.c_str());
    list_.SetItem(i, 6, test_episodes_[i].video_type.c_str());
    list_.SetItem(i, 7, test_episodes_[i].resolution.c_str());
    list_.SetItem(i, 8, test_episodes_[i].checksum.c_str());
    list_.SetItem(i, 9, test_episodes_[i].extras.c_str());
    list_.SetItem(i, 10, test_episodes_[i].name.c_str());
    list_.SetItem(i, 11, test_episodes_[i].format.c_str());
  }
  list_.Sort(1, 1, ui::kListSortDefault, ui::ListViewCompareProc);

  // Set title
  int success_count = 0, total_items = episodes_.size();
  for (int i = 0; i < total_items; i++) {
    if (episodes_[i].title == test_episodes_[i].title && 
      episodes_[i].number == test_episodes_[i].number) {
        success_count++;
    }
  }
  wstring title = L"Taiga Recognition Test";
  title += L" - Success rate: " + ToWstr(success_count) + L"/" + ToWstr(total_items);
  title += L" (" + ToWstr(((float)success_count / (float)total_items) * 100.0f, 2) + L"%)";
  title += L" - Time: " + ToWstr(tick) + L" ms";
  SetText(title.c_str());
  
  // Success
  return TRUE;
}

// =============================================================================

LRESULT RecognitionTestDialog::OnNotify(int idCtrl, LPNMHDR pnmh) {
  // ListView control
  if (idCtrl == IDC_LIST_TEST_RECOGNITION) {
    switch (pnmh->code) {
      // Column click
      case LVN_COLUMNCLICK: {
        LPNMLISTVIEW lplv = reinterpret_cast<LPNMLISTVIEW>(pnmh);
        int order = 1;
        if (lplv->iSubItem == list_.GetSortColumn()) order = list_.GetSortOrder() * -1;
        int type = ui::kListSortDefault;
        switch (lplv->iSubItem) {
          case 3:
          case 4:
            type = ui::kListSortNumber;
            break;
        }
        list_.Sort(lplv->iSubItem, order, type, ui::ListViewCompareProc);
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
            EpisodeTest* e = reinterpret_cast<EpisodeTest*>(pCD->nmcd.lItemlParam);
            if (!e) return CDRF_NOTIFYPOSTPAINT;
            #define CheckSubItem(e, t) \
              e->t == episodes_[e->anime_id].t ? RGB(230, 255, 230) : \
              e->t.empty() ? RGB(245, 255, 245) : RGB(255, 230, 230)
            switch (pCD->iSubItem) {
              // Title
              case 1: pCD->clrTextBk = CheckSubItem(e, title); break;
              // Group
              case 2: pCD->clrTextBk = CheckSubItem(e, group); break;
              // Episode
              case 3: pCD->clrTextBk = CheckSubItem(e, number); break;
              // Version
              case 4: pCD->clrTextBk = CheckSubItem(e, version); break;
              // Audio
              case 5: pCD->clrTextBk = CheckSubItem(e, audio_type); break;
              // Video
              case 6: pCD->clrTextBk = CheckSubItem(e, video_type); break;
              // Resolution
              case 7: pCD->clrTextBk = CheckSubItem(e, resolution); break;
              // Checksum
              case 8: pCD->clrTextBk = CheckSubItem(e, checksum); break;
              // Extra
              case 9: pCD->clrTextBk = CheckSubItem(e, extras); break;
              // Name
              case 10: pCD->clrTextBk = CheckSubItem(e, name); break;
              // Format
              case 11: pCD->clrTextBk = CheckSubItem(e, format); break;
              // Default
              default: pCD->clrTextBk = GetSysColor(COLOR_WINDOW);
            }
            if (e->priority < 0) {
              pCD->clrText = RGB(200, 200, 200);
            } else if (e->priority > 0) {
              pCD->clrText = RGB(255, 0, 0);
            }
            #undef CheckSubItem
            return CDRF_NOTIFYPOSTPAINT;
          }
        }
      }
    }
  }

  return 0;
}

void RecognitionTestDialog::OnSize(UINT uMsg, UINT nType, SIZE size) {
  switch (uMsg) {
    case WM_SIZE: {
      list_.SetPosition(NULL, 0, 0, size.cx, size.cy);
    }
  }
}