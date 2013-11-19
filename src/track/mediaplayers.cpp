/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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
#include <tlhelp32.h>

#include "media.h"

#include "library/anime.h"
#include "base/common.h"
#include "base/process.h"
#include "recognition.h"
#include "taiga/settings.h"
#include "base/string.h"
#include "taiga/taiga.h"
#include "base/xml.h"

class MediaPlayers MediaPlayers;

// =============================================================================

MediaPlayers::MediaPlayers()
    : index(-1), 
      index_old(-1), 
      title_changed_(false) {
}

BOOL MediaPlayers::Load() {
  // Initialize
  wstring file = Taiga.GetDataPath() + L"media.xml";
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
    items.back().engine = XML_ReadStrValue(player, L"engine");
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

// =============================================================================

int MediaPlayers::Check() {
  index = -1;
  bool recognized = CurrentEpisode.anime_id > anime::ID_UNKNOWN;

  // Go through windows, starting with the highest in the Z order
  HWND hwnd = GetWindow(g_hMain, GW_HWNDFIRST);
  while (hwnd != nullptr) {
    for (auto item = items.begin(); item != items.end(); ++item) {
      if (!item->enabled)
        continue;
      if (item->visible && !IsWindowVisible(hwnd))
        continue;
      // Compare window classes
      for (auto c = item->classes.begin(); c != item->classes.end(); ++c) {
        if (*c == GetWindowClass(hwnd)) {
          // Compare file names
          for (auto f = item->files.begin(); f != item->files.end(); ++f) {
            if (IsEqual(*f, GetFileName(GetWindowPath(hwnd)))) {
              if (item->mode != MEDIA_MODE_WEBBROWSER || !GetWindowTitle(hwnd).empty()) {
                // Stick with the previously recognized window, if there is one
                if (!recognized || item->window_handle == hwnd) {
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

bool MediaPlayers::TitleChanged() const {
  return title_changed_;
}

void MediaPlayers::SetTitleChanged(bool title_changed) {
  title_changed_ = title_changed;
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
          return wstring(buffer);
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

enum WebBrowserEngines {
  WEBENGINE_UNKNOWN = -1,
  // Google Chrome (and other browsers based on Chromium)
  WEBENGINE_WEBKIT,
  // Mozilla Firefox
  WEBENGINE_GECKO,
  // Internet Explorer
  WEBENGINE_TRIDENT,
  // Opera (older versions)
  WEBENGINE_PRESTO
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
  int web_engine = WEBENGINE_UNKNOWN;

  // Get window title
  wstring title = GetWindowTitle(hwnd);
  EditTitle(title, index);

  // Return current title if the same web page is still open
  if (CurrentEpisode.anime_id > 0)
    if (InStr(title, current_title) > -1)
      return current_title;

  // Delay operation to save some CPU
  static int counter = 0;
  if (counter < 5) {
    counter++;
    return current_title;
  } else {
    counter = 0;
  }

  // Select web browser engine
  if (items.at(index).engine == L"WebKit") {
    web_engine = WEBENGINE_WEBKIT;
  } else if (items.at(index).engine == L"Gecko") {
    web_engine = WEBENGINE_GECKO;
  } else if (items.at(index).engine == L"Trident") {
    web_engine = WEBENGINE_TRIDENT;
  } else if (items.at(index).engine == L"Presto") {
    web_engine = WEBENGINE_PRESTO;
  } else {
    return L"";
  }

  // Build accessibility data
  acc_obj.children.clear();
  if (acc_obj.FromWindow(hwnd) == S_OK) {
    acc_obj.BuildChildren(acc_obj.children, nullptr, web_engine);
    acc_obj.Release();
  }

  // Check other tabs
  if (CurrentEpisode.anime_id > 0) {
    AccessibleChild* child = nullptr;
    switch (web_engine) {
      case WEBENGINE_WEBKIT:
      case WEBENGINE_GECKO:
        child = FindAccessibleChild(acc_obj.children, L"", L"page tab list");
        break;
      case WEBENGINE_TRIDENT:
        child = FindAccessibleChild(acc_obj.children, L"Tab Row", L"");
        break;
      case WEBENGINE_PRESTO:
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
  switch (web_engine) {
    case WEBENGINE_WEBKIT:
      child = FindAccessibleChild(acc_obj.children, L"Address and search bar", L"grouping");
      if (child == nullptr)
        child = FindAccessibleChild(acc_obj.children, L"Address", L"grouping");
      if (child == nullptr)
        child = FindAccessibleChild(acc_obj.children, L"Location", L"grouping");
      if (child == nullptr)
        child = FindAccessibleChild(acc_obj.children, L"Address field", L"editable text");
      break;
    case WEBENGINE_GECKO:
      child = FindAccessibleChild(acc_obj.children, L"Search or enter address", L"editable text");
      if (child == nullptr)
        child = FindAccessibleChild(acc_obj.children, L"Go to a Website", L"editable text");
      if (child == nullptr)
        child = FindAccessibleChild(acc_obj.children, L"Go to a Web Site", L"editable text");
      break;
    case WEBENGINE_TRIDENT:
      child = FindAccessibleChild(acc_obj.children, L"Address and search using Bing", L"editable text");
      if (child == nullptr)
        child = FindAccessibleChild(acc_obj.children, L"Address and search using Google", L"editable text");
      break;
    case WEBENGINE_PRESTO:
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
      Erase(title, L" (s)");
      Erase(title, L" (d)");
      break;
    // Crunchyroll
    case STREAM_CRUNCHYROLL:
      EraseLeft(title, L"Crunchyroll - Watch ");
      break;
    // Hulu
    case STREAM_HULU:
      EraseLeft(title, L"Watch ");
      EraseRight(title, L" online | Free | Hulu");
      EraseRight(title, L" online | Plus | Hulu");
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
    case WEBENGINE_UNKNOWN:
      return false;
    case WEBENGINE_GECKO:
      if (IsEqual(child.role, L"document"))
        return false;
      break;
    case WEBENGINE_TRIDENT:
      if (IsEqual(child.role, L"pane") || IsEqual(child.role, L"scroll bar"))
        return false;
      break;
    case WEBENGINE_PRESTO:
      if (IsEqual(child.role, L"document") || IsEqual(child.role, L"pane"))
        return false;
      break;
  }

  return true;
}