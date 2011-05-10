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
#include "common.h"
#include "string.h"
#include "update.h"
#include "xml.h"

// =============================================================================

CUpdateHelper::CUpdateHelper() :
  RestartApp(false), UpdateAvailable(false), m_App(NULL)
{
}

// =============================================================================

bool CUpdateHelper::Check(const wstring& address, CApp& app, DWORD client_mode) {
  OnCheck();
  m_App = &app;
  if (m_App == NULL || address.empty()) return false;

  CUrl url(address);
  return Client.Get(url, L"", client_mode);
}

bool CUpdateHelper::DownloadNextFile(DWORD client_mode) {
  // Check files
  for (unsigned int i = 0; i < Files.size(); i++) {
    if (Files[i].Download) {
      OnProgress(i);

      // Get file path
      wstring path = CheckSlash(GetPathOnly(m_App->GetModulePath())) + Files[i].Path;
      ReplaceChar(path, '/', '\\');
      CreateFolder(GetPathOnly(path));
      if (!RestartApp) {
        if (IsEqual(Files[i].Path, GetFileName(m_App->GetModulePath()))) {
          path += L".new";
          RestartApp = true;
        }
      }

      // Download file
      CUrl url(VersionInfo.URL + Files[i].Path);
      return Client.Get(url, path, client_mode, reinterpret_cast<LPARAM>(&Files[i]));
    }
  }

  OnDone();
  return false;
}

bool CUpdateHelper::ParseData(wstring data, DWORD client_mode) {
  // Reset values
  Actions.clear();
  Files.clear();
  UpdateAvailable = false;
  
  // Read XML data
  xml_document doc;
  xml_parse_result result = doc.load(data.c_str());
  if (result.status != status_ok) {
    return false;
  }
  
  // Read startup actions
  xml_node actions = doc.child(L"actions");
  for (xml_node action = actions.child(L"action"); action; action = action.next_sibling(L"action")) {
    Actions.push_back(action.child_value());
  }

  // Read version information
  xml_node version = doc.child(L"version");
  VersionInfo.Major = version.child_value(L"major");
  VersionInfo.Minor = version.child_value(L"minor");
  VersionInfo.Revision = version.child_value(L"revision");
  VersionInfo.Build = version.child_value(L"build");
  VersionInfo.URL = version.child_value(L"url");

  // Compare version information
  if (ToINT(VersionInfo.Major) > m_App->GetVersionMajor() || 
      ToINT(VersionInfo.Minor) > m_App->GetVersionMinor() || 
      ToINT(VersionInfo.Revision) > m_App->GetVersionRevision()) {
        UpdateAvailable = true;
  }

  // Read files
  xml_node files = doc.child(L"files");
  for (xml_node file = files.child(L"file"); file; file = file.next_sibling(L"file")) {
    Files.resize(Files.size() + 1);
    Files.back().Path = file.child_value(L"path");
    Files.back().Checksum = file.child_value(L"checksum");
  }

  // Check files
  for (unsigned int i = 0; i < Files.size(); i++) {
    if (Files[i].Checksum.empty()) {
      Files[i].Download = true;
    } else {
      wstring path = CheckSlash(GetPathOnly(m_App->GetModulePath())) + Files[i].Path;
      ReplaceChar(path, '/', '\\');
      wstring crc;
      OnCRCCheck(path, crc);
      if (crc.empty() || !IsEqual(crc, Files[i].Checksum)) {
        Files[i].Download = true;
      } else {
        Files[i].Download = false;
      }
    }
  }

  if (UpdateAvailable && DownloadNextFile(client_mode)) {
    return true;
  } else {
    return false;
  }
}

bool CUpdateHelper::RestartApplication(const wstring& updatehelper_exe, const wstring& current_exe, const wstring& new_exe) {
  if (RestartApp) {
    if (FileExists(CheckSlash(m_App->GetCurrentDirectory()) + new_exe)) {
      if (OnRestartApp()) {
        Execute(updatehelper_exe, current_exe + L" " + new_exe + L" " + ToWSTR(GetCurrentProcessId()));
        m_App->PostQuitMessage();
        return true;
      }
    }
  }

  return false;
}