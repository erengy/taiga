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

#include "std.h"
#include "common.h"
#include "media.h"
#include "process.h"
#include "recognition.h"
#include "string.h"
#include "taiga.h"
#include "xml.h"

CMediaPlayers MediaPlayers;

// =============================================================================

BOOL CMediaPlayers::Read() {
  // Initialize
  wstring file = Taiga.GetDataPath() + L"Media.xml";
  Index = -1;
  
  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(ToANSI(file));
  if (result.status != status_ok) {
    MessageBox(NULL, L"Could not read media list.", file.c_str(), MB_OK | MB_ICONERROR);
    return FALSE;
  }

  // Read player list
  xml_node mediaplayers = doc.child(L"media_players");
  for (xml_node player = mediaplayers.child(L"player"); player; player = player.next_sibling(L"player")) {
    Item.resize(Item.size() + 1);
    Item.back().Name = XML_ReadStrValue(player, L"name");
    Item.back().Enabled = XML_ReadIntValue(player, L"enabled");
    Item.back().Visible = XML_ReadIntValue(player, L"visible");
    Item.back().Mode = XML_ReadIntValue(player, L"mode");
    XML_ReadChildNodes(player, Item.back().Class, L"class");
    XML_ReadChildNodes(player, Item.back().File, L"file");
    XML_ReadChildNodes(player, Item.back().Folder, L"folder");
    for (xml_node child_node = player.child(L"edit"); child_node; child_node = child_node.next_sibling(L"edit")) {
      CMediaPlayer::CEditTitle edit;
      edit.Mode = child_node.attribute(L"mode").as_int();
      edit.Value = child_node.child_value();
      Item.back().Edit.push_back(edit);
    }
  }
  
  return TRUE;
}

BOOL CMediaPlayers::Write() {
  // Initialize
  wstring folder = Taiga.GetDataPath();
  wstring file = folder + L"Media.xml";
  
  // Create XML document
  xml_document doc;
  xml_node players = doc.append_child();
  players.set_name(L"media_players");

  // Write player list
  for (unsigned int i = 0; i < Item.size(); i++) {
    wstring comment = L" " + Item[i].Name + L" ";
    players.append_child(node_comment).set_value(comment.c_str());
    xml_node player = players.append_child();
    player.set_name(L"player");
    player.append_child().set_name(L"name");
    player.child(L"name").append_child(node_pcdata).set_value(Item[i].Name.c_str());
    player.append_child().set_name(L"enabled");
    player.child(L"enabled").append_child(node_pcdata).set_value(ToWSTR(Item[i].Enabled).c_str());
    player.append_child().set_name(L"visible");
    player.child(L"visible").append_child(node_pcdata).set_value(ToWSTR(Item[i].Visible).c_str());
    player.append_child().set_name(L"mode");
    player.child(L"mode").append_child(node_pcdata).set_value(ToWSTR(Item[i].Mode).c_str());
    XML_WriteChildNodes(player, Item[i].Class, L"class");
    XML_WriteChildNodes(player, Item[i].File, L"file");
    XML_WriteChildNodes(player, Item[i].Folder, L"folder");
    for (unsigned int j = 0; j < Item[i].Edit.size(); j++) {
      xml_node edit = player.append_child();
      edit.set_name(L"edit");
      edit.append_attribute(L"mode") = Item[i].Edit[j].Mode;
      edit.append_child(node_pcdata).set_value(Item[i].Edit[j].Value.c_str());
    }
  }

  // Save file
  CreateDirectory(folder.c_str(), NULL);
  return doc.save_file(ToANSI(file), L"\x09", format_default | format_write_bom_utf8);
  return TRUE;
}

// =============================================================================

int CMediaPlayers::Check() {
  m_bTitleChanged = false;
  Index = -1;
  
  HWND hwnd = GetWindow(g_hMain, 0);
  while (hwnd != NULL) {
    for (UINT i = 0; i < Item.size(); i++) {
      if (Item[i].Enabled == FALSE) continue;
      if (Item[i].Visible == FALSE || IsWindowVisible(hwnd)) {
        for (UINT c = 0; c < Item[i].Class.size(); c++) {
          if (Item[i].Class[c] == GetWindowClass(hwnd)) {
            for (UINT f = 0; f < Item[i].File.size(); f++) {
              if (Item[i].File[f] == GetFileName(GetWindowPath(hwnd))) {
                NewCaption = GetTitle(hwnd, Item[i].Class[c], Item[i].Mode);
                if (CurrentCaption != NewCaption) m_bTitleChanged = true;
                CurrentCaption = NewCaption;
                Item[i].WindowHandle = hwnd;
                Index = IndexOld = i;
                return i;
              }
            }
          }
        }
      }
    }
    hwnd = GetWindow(hwnd, GW_HWNDNEXT);
  }

  return -1;
}

void CMediaPlayers::EditTitle(wstring& str) {
  if (str.empty() || Index == -1 || Item[Index].Edit.empty()) return;
  
  for (unsigned int i = 0; i < Item[Index].Edit.size(); i++) {
    switch (Item[Index].Edit[i].Mode) {
      // Erase
      case 1: {
        Replace(str, Item[Index].Edit[i].Value, L"", false, true);
        break;
      }
      // Cut right side
      case 2: {
        int pos = InStr(str, Item[Index].Edit[i].Value, 0);
        if (pos > -1) str.resize(pos);
        break;
      }
    }
  }

  TrimRight(str, L" -");
}

