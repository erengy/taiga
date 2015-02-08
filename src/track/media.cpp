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

#include <windows.h>
#include <tlhelp32.h>

#include "base/file.h"
#include "base/foreach.h"
#include "base/process.h"
#include "base/string.h"
#include "base/xml.h"
#include "library/anime.h"
#include "library/anime_db.h"
#include "library/anime_episode.h"
#include "library/anime_util.h"
#include "taiga/path.h"
#include "taiga/settings.h"
#include "taiga/timer.h"
#include "track/media.h"
#include "track/recognition.h"
#include "ui/dlg/dlg_anime_info.h"
#include "ui/dialog.h"
#include "ui/ui.h"

class MediaPlayers MediaPlayers;

MediaPlayers::MediaPlayers()
    : current_window_handle_(nullptr),
      player_running_(false),
      title_changed_(false) {
}

bool MediaPlayers::Load() {
  items.clear();

  xml_document document;
  std::wstring path = taiga::GetPath(taiga::kPathMedia);
  xml_parse_result parse_result = document.load_file(path.c_str());

  if (parse_result.status != pugi::status_ok) {
    ui::DisplayErrorMessage(L"Could not read media list.", path.c_str());
    return false;
  }

  // Read player list
  xml_node mediaplayers = document.child(L"media_players");
  foreach_xmlnode_(player, mediaplayers, L"player") {
    items.resize(items.size() + 1);
    items.back().name = XmlReadStrValue(player, L"name");
    items.back().enabled = XmlReadIntValue(player, L"enabled");
    items.back().engine = XmlReadStrValue(player, L"engine");
    items.back().visible = XmlReadIntValue(player, L"visible");
    items.back().mode = XmlReadIntValue(player, L"mode");
    XmlReadChildNodes(player, items.back().classes, L"class");
    XmlReadChildNodes(player, items.back().files, L"file");
    XmlReadChildNodes(player, items.back().folders, L"folder");
    for (xml_node child_node = player.child(L"edit"); child_node;
         child_node = child_node.next_sibling(L"edit")) {
      MediaPlayer::EditTitle edit;
      edit.mode = child_node.attribute(L"mode").as_int();
      edit.value = child_node.child_value();
      items.back().edits.push_back(edit);
    }
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

MediaPlayer* MediaPlayers::FindPlayer(const std::wstring& name) {
  if (!name.empty())
    foreach_(item, items)
      if (item->name == name)
        return &(*item);

  return nullptr;
}

bool MediaPlayers::IsPlayerActive() const {
  if (!Settings.GetBool(taiga::kSync_Update_CheckPlayer))
    return true;

  return current_window_handle_ &&
         current_window_handle_ == GetForegroundWindow();
}

HWND MediaPlayers::current_window_handle() const {
  return current_window_handle_;
}

std::wstring MediaPlayers::current_player() const {
  return current_player_;
}

bool MediaPlayers::player_running() const {
  return player_running_;
}

void MediaPlayers::set_player_running(bool player_running) {
  player_running_ = player_running;
}

std::wstring MediaPlayers::current_title() const {
  return current_title_;
}

bool MediaPlayers::title_changed() const {
  return title_changed_;
}

void MediaPlayers::set_title_changed(bool title_changed) {
  title_changed_ = title_changed;
}

////////////////////////////////////////////////////////////////////////////////

MediaPlayer* MediaPlayers::CheckRunningPlayers() {
  // Go through windows, starting with the highest in the Z-order
  HWND hwnd = GetWindow(ui::GetWindowHandle(ui::kDialogMain), GW_HWNDFIRST);
  while (hwnd != nullptr) {
    foreach_(item, items) {
      if (!item->enabled)
        continue;
      if (item->visible && !IsWindowVisible(hwnd))
        continue;

      // Compare window classes
      foreach_(window_class, item->classes) {
        if (*window_class == GetWindowClass(hwnd)) {
          // Compare file names
          foreach_(file, item->files) {
            if (IsEqual(*file, GetFileName(GetWindowPath(hwnd)))) {
              if (item->mode != kMediaModeWebBrowser ||
                  !GetWindowTitle(hwnd).empty()) {
                // Stick with the previously recognized window, if there is one
                bool recognized = anime::IsValidId(CurrentEpisode.anime_id);
                if (!recognized || current_window_handle_ == hwnd) {
                  // We have a match!
                  player_running_ = true;
                  current_player_ = item->name;
                  std::wstring title = GetTitle(hwnd, *window_class, item->mode);
                  EditTitle(title, &(*item));
                  if (current_title_ != title) {
                    current_title_ = title;
                    set_title_changed(true);
                  }
                  current_window_handle_ = hwnd;
                  return &(*item);
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
  current_player_.clear();
  if (!current_title_.empty()) {
    current_title_.clear();
    set_title_changed(true);
  }
  current_window_handle_ = nullptr;
  return nullptr;
}

MediaPlayer* MediaPlayers::GetRunningPlayer() {
  return FindPlayer(current_player_);
}

void MediaPlayers::EditTitle(std::wstring& str,
                             const MediaPlayer* media_player) {
  if (str.empty() || !media_player || media_player->edits.empty())
    return;

  foreach_(it, media_player->edits) {
    switch (it->mode) {
      // Erase
      case 1: {
        ReplaceString(str, it->value, L"");
        break;
      }
      // Cut right side
      case 2: {
        int pos = InStr(str, it->value, 0);
        if (pos > -1)
          str.resize(pos);
        break;
      }
    }
  }

  TrimRight(str, L" -");
}

std::wstring MediaPlayer::GetPath() const {
  foreach_(folder, folders) {
    foreach_(file, files) {
      std::wstring path = *folder + *file;
      path = ExpandEnvironmentStrings(path);
      if (FileExists(path))
        return path;
    }
  }

  return std::wstring();
}

std::wstring MediaPlayers::GetTitle(HWND hwnd,
                                    const std::wstring& class_name,
                                    int mode) {
  switch (mode) {
    // File handle
    case kMediaModeFileHandle:
      return GetTitleFromProcessHandle(hwnd);
    // Winamp API
    case kMediaModeWinampApi:
      return GetTitleFromWinampAPI(hwnd, false);
    // Special message
    case kMediaModeSpecialMessage:
      return GetTitleFromSpecialMessage(hwnd, class_name);
    // MPlayer
    case kMediaModeMplayer:
      return GetTitleFromMPlayer();
    // Browser
    case kMediaModeWebBrowser:
      return GetTitleFromBrowser(hwnd);
    // Window title
    case kMediaModeWindowTitle:
    default:
      return GetWindowTitle(hwnd);
  }
}

////////////////////////////////////////////////////////////////////////////////

void ProcessMediaPlayerStatus(const MediaPlayer* media_player) {
  // Media player is running
  if (media_player) {
    ProcessMediaPlayerTitle(*media_player);

  // Media player is not running
  } else {
    auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);

    // Media player was running, and the media was recognized
    if (anime_item) {
      bool processed = CurrentEpisode.processed;  // TODO: temporary solution...
      CurrentEpisode.Set(anime::ID_UNKNOWN);
      EndWatching(*anime_item, CurrentEpisode);
      if (Settings.GetBool(taiga::kSync_Update_WaitPlayer)) {
        CurrentEpisode.anime_id = anime_item->GetId();
        CurrentEpisode.processed = processed;
        UpdateList(*anime_item, CurrentEpisode);
        CurrentEpisode.anime_id = anime::ID_UNKNOWN;
      }
      taiga::timers.timer(taiga::kTimerMedia)->Reset();

    // Media player was running, but not watching
    } else if (MediaPlayers.player_running()) {
      ui::ClearStatusText();
      CurrentEpisode.Set(anime::ID_UNKNOWN);
      MediaPlayers.set_player_running(false);
      ui::DlgNowPlaying.SetCurrentId(anime::ID_UNKNOWN);
    }
  }
}

void ProcessMediaPlayerTitle(const MediaPlayer& media_player) {
  auto anime_item = AnimeDatabase.FindItem(CurrentEpisode.anime_id);

  if (CurrentEpisode.anime_id == anime::ID_UNKNOWN) {
    if (!Settings.GetBool(taiga::kApp_Option_EnableRecognition))
      return;

    CurrentEpisode.streaming_media = !media_player.engine.empty();

    // Examine title and compare it with list items
    if (Meow.Parse(MediaPlayers.current_title(), CurrentEpisode)) {
      bool is_inside_root_folders = true;
      if (Settings.GetBool(taiga::kSync_Update_OutOfRoot))
        if (!CurrentEpisode.folder.empty() && !Settings.root_folders.empty())
          is_inside_root_folders = anime::IsInsideRootFolders(CurrentEpisode.folder);
      if (is_inside_root_folders) {
        static track::recognition::MatchOptions match_options;
        match_options.check_airing_date = true;
        match_options.check_anime_type = true;
        match_options.check_episode_number = true;
        auto anime_id = Meow.Identify(CurrentEpisode, true, match_options);
        if (anime::IsValidId(anime_id)) {
          // Recognized
          anime_item = AnimeDatabase.FindItem(anime_id);
          MediaPlayers.set_title_changed(false);
          CurrentEpisode.Set(anime_item->GetId());
          StartWatching(*anime_item, CurrentEpisode);
          return;
        }
      }
    }
    // Not recognized
    CurrentEpisode.Set(anime::ID_NOTINLIST);
    ui::OnRecognitionFail();

  } else {
    if (MediaPlayers.title_changed()) {
      // Caption changed
      MediaPlayers.set_title_changed(false);
      ui::ClearStatusText();
      bool processed = CurrentEpisode.processed;  // TODO: not a good solution...
      CurrentEpisode.Set(anime::ID_UNKNOWN);
      if (anime_item) {
        EndWatching(*anime_item, CurrentEpisode);
        CurrentEpisode.anime_id = anime_item->GetId();
        CurrentEpisode.processed = processed;
        UpdateList(*anime_item, CurrentEpisode);
        CurrentEpisode.anime_id = anime::ID_UNKNOWN;
      }
      taiga::timers.timer(taiga::kTimerMedia)->Reset();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

// BS.Player
#define BSP_CLASS L"BSPlayer"
#define BSP_GETFILENAME 0x1010B

// Winamp
#define IPC_ISPLAYING        104
#define IPC_GETLISTPOS       125
#define IPC_GETPLAYLISTFILE  211
#define IPC_GETPLAYLISTFILEW 214

std::wstring MediaPlayers::GetTitleFromProcessHandle(HWND hwnd, ULONG process_id) {
  if (hwnd != NULL && process_id == 0)
    GetWindowThreadProcessId(hwnd, &process_id);

  std::vector<std::wstring> files_vector;

  if (GetProcessFiles(process_id, files_vector)) {
    foreach_(it, files_vector) {
      if (Meow.IsValidFileExtension(GetFileExtension(*it))) {
        if (it->at(1) != L':') {
          TranslateDeviceName(*it);
        }
        if (it->at(1) == L':') {
          WCHAR buffer[4096] = {0};
          GetLongPathName(it->c_str(), buffer, 4096);
          return std::wstring(buffer);
        } else {
          return GetFileName(*it);
        }
      }
    }
  }

  return std::wstring();
}

std::wstring MediaPlayers::GetTitleFromWinampAPI(HWND hwnd, bool use_unicode) {
  if (IsWindow(hwnd)) {
    if (SendMessage(hwnd, WM_USER, 0, IPC_ISPLAYING)) {
      int list_index = SendMessage(hwnd, WM_USER, 0, IPC_GETLISTPOS);
      int base_address = SendMessage(hwnd, WM_USER, list_index,
          use_unicode ? IPC_GETPLAYLISTFILEW : IPC_GETPLAYLISTFILE);
      if (base_address) {
        DWORD process_id;
        GetWindowThreadProcessId(hwnd, &process_id);
        HANDLE hwnd_winamp = OpenProcess(PROCESS_VM_READ, FALSE, process_id);
        if (hwnd_winamp) {
          if (use_unicode) {
            wchar_t file_name[MAX_PATH];
            ReadProcessMemory(hwnd_winamp,
                              reinterpret_cast<LPCVOID>(base_address),
                              file_name, MAX_PATH, NULL);
            CloseHandle(hwnd_winamp);
            return file_name;
          } else {
            char file_name[MAX_PATH];
            ReadProcessMemory(hwnd_winamp,
                              reinterpret_cast<LPCVOID>(base_address),
                              file_name, MAX_PATH, NULL);
            CloseHandle(hwnd_winamp);
            return StrToWstr(file_name);
          }
        }
      }
    }
  }

  return std::wstring();
}

std::wstring MediaPlayers::GetTitleFromSpecialMessage(
    HWND hwnd,
    const std::wstring& class_name) {
  // BS.Player
  if (class_name == BSP_CLASS) {
    if (IsWindow(hwnd)) {
      COPYDATASTRUCT cds;
      char file_name[MAX_PATH];
      void* data = &file_name;
      cds.dwData = BSP_GETFILENAME;
      cds.lpData = &data;
      cds.cbData = 4;
      SendMessage(hwnd, WM_COPYDATA,
                  reinterpret_cast<WPARAM>(ui::GetWindowHandle(ui::kDialogMain)),
                  reinterpret_cast<LPARAM>(&cds));
      return StrToWstr(file_name);
    }
  }

  return std::wstring();
}

std::wstring MediaPlayers::GetTitleFromMPlayer() {
  std::wstring title;

  HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hProcessSnap != INVALID_HANDLE_VALUE) {
    PROCESSENTRY32 pe32;
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