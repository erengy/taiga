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

#ifndef TAIGA_TRACK_TORRENT_CLIENT_H
#define TAIGA_TRACK_TORRENT_CLIENT_H

#include <string>
#include <map>
#include <memory>

class TorrentClient;

class TorrentClients {
public:
  TorrentClients();

  std::shared_ptr<TorrentClient> GetClientByPath(const std::wstring& path);
private:
  std::map<std::wstring, std::shared_ptr<TorrentClient>> clients_;
};

class TorrentClient {
public:  
  virtual ~TorrentClient() {}

  void DownloadTorrent(const std::wstring& app_path, const std::wstring& download_path, const std::wstring& file);
protected:
  virtual std::wstring GenerateParameters(const std::wstring& download_path, const std::wstring& file) = 0;
};

class Deluge : public TorrentClient {
public:
  static std::wstring& kBinaryName();
private:
  std::wstring GenerateParameters(const std::wstring& download_path, const std::wstring& file);
};

class UTorrent : public TorrentClient {
public:
  static std::wstring& kBinaryName();
private:
  std::wstring GenerateParameters(const std::wstring& download_path, const std::wstring& file);
};

#endif  // TAIGA_TRACK_TORRENT_CLIENT_H