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

#include "std.h"
#include <tlhelp32.h>
#include "common.h"
#include "media.h"
#include "process.h"
#include "recognition.h"
#include "string.h"
#include "taiga.h"
#include "xml.h"

class MediaPlayers MediaPlayers;

// =============================================================================

MediaPlayers::MediaPlayers() : 
  index(-1), index_old(-1), title_changed_(false)
{
}

BOOL MediaPlayers::Load() {
  // Initialize
  wstring file = Taiga.GetDataPath() + L"Media.xml";
  items.clear();
  index = -1;
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file.c_str());
  if (result.status != status_ok) {
    MessageBox(NULL, L"Could not read media list.", file.c_str(), MB_OK | MB_ICONERROR);
    return FALSE;
  }

  // Read player list
  xml_node mediaplayers = doc.child(L"media_players");
  for (xml_node player = mediaplayers.child(L"player"); player; player = player.next_sibling(L"player")) {
    items.resize(items.size() + 1);
    items.back().name = XML_ReadStrValue(player, L"name");
    items.back().enabled = XML_ReadIntValue(player, L"enabled");
    items.back().visible = XML_ReadIntValue(player, L"visible");
    items.back().mode = XML_ReadIntValue(player, L"mode");
    XML_ReadChildNodes(player, items.back().classes, L"class");
    XML_ReadChildNodes(player, items.back().files, L"file");
    XML_ReadChildNodes(player, items.back().folders, L"folder");
    for (xml_node child_node = player.child(L"edit"); child_node; child_node = child_node.next_sibling(L"edit")) {
      MediaPlayer::EditTitle edit;
      edit.mode = child_node.attribute(L"mode").as_int();
      edit.value = child_node.child_value();
      items.back().edits.push_back(edit);
    }
  }
  
  return TRUE;
}

BOOL MediaPlayers::Save() {
  // Initialize
  wstring folder = Taiga.GetDataPath();
  wstring file = folder + L"Media.xml";
  
  // Create XML document
  xml_document doc;
  xml_node players = doc.append_child();
  players.set_name(L"media_players");

  // Write player list
  for (unsigned int i = 0; i < items.size(); i++) {
    wstring comment = L" " + items[i].name + L" ";
    players.append_child(node_comment).set_value(comment.c_str());
    xml_node player = players.append_child();
    player.set_name(L"player");
    player.append_child().set_name(L"name");
    player.child(L"name").append_child(node_pcdata).set_value(items[i].name.c_str());
    player.append_child().set_name(L"enabled");
    player.child(L"enabled").append_child(node_pcdata).set_value(ToWSTR(items[i].enabled).c_str());
    player.append_child().set_name(L"visible");
    player.child(L"visible").append_child(node_pcdata).set_value(ToWSTR(items[i].visible).c_str());
    player.append_child().set_name(L"mode");
    player.child(L"mode").append_child(node_pcdata).set_value(ToWSTR(items[i].mode).c_str());
    XML_WriteChildNodes(player, items[i].classes, L"class");
    XML_WriteChildNodes(player, items[i].files, L"file");
    XML_WriteChildNodes(player, items[i].folders, L"folder");
    for (unsigned int j = 0; j < items[i].edits.size(); j++) {
      xml_node edit = player.append_child();
      edit.set_name(L"edit");
      edit.append_attribute(L"mode") = items[i].edits[j].mode;
      edit.append_child(node_pcdata).set_value(items[i].edits[j].value.c_str());
    }
  }

  // Save file
  CreateDirectory(folder.c_str(), NULL);
  return doc.save_file(file.c_str(), L"\x09", format_default | format_write_bom);
  return TRUE;
}

// =============================================================================

