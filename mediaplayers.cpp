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
#include "anime.h"
#include "common.h"
#include "media.h"
#include "process.h"
#include "recognition.h"
#include "settings.h"
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
                if (item->mode != MEDIA_MODE_WEBBROWSER || !GetWindowTitle(hwnd).empty()) {
                  // We have a match!
                  index = index_old = item - items.begin();
                  new_title = GetTitle(hwnd, *c, item->mode);
                  EditTitle(new_title, index);
                  if (current_title != new_title) title_changed_ = true;
                  current_title = new_title;
                  item->window_handle = hwnd;
                  return index;
                }
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

void MediaPlayers::EditTitle(wstring& str, int player_index) {
  if (str.empty() || items[player_index].edits.empty()) return;
  
  for (unsigned int i = 0; i < items[player_index].edits.size(); i++) {
    switch (items[player_index].edits[i].mode) {
      // Erase
      case 1: {
        Replace(str, items[player_index].edits[i].value, L"", false, true);
        break;
      }
      // Cut right side
      case 2: {
        int pos = InStr(str, items[player_index].edits[i].value, 0);
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
    case MEDIA_MODE_FILEHANDLE:
      return GetTitleFromProcessHandle(hwnd);
    // Winamp API
    case MEDIA_MODE_WINAMPAPI:
      return GetTitleFromWinampAPI(hwnd, false);
    // Special message
    case MEDIA_MODE_SPECIALMESSAGE:
      return GetTitleFromSpecialMessage(hwnd, class_name);
    // MPlayer
    case MEDIA_MODE_MPLAYER:
      return GetTitleFromMPlayer();
    // Browser
    case MEDIA_MODE_WEBBROWSER:
      return GetTitleFromBrowser(hwnd);
    // Window title
    case MEDIA_MODE_WINDOWTITLE:
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
            return new_title;
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

// =============================================================================

/* Streaming media recognition */

enum StreamingVideoProviders {
  STREAM_UNKNOWN = -1,
  STREAM_ANN,
  STREAM_CRUNCHYROLL,
  STREAM_HULU,
  STREAM_VEOH,
  STREAM_VIZANIME,
  STREAM_YOUTUBE
};

enum WebBrowsers {
  WEBBROWSER_UNKNOWN = -1,
  WEBBROWSER_CHROME,
  WEBBROWSER_FIREFOX,
  WEBBROWSER_IE,
  WEBBROWSER_OPERA
};

AccessibleChild* FindAccessibleChild(vector<AccessibleChild>& children, const wstring& name, const wstring& role) {
  AccessibleChild* child = nullptr;
  
  for (auto it = children.begin(); it != children.end(); ++it) {
    if (name.empty() || IsEqual(name, it->name)) {
      if (role.empty() || IsEqual(role, it->role)) {
        child = &(*it);
      }
    }
    if (child == nullptr && !it->children.empty()) {
      child = FindAccessibleChild(it->children, name, role);
    }
    if (child) {
      break;
    }
  }

  return child;
}

#ifdef _DEBUG
void BuildTreeString(vector<AccessibleChild>& children, wstring& str, int indent) {
  for (auto it = children.begin(); it != children.end(); ++it) {
    str.append(indent * 4, L' ');
    str += L"[" + it->role + L"] " + it->name + L" = " + it->value + L"\n";
    BuildTreeString(it->children, str, indent + 1);
  }
}
#endif

wstring MediaPlayers::GetTitleFromBrowser(HWND hwnd) {
  int stream_provider = STREAM_UNKNOWN;
  int web_browser = WEBBROWSER_UNKNOWN;

  // Get window title
  wstring title = GetWindowTitle(hwnd);
  EditTitle(title, index);

  // Return current title if the same web page is still open
  if (CurrentEpisode.anime_id > 0) {
    if (InStr(title, current_title) > -1) {
      return current_title;
    }
  }

  // Delay operation to save some CPU
  static int counter = 0;
  if (counter < 5) {
    counter++;
    return current_title;
  } else {
    counter = 0;
  }

  // Detect web browser
  if (InStr(items.at(index).name, L"Chrome") > -1) {
    web_browser = WEBBROWSER_CHROME;
  } else if (InStr(items.at(index).name, L"Firefox") > -1) {
    web_browser = WEBBROWSER_FIREFOX;
  /*
  } else if (InStr(items.at(index).name, L"Explorer") > -1) {
    web_browser = WEBBROWSER_IE;
  */
  } else if (InStr(items.at(index).name, L"Opera") > -1) {
    web_browser = WEBBROWSER_OPERA;
  } else {
    return L"";
  }

  // Build accessibility data
  acc_obj.children.clear();
  if (acc_obj.FromWindow(hwnd) == S_OK) {
    acc_obj.BuildChildren(acc_obj.children, nullptr, web_browser);
    acc_obj.Release();
  }

  // Check other tabs
  if (CurrentEpisode.anime_id > 0) {
    AccessibleChild* child = nullptr;
    switch (web_browser) {
      case WEBBROWSER_CHROME:
      case WEBBROWSER_FIREFOX:
        child = FindAccessibleChild(acc_obj.children, L"", L"page tab list");
        break;
      case WEBBROWSER_IE:
        child = FindAccessibleChild(acc_obj.children, L"Tab Row", L"");
        break;
      case WEBBROWSER_OPERA:
        child = FindAccessibleChild(acc_obj.children, L"", L"client");
        break;
    }
    if (child) {
      for (auto it = child->children.begin(); it != child->children.end(); ++it) {
        if (InStr(it->name, current_title) > -1) {
          // Tab is still open, just not active
          return current_title;
        }
      }
    }
    // Tab is closed
    return L"";
  }

  // Find URL
  AccessibleChild* child = nullptr;
  switch (web_browser) {
    case WEBBROWSER_CHROME:
      child = FindAccessibleChild(acc_obj.children, L"Address", L"grouping");
      if (child == nullptr) {
        child = FindAccessibleChild(acc_obj.children, L"Location", L"grouping");
      }
      break;
    case WEBBROWSER_FIREFOX:
      child = FindAccessibleChild(acc_obj.children, L"Go to a Website", L"editable text");
      if (child == nullptr) {
        child = FindAccessibleChild(acc_obj.children, L"Go to a Web Site", L"editable text");
      }
      break;
    case WEBBROWSER_IE:
      child = FindAccessibleChild(acc_obj.children, L"Address and search using Bing", L"editable text");
      if (child == nullptr) {
        child = FindAccessibleChild(acc_obj.children, L"Address and search using Google", L"editable text");
      }
      break;
    case WEBBROWSER_OPERA:
      child = FindAccessibleChild(acc_obj.children, L"", L"client");
      if (child && !child->children.empty()) {
        child = FindAccessibleChild(child->children.at(0).children, L"", L"tool bar");
        if (child && !child->children.empty()) {
          child = FindAccessibleChild(child->children, L"", L"combo box");
          if (child && !child->children.empty()) {
            child = FindAccessibleChild(child->children, L"", L"editable text");
          }
        }
      }
      break;
  }
  
  // Check URL for known streaming video providers
  if (child) {
    // Anime News Network
    if (Settings.Recognition.Streaming.ann_enabled && 
      InStr(child->value, L"animenewsnetwork.com/video") > -1) {
        stream_provider = STREAM_ANN;
    // Crunchyroll
    } else if (Settings.Recognition.Streaming.crunchyroll_enabled && 
      InStr(child->value, L"crunchyroll.com/") > -1) {
        stream_provider = STREAM_CRUNCHYROLL;
    // Hulu
    /*
    } else if (InStr(child->value, L"hulu.com/watch") > -1) {
      stream_provider = STREAM_HULU;
    */
    // Veoh
    } else if (Settings.Recognition.Streaming.veoh_enabled && 
      InStr(child->value, L"veoh.com/watch") > -1) {
        stream_provider = STREAM_VEOH;
    // Viz Anime
    } else if (Settings.Recognition.Streaming.viz_enabled && 
      InStr(child->value, L"vizanime.com/ep") > -1) {
        stream_provider = STREAM_VIZANIME;
    // YouTube
    } else if (Settings.Recognition.Streaming.youtube_enabled && 
      InStr(child->value, L"youtube.com/watch") > -1) {
        stream_provider = STREAM_YOUTUBE;
    }
  }

  // Clean-up title
  switch (stream_provider) {
    // Anime News Network
    case STREAM_ANN:
      EraseRight(title, L" - Anime News Network");
      break;
    // Crunchyroll
    case STREAM_CRUNCHYROLL:
      EraseLeft(title, L"Watch ");
      break;
    // Hulu
    case STREAM_HULU:
      EraseLeft(title, L"Hulu - ");
      EraseRight(title, L" - Watch the full episode now.");
      break;
    // Veoh
    case STREAM_VEOH:
      EraseLeft(title, L"Watch Videos Online | ");
      EraseRight(title, L" | Veoh.com");
      break;
    // Viz Anime
    case STREAM_VIZANIME:
      EraseRight(title, L" - VIZ ANIME: Free Online Anime - All The Time");
      break;
    // YouTube
    case STREAM_YOUTUBE:
      EraseRight(title, L" - YouTube");
      break;
    // Some other website, or URL is not found
    case STREAM_UNKNOWN:
      title.clear();
      break;
  }

  return title;
}

bool MediaPlayers::BrowserAccessibleObject::AllowChildTraverse(AccessibleChild& child, LPARAM param) {
  switch (param) {
    case WEBBROWSER_UNKNOWN:
      return false;
    case WEBBROWSER_FIREFOX:
      if (IsEqual(child.role, L"document")) return false;
      break;
    case WEBBROWSER_IE:
      if (IsEqual(child.role, L"pane") || IsEqual(child.role, L"scroll bar")) return false;
      break;
    case WEBBROWSER_OPERA:
      if (IsEqual(child.role, L"document") || IsEqual(child.role, L"pane")) return false;
      break;
  }

  return true;
}