wstring CMediaPlayers::CMediaPlayer::GetPath() {
  for (size_t i = 0; i < Folder.size(); i++) {
    for (size_t j = 0; j < File.size(); j++) {
      wstring path = Folder[i] + File[j];
      path = ExpandEnvironmentStrings(path);
      if (FileExists(path)) return path;
    }
  }
  return L"";
}

wstring CMediaPlayers::GetTitle(HWND hwnd, const wstring& class_name, int mode) {
  switch (mode) {
    // File handle
    case 1:
      return GetTitleFromProcessHandle(hwnd);
    // Winamp API
    case 2:
      return GetTitleFromWinampAPI(hwnd, false);
    // Special message
    case 3:
      return GetTitleFromSpecialMessage(hwnd, class_name);
    // Window title
    default:
      return GetWindowTitle(hwnd);
  }
}

// =============================================================================

// BS.Player
#define BSP_CLASS L"BSPlayer"
#define BSP_GETFILENAME 0x1010B

// JetAudio
#define JETAUDIO_REMOTECLASS L"COWON Jet-Audio Remocon Class"
#define JETAUDIO_MAINCLASS   L"COWON Jet-Audio MainWnd Class"
#define JETAUDIO_WINDOW      L"Jet-Audio Remote Control"
#define WM_REMOCON_GETSTATUS      WM_APP + 740
#define GET_STATUS_STATUS         1
#define GET_STATUS_TRACK_FILENAME 11

// Winamp
#define IPC_ISPLAYING        104
#define IPC_GETLISTPOS       125
#define IPC_GETPLAYLISTFILE  211
#define IPC_GETPLAYLISTFILEW 214

wstring CMediaPlayers::GetTitleFromProcessHandle(HWND hwnd) {
  if (IsWindow(hwnd)) {
    vector<wstring> files_vector;
    if (GetProcessFiles(hwnd, files_vector)) {
      for (unsigned int i = 0; i < files_vector.size(); i++) {
        if (CheckFileExtension(GetFileExtension(files_vector[i]), Meow.ValidExtensions)) {
          if (files_vector[i].at(1) != ':') {
            TranslateDeviceName(files_vector[i]);
          }
          if (files_vector[i].at(1) != ':') {
            return GetFileName(files_vector[i]);
          } else {
            return files_vector[i];
          }
        }
      }
    }
  }
  return L"";
}

wstring CMediaPlayers::GetTitleFromWinampAPI(HWND hwnd, bool use_unicode) {
  if (IsWindow(hwnd)) {
    if (SendMessage(hwnd, WM_USER, 0, IPC_ISPLAYING)) {
      int list_index = SendMessage(hwnd, WM_USER, 0, IPC_GETLISTPOS);
      int base_address = SendMessage(hwnd, WM_USER, list_index, 
        use_unicode ? IPC_GETPLAYLISTFILEW : IPC_GETPLAYLISTFILE);
      if (base_address) {
        DWORD process_id; GetWindowThreadProcessId(hwnd, &process_id);
        HANDLE hwnd_winamp = OpenProcess(PROCESS_VM_READ, FALSE, process_id);
        if (hwnd_winamp) {
          if (use_unicode) {
            wchar_t file_name[MAX_PATH];
            ReadProcessMemory(hwnd_winamp, reinterpret_cast<LPCVOID>(base_address), 
              file_name, MAX_PATH, NULL);
            CloseHandle(hwnd_winamp);
            return file_name;
          } else {
            char file_name[MAX_PATH];
            ReadProcessMemory(hwnd_winamp, reinterpret_cast<LPCVOID>(base_address), 
              file_name, MAX_PATH, NULL);
            CloseHandle(hwnd_winamp);
            return ToUTF8(file_name);
          }
        }
      }
    }
  }
  return L"";
}

wstring CMediaPlayers::GetTitleFromSpecialMessage(HWND hwnd, const wstring& class_name) {
  // BS.Player
  if (class_name == BSP_CLASS) {
    if (IsWindow(hwnd)) {
      COPYDATASTRUCT cds;
      char file_name[MAX_PATH];
      void* data = &file_name;
      cds.dwData = BSP_GETFILENAME;
      cds.lpData = &data;
      cds.cbData = 4;
      SendMessage(hwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(g_hMain), 
        reinterpret_cast<LPARAM>(&cds));
      return ToUTF8(file_name);
    }
  
  // JetAudio  
  } else if (class_name == JETAUDIO_MAINCLASS) {
    HWND hwnd_remote = FindWindow(JETAUDIO_REMOTECLASS, JETAUDIO_WINDOW);
    if (IsWindow(hwnd_remote))  {
      if (SendMessage(hwnd_remote, WM_REMOCON_GETSTATUS, 0, GET_STATUS_STATUS) != MCI_MODE_STOP) {
        if (SendMessage(hwnd_remote, WM_REMOCON_GETSTATUS, 
          reinterpret_cast<WPARAM>(g_hMain), GET_STATUS_TRACK_FILENAME)) {
            return MediaPlayers.NewCaption;
        }
      }
    }
  }

  return L"";
}