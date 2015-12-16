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

void TorrentClient::DownloadTorrent(const std::wstring& app_path,
                                    const std::wstring& download_path,
                                    const std::wstring& file) {
  std::wstring parameters = GenerateParameters(download_path, file);
  Execute(GetAppPath(app_path), parameters);
}

std::wstring Deluge::GenerateParameters(const std::wstring& download_path,
                                        const std::wstring& file) const {
  return L"add -p \\\"" + download_path + L"\\\" \\\"" + file + L"\\\"";
}

std::wstring TorrentClient::GetAppPath(const std::wstring& app_path) const {
  return app_path;
}

void Deluge::RegisterClient(TorrentClients& clients) {
  clients.AddClient(L"deluge.exe", shared_from_this());
  clients.AddClient(L"deluge-console.exe", shared_from_this());
}

std::wstring Deluge::GetAppPath(const std::wstring& app_path) const {
  if (GetFileName(app_path) == L"deluge.exe") {
    return GetPathOnly(app_path) + L"deluge-console.exe";
  }
  return app_path;
}

std::wstring UTorrent::GenerateParameters(const std::wstring& download_path,
                                          const std::wstring& file) const {
  std::wstring parameters;
  if (!download_path.empty()) {
    parameters = L"/directory \"" + download_path + L"\" ";
  }

  return parameters + L"\"" + file + L"\"";
}

void UTorrent::RegisterClient(TorrentClients& clients) {
  clients.AddClient(L"uTorrent.exe", shared_from_this());
}

TorrentClients::TorrentClients() {
  std::make_shared<Deluge>()->RegisterClient(*this);
  std::make_shared<UTorrent>()->RegisterClient(*this);
}

std::shared_ptr<TorrentClient> TorrentClients::GetClientByPath(const std::wstring& path) const {
  std::wstring app = GetFileName(path);

  auto item = clients_.find(app);
  if (item != clients_.end()) {
    return item->second;
  }

  LOG(LevelNotice, L"Application is not a supported torrent client:\n"
                   L"Path: " + app);
  return nullptr;
}

void TorrentClients::AddClient(const std::wstring& name,
                               const std::shared_ptr<TorrentClient> client) {
  clients_[name] = client;
}