int MediaPlayers::Check() {
  title_changed_ = false;
  index = -1;

  HWND hwnd = GetWindow(g_hMain, GW_HWNDFIRST);
  while (hwnd != NULL) {
    for (auto item = items.begin(); item != items.end(); ++item) {
      if (item->enabled == FALSE) continue;
      if (item->visible == FALSE || IsWindowVisible(hwnd)) {
        // Compare window classes
        for (auto c = item->classes.begin(); c != item->classes.end(); ++c) {
          if (*c == GetWindowClass(hwnd)) {
            // Compare file names
            for (auto f = item->files.begin(); f != item->files.end(); ++f) {
              if (IsEqual(*f, GetFileName(GetWindowPath(hwnd)))) {
                // We have a match!
                new_caption = GetTitle(hwnd, *c, item->mode);
                if (current_caption != new_caption) title_changed_ = true;
                current_caption = new_caption;
                item->window_handle = hwnd;
                index = index_old = item - items.begin();
                return index;
              }
            }
          }
        }
      }
    }
    // Check next window
    hwnd = GetWindow(hwnd, GW_HWNDNEXT);
  }

  // Not found
  return -1;
}

void MediaPlayers::EditTitle(wstring& str) {
  if (str.empty() || index == -1 || items[index].edits.empty()) return;
  
  for (unsigned int i = 0; i < items[index].edits.size(); i++) {
    switch (items[index].edits[i].mode) {
      // Erase
      case 1: {
        Replace(str, items[index].edits[i].value, L"", false, true);
        break;
      }
      // Cut right side
      case 2: {
        int pos = InStr(str, items[index].edits[i].value, 0);
        if (pos > -1) str.resize(pos);
        break;
      }
    }
  }

  TrimRight(str, L" -");
}

wstring MediaPlayers::MediaPlayer::GetPath() {
  for (size_t i = 0; i < folders.size(); i++) {
    for (size_t j = 0; j < files.size(); j++) {
      wstring path = folders[i] + files[j];
      path = ExpandEnvironmentStrings(path);
      if (FileExists(path)) return path;
    }
  }
  return L"";
}

wstring MediaPlayers::GetTitle(HWND hwnd, const wstring& class_name, int mode) {
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
    // MPlayer
    case 4:
      return GetTitleFromMPlayer();
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

wstring MediaPlayers::GetTitleFromProcessHandle(HWND hwnd, ULONG process_id) {
  vector<wstring> files_vector;
  if (hwnd != NULL && process_id == 0) {
    GetWindowThreadProcessId(hwnd, &process_id);
  }
  if (GetProcessFiles(process_id, files_vector)) {
    for (unsigned int i = 0; i < files_vector.size(); i++) {
      if (CheckFileExtension(GetFileExtension(files_vector[i]), Meow.valid_extensions)) {
        if (files_vector[i].at(1) != L':') {
          TranslateDeviceName(files_vector[i]);
        }
        if (files_vector[i].at(1) == L':') {
          WCHAR buffer[4096] = {0};
          GetLongPathName(files_vector[i].c_str(), buffer, 4096);
          return GetFileName(buffer);
        } else {
          return GetFileName(files_vector[i]);
        }
      }
    }
  }
  return L"";
}

wstring MediaPlayers::GetTitleFromWinampAPI(HWND hwnd, bool use_unicode) {
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

wstring MediaPlayers::GetTitleFromSpecialMessage(HWND hwnd, const wstring& class_name) {
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
            return new_caption;
        }
      }
    }
  }

  return L"";
}

wstring MediaPlayers::GetTitleFromMPlayer() {
  HANDLE hProcessSnap;
  PROCESSENTRY32 pe32;
  wstring title;

  hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hProcessSnap != INVALID_HANDLE_VALUE) {
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hProcessSnap, &pe32)) {
      do {
        if (IsEqual(pe32.szExeFile, L"mplayer.exe")) {
          title = GetTitleFromProcessHandle(NULL, pe32.th32ProcessID);
          break;
        }
      } while (Process32Next(hProcessSnap, &pe32));
    }
    CloseHandle(hProcessSnap);
  }
  
  return title;
}