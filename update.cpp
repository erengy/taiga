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

#include "std.h"

#include "update.h"

#include "common.h"
#include "string.h"
#include "xml.h"

// =============================================================================

UpdateHelper::UpdateHelper()
    : restart_app(false), 
      update_available(false), 
      app_(nullptr) {
}

// =============================================================================

bool UpdateHelper::Check(const wstring& address, win32::App& app, DWORD client_mode) {
  OnCheck();
  app_ = &app;
  if (app_ == nullptr || address.empty()) return false;

  win32::Url url(address);
  return client.Get(url, L"", client_mode);
}

bool UpdateHelper::DownloadNextFile(DWORD client_mode) {
  // Check files
  for (unsigned int i = 0; i < files.size(); i++) {
    if (files[i].download) {
      OnProgress(i);

      // Get file path
      wstring path = CheckSlash(GetPathOnly(app_->GetModulePath())) + files[i].path;
      ReplaceChar(path, '/', '\\');
      CreateFolder(GetPathOnly(path));
      if (!restart_app) {
        if (IsEqual(files[i].path, GetFileName(app_->GetModulePath()))) {
          path += L".new";
          restart_app = true;
        }
      }

      // Download file
      win32::Url url(version_info.url + files[i].path);
      return client.Get(url, path, client_mode, reinterpret_cast<LPARAM>(&files[i]));
    }
  }

  OnDone();
  return false;
}

bool UpdateHelper::ParseData(wstring data, DWORD client_mode) {
  // Reset values
  actions.clear();
  files.clear();
  update_available = false;
  
  // Load XML data
  xml_document doc;
  xml_parse_result result = doc.load(data.c_str());
  if (result.status != status_ok) {
    return false;
  }
  
  // Read startup actions
  xml_node actions_node = doc.child(L"actions");
  for (xml_node action = actions_node.child(L"action"); action; action = action.next_sibling(L"action")) {
    actions.push_back(action.child_value());
  }

  // Read version information
  xml_node version = doc.child(L"version");
  version_info.major = version.child_value(L"major");
  version_info.minor = version.child_value(L"minor");
  version_info.revision = version.child_value(L"revision");
  version_info.build = version.child_value(L"build");
  version_info.url = version.child_value(L"url");

  // Compare version information
  // TODO: fix (0.9 > 1.0 because 9 > 0)
  if (ToInt(version_info.major) > app_->GetVersionMajor() || 
      ToInt(version_info.minor) > app_->GetVersionMinor() || 
      ToInt(version_info.revision) > app_->GetVersionRevision()) {
        update_available = true;
  }

  // Read files
  xml_node files_node = doc.child(L"files");
  for (xml_node file = files_node.child(L"file"); file; file = file.next_sibling(L"file")) {
    files.resize(files.size() + 1);
    files.back().path = file.child_value(L"path");
    files.back().checksum = file.child_value(L"checksum");
  }

  // Check files
  for (unsigned int i = 0; i < files.size(); i++) {
    if (files[i].checksum.empty()) {
      files[i].download = true;
    } else {
      wstring path = CheckSlash(GetPathOnly(app_->GetModulePath())) + files[i].path;
      ReplaceChar(path, '/', '\\');
      wstring crc;
      OnCRCCheck(path, crc);
      if (crc.empty() || !IsEqual(crc, files[i].checksum)) {
        files[i].download = true;
      } else {
        files[i].download = false;
      }
    }
  }

  if (update_available && DownloadNextFile(client_mode)) {
    return true;
  } else {
    return false;
  }
}

bool UpdateHelper::RestartApplication(const wstring& updatehelper_exe, const wstring& current_exe, const wstring& new_exe) {
  if (restart_app) {
    if (FileExists(CheckSlash(app_->GetCurrentDirectory()) + new_exe)) {
      if (OnRestartApp()) {
        Execute(updatehelper_exe, current_exe + L" " + new_exe + L" " + ToWstr(GetCurrentProcessId()));
        app_->PostQuitMessage();
        return true;
      }
    }
  }

  return false;
}