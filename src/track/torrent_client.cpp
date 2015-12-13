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

#include "base/file.h"
#include "base/log.h"
#include "base/string.h"
#include "track/torrent_client.h"

void TorrentClient::DownloadTorrent(const std::wstring& app_path, const std::wstring& download_path, const std::wstring& file) {
  std::wstring parameters = GenerateParameters(download_path, file);
  Execute(app_path, parameters);
}

std::wstring Deluge::GenerateParameters(const std::wstring& download_path, const std::wstring& file) {
  return L"add -p \\\"" + download_path + L"\\\" \\\"" + file + L"\\\"";
}

std::wstring& Deluge::kBinaryName() {
  static std::wstring *x = new std::wstring(L"deluge-console.exe");
  return *x;
}

std::wstring UTorrent::GenerateParameters(const std::wstring& download_path, const std::wstring& file) {
  std::wstring parameters;
  if (!download_path.empty()) {
    parameters = L"/directory \"" + download_path + L"\" ";
  }

  return parameters + L"\"" + file + L"\"";
}

std::wstring& UTorrent::kBinaryName() {
  static std::wstring *x = new std::wstring(L"uTorrent.exe");
  return *x;
}

TorrentClients::TorrentClients() {
  clients_[Deluge::kBinaryName()]   = std::make_shared<Deluge>();
  clients_[UTorrent::kBinaryName()] = std::make_shared<UTorrent>();
}

std::shared_ptr<TorrentClient> TorrentClients::GetClientByPath(const std::wstring& path) {
  std::wstring app = GetFileName(path);

  auto item = clients_.find(app);
  if (item != clients_.end()) {
    return item->second;
  }

  LOG(LevelNotice, L"Application is not a supported torrent client:\n"
                   L"Path: " + app);
  return nullptr;
}