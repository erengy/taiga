/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include "ui/dlg/dlg_settings_advanced.h"

#include "base/string.h"

namespace ui {

std::vector<AdvancedSetting> GetAdvancedSettingKeys() {
  static const std::vector<AdvancedSetting> keys = {
    AdvancedSetting::AppConnectionNoRevoke,
    AdvancedSetting::SyncNotifyFormat,
    AdvancedSetting::AppConnectionProxyHost,
    AdvancedSetting::AppConnectionProxyPassword,
    AdvancedSetting::AppConnectionProxyUsername,
    AdvancedSetting::AppPositionRemember,
    AdvancedSetting::AppConnectionReuseActive,
    AdvancedSetting::AppInterfaceTheme,
    AdvancedSetting::LibraryFileSizeThreshold,
    AdvancedSetting::LibraryMediaPlayerPath,
    AdvancedSetting::RecognitionIgnoredStrings,
    AdvancedSetting::RecognitionLookupParentDirectories,
    AdvancedSetting::RecognitionDetectionInterval,
    AdvancedSetting::SyncServiceKitsuPartialLibrary,
    AdvancedSetting::ShareDiscordApplicationId,
    AdvancedSetting::TorrentFilterArchiveMaxCount,
    AdvancedSetting::TorrentDownloadFileLocation,
    AdvancedSetting::TorrentDownloadUseMagnet,
  };
  return keys;
}

std::wstring GetAdvancedSettingDescription(const AdvancedSetting key) {
  switch (key) {
    case AdvancedSetting::AppConnectionNoRevoke:
      return L"Application / Disable certificate revocation checks";
    case AdvancedSetting::SyncNotifyFormat:
      return L"Application / Episode notification format";
    case AdvancedSetting::AppConnectionProxyHost:
      return L"Application / Proxy host";
    case AdvancedSetting::AppConnectionProxyPassword:
      return L"Application / Proxy password";
    case AdvancedSetting::AppConnectionProxyUsername:
      return L"Application / Proxy username";
    case AdvancedSetting::AppPositionRemember:
      return L"Application / Remember main window position and size";
    case AdvancedSetting::AppConnectionReuseActive:
      return L"Application / Reuse active connections";
    case AdvancedSetting::AppInterfaceTheme:
      return L"Application / UI theme";
    case AdvancedSetting::LibraryFileSizeThreshold:
      return L"Library / File size threshold";
    case AdvancedSetting::LibraryMediaPlayerPath:
      return L"Library / Media player path";
    case AdvancedSetting::RecognitionIgnoredStrings:
      return L"Recognition / Ignored strings";
    case AdvancedSetting::RecognitionLookupParentDirectories:
      return L"Recognition / Look up parent directories";
    case AdvancedSetting::RecognitionDetectionInterval:
      return L"Recognition / Media detection interval";
    case AdvancedSetting::SyncServiceKitsuPartialLibrary:
      return L"Services / Kitsu / Download partial library";
    case AdvancedSetting::ShareDiscordApplicationId:
      return L"Sharing / Discord / Application ID";
    case AdvancedSetting::TorrentFilterArchiveMaxCount:
      return L"Torrents / Archive limit";
    case AdvancedSetting::TorrentDownloadFileLocation:
      return L"Torrents / Download path for .torrent files";
    case AdvancedSetting::TorrentDownloadUseMagnet:
      return L"Torrents / Use magnet links if available";
  }

  return std::wstring{};
}

std::wstring GetAdvancedSettingValue(const AdvancedSetting key) {
  const auto bool_to_wstr = [](const bool value) {
    return value ? L"true" : L"false";
  };

  switch (key) {
    case AdvancedSetting::AppConnectionNoRevoke:
      return bool_to_wstr(taiga::settings.GetAppConnectionNoRevoke());
    case AdvancedSetting::SyncNotifyFormat:
      return taiga::settings.GetSyncNotifyFormat();
    case AdvancedSetting::AppConnectionProxyHost:
      return taiga::settings.GetAppConnectionProxyHost();
    case AdvancedSetting::AppConnectionProxyPassword:
      return taiga::settings.GetAppConnectionProxyPassword();
    case AdvancedSetting::AppConnectionProxyUsername:
      return taiga::settings.GetAppConnectionProxyUsername();
    case AdvancedSetting::AppPositionRemember:
      return bool_to_wstr(taiga::settings.GetAppPositionRemember());
    case AdvancedSetting::AppConnectionReuseActive:
      return bool_to_wstr(taiga::settings.GetAppConnectionReuseActive());
    case AdvancedSetting::AppInterfaceTheme:
      return taiga::settings.GetAppInterfaceTheme();
    case AdvancedSetting::LibraryFileSizeThreshold:
      return ToWstr(taiga::settings.GetLibraryFileSizeThreshold());
    case AdvancedSetting::LibraryMediaPlayerPath:
      return taiga::settings.GetLibraryMediaPlayerPath();
    case AdvancedSetting::RecognitionIgnoredStrings:
      return taiga::settings.GetRecognitionIgnoredStrings();
    case AdvancedSetting::RecognitionLookupParentDirectories:
      return bool_to_wstr(taiga::settings.GetRecognitionLookupParentDirectories());
    case AdvancedSetting::RecognitionDetectionInterval:
      return ToWstr(taiga::settings.GetRecognitionDetectionInterval());
    case AdvancedSetting::SyncServiceKitsuPartialLibrary:
      return bool_to_wstr(taiga::settings.GetSyncServiceKitsuPartialLibrary());
    case AdvancedSetting::ShareDiscordApplicationId:
      return taiga::settings.GetShareDiscordApplicationId();
    case AdvancedSetting::TorrentFilterArchiveMaxCount:
      return ToWstr(taiga::settings.GetTorrentFilterArchiveMaxCount());
    case AdvancedSetting::TorrentDownloadFileLocation:
      return taiga::settings.GetTorrentDownloadFileLocation();
    case AdvancedSetting::TorrentDownloadUseMagnet:
      return bool_to_wstr(taiga::settings.GetTorrentDownloadUseMagnet());
  }

  return std::wstring{};
}

void SetAdvancedSetting(const AdvancedSetting key, const std::wstring& value) {
  switch (key) {
    case AdvancedSetting::AppConnectionNoRevoke:
      taiga::settings.SetAppConnectionNoRevoke(ToBool(value));
      break;
    case AdvancedSetting::SyncNotifyFormat:
      taiga::settings.SetSyncNotifyFormat(value);
      break;
    case AdvancedSetting::AppConnectionProxyHost:
      taiga::settings.SetAppConnectionProxyHost(value);
      break;
    case AdvancedSetting::AppConnectionProxyPassword:
      taiga::settings.SetAppConnectionProxyPassword(value);
      break;
    case AdvancedSetting::AppConnectionProxyUsername:
      taiga::settings.SetAppConnectionProxyUsername(value);
      break;
    case AdvancedSetting::AppPositionRemember:
      taiga::settings.SetAppPositionRemember(ToBool(value));
      break;
    case AdvancedSetting::AppConnectionReuseActive:
      taiga::settings.SetAppConnectionReuseActive(ToBool(value));
      break;
    case AdvancedSetting::AppInterfaceTheme:
      taiga::settings.SetAppInterfaceTheme(value);
      break;
    case AdvancedSetting::LibraryFileSizeThreshold:
      taiga::settings.SetLibraryFileSizeThreshold(ToInt(value));
      break;
    case AdvancedSetting::LibraryMediaPlayerPath:
      taiga::settings.SetLibraryMediaPlayerPath(value);
      break;
    case AdvancedSetting::RecognitionIgnoredStrings:
      taiga::settings.SetRecognitionIgnoredStrings(value);
      break;
    case AdvancedSetting::RecognitionLookupParentDirectories:
      taiga::settings.SetRecognitionLookupParentDirectories(ToBool(value));
      break;
    case AdvancedSetting::RecognitionDetectionInterval:
      taiga::settings.SetRecognitionDetectionInterval(ToInt(value));
      break;
    case AdvancedSetting::SyncServiceKitsuPartialLibrary:
      taiga::settings.SetSyncServiceKitsuPartialLibrary(ToBool(value));
      break;
    case AdvancedSetting::ShareDiscordApplicationId:
      taiga::settings.SetShareDiscordApplicationId(value);
      break;
    case AdvancedSetting::TorrentFilterArchiveMaxCount:
      taiga::settings.SetTorrentFilterArchiveMaxCount(ToInt(value));
      break;
    case AdvancedSetting::TorrentDownloadFileLocation:
      taiga::settings.SetTorrentDownloadFileLocation(value);
      break;
    case AdvancedSetting::TorrentDownloadUseMagnet:
      taiga::settings.SetTorrentDownloadUseMagnet(ToBool(value));
      break;
  }
}

}  // namespace ui
