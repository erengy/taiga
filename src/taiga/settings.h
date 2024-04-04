/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <nstd/compare.hpp>

#include "base/settings.h"
#include "taiga/settings_keys.h"

namespace anime {
enum class TitleLanguage;
}
namespace semaver {
class Version;
}
namespace sync {
enum class ServiceId;
}
namespace track {
enum TorrentAction;
enum TorrentApp;
}

namespace taiga {

class Settings {
public:
  class AnimeListColumn : public nstd::Comparable<AnimeListColumn> {
  public:
    int order = 0;
    bool visible = true;
    int width = 0;

    int compare(const AnimeListColumn& rhs) const override;
  };

  struct AnimeSettings {
    std::wstring folder;
    std::vector<std::wstring> synonyms;
    bool use_alternative = false;
  };

  Settings() = default;
  ~Settings();

  bool Load();
  bool Save();

  void ApplyChanges();
  bool HandleCompatibility();
  void SetModified();

  base::Settings::value_t GetDefaultValue(const AppSettingKey key) const;

  // Meta
  semaver::Version GetMetaVersion() const;

  // Services
  sync::ServiceId GetSyncActiveService() const;
  void SetSyncActiveService(const sync::ServiceId service_id);
  bool GetSyncAutoOnStart() const;
  void SetSyncAutoOnStart(const bool enabled);
  std::wstring GetSyncServiceMalUsername() const;
  void SetSyncServiceMalUsername(const std::wstring& username);
  std::wstring GetSyncServiceMalAccessToken() const;
  void SetSyncServiceMalAccessToken(const std::wstring& token);
  std::wstring GetSyncServiceMalRefreshToken() const;
  void SetSyncServiceMalRefreshToken(const std::wstring& token);
  std::wstring GetSyncServiceKitsuDisplayName() const;
  void SetSyncServiceKitsuDisplayName(const std::wstring& name);
  std::wstring GetSyncServiceKitsuEmail() const;
  void SetSyncServiceKitsuEmail(const std::wstring& email);
  std::wstring GetSyncServiceKitsuUsername() const;
  void SetSyncServiceKitsuUsername(const std::wstring& username);
  std::wstring GetSyncServiceKitsuPassword() const;
  void SetSyncServiceKitsuPassword(const std::wstring& password);
  bool GetSyncServiceKitsuPartialLibrary() const;
  void SetSyncServiceKitsuPartialLibrary(const bool enabled);
  std::wstring GetSyncServiceKitsuRatingSystem() const;
  void SetSyncServiceKitsuRatingSystem(const std::wstring& rating_system);
  std::wstring GetSyncServiceAniListUsername() const;
  void SetSyncServiceAniListUsername(const std::wstring& username);
  std::wstring GetSyncServiceAniListRatingSystem() const;
  void SetSyncServiceAniListRatingSystem(const std::wstring& rating_system);
  std::wstring GetSyncServiceAniListToken() const;
  void SetSyncServiceAniListToken(const std::wstring& token);

  // Library
  int GetLibraryFileSizeThreshold() const;
  void SetLibraryFileSizeThreshold(const int bytes);
  std::wstring GetLibraryMediaPlayerPath() const;
  void SetLibraryMediaPlayerPath(const std::wstring& path);
  bool GetLibraryWatchFolders() const;
  void SetLibraryWatchFolders(const bool enabled);

  // Application
  int GetAppListDoubleClickAction() const;
  void SetAppListDoubleClickAction(const int action);
  int GetAppListMiddleClickAction() const;
  void SetAppListMiddleClickAction(const int action);
  bool GetAppListDisplayEnglishTitles() const;
  void SetAppListDisplayEnglishTitles(const bool enabled);
  bool GetAppListHighlightNewEpisodes() const;
  void SetAppListHighlightNewEpisodes(const bool enabled);
  bool GetAppListDisplayHighlightedOnTop() const;
  void SetAppListDisplayHighlightedOnTop(const bool enabled);
  bool GetAppListProgressDisplayAired() const;
  void SetAppListProgressDisplayAired(const bool enabled);
  bool GetAppListProgressDisplayAvailable() const;
  void SetAppListProgressDisplayAvailable(const bool enabled);
  std::wstring GetAppListSortColumnPrimary() const;
  void SetAppListSortColumnPrimary(const std::wstring& column);
  std::wstring GetAppListSortColumnSecondary() const;
  void SetAppListSortColumnSecondary(const std::wstring& column);
  int GetAppListSortOrderPrimary() const;
  void SetAppListSortOrderPrimary(const int order);
  int GetAppListSortOrderSecondary() const;
  void SetAppListSortOrderSecondary(const int order);
  anime::TitleLanguage GetAppListTitleLanguagePreference() const;
  void SetAppListTitleLanguagePreference(const anime::TitleLanguage language);
  bool GetAppBehaviorAutostart() const;
  void SetAppBehaviorAutostart(const bool enabled);
  bool GetAppBehaviorStartMinimized() const;
  void SetAppBehaviorStartMinimized(const bool enabled);
  bool GetAppBehaviorCheckForUpdates() const;
  void SetAppBehaviorCheckForUpdates(const bool enabled);
  bool GetAppBehaviorScanAvailableEpisodes() const;
  void SetAppBehaviorScanAvailableEpisodes(const bool enabled);
  bool GetAppBehaviorCloseToTray() const;
  void SetAppBehaviorCloseToTray(const bool enabled);
  bool GetAppBehaviorMinimizeToTray() const;
  void SetAppBehaviorMinimizeToTray(const bool enabled);
  std::wstring GetAppConnectionProxyHost() const;
  void SetAppConnectionProxyHost(const std::wstring& host);
  std::wstring GetAppConnectionProxyUsername() const;
  void SetAppConnectionProxyUsername(const std::wstring& username);
  std::wstring GetAppConnectionProxyPassword() const;
  void SetAppConnectionProxyPassword(const std::wstring& password);
  bool GetAppConnectionNoRevoke() const;
  void SetAppConnectionNoRevoke(const bool enabled);
  bool GetAppConnectionReuseActive() const;
  void SetAppConnectionReuseActive(const bool enabled);
  std::wstring GetAppInterfaceTheme() const;
  void SetAppInterfaceTheme(const std::wstring& theme);
  std::wstring GetAppInterfaceExternalLinks() const;
  void SetAppInterfaceExternalLinks(const std::wstring& str);

  // Recognition
  int GetRecognitionDetectionInterval() const;
  void SetRecognitionDetectionInterval(const int seconds);
  bool GetRecognitionDetectMediaPlayers() const;
  void SetRecognitionDetectMediaPlayers(const bool enabled);
  bool GetRecognitionDetectStreamingMedia() const;
  void SetRecognitionDetectStreamingMedia(const bool enabled);
  std::wstring GetRecognitionIgnoredStrings() const;
  void SetRecognitionIgnoredStrings(const std::wstring& str);
  bool GetRecognitionLookupParentDirectories() const;
  void SetRecognitionLookupParentDirectories(const bool enabled);
  std::wstring GetRecognitionRelationsLastModified() const;
  void SetRecognitionRelationsLastModified(const std::wstring& last_modified);
  int GetSyncUpdateDelay() const;
  void SetSyncUpdateDelay(const int seconds);
  bool GetSyncUpdateAskToConfirm() const;
  void SetSyncUpdateAskToConfirm(const bool enabled);
  bool GetSyncUpdateCheckPlayer() const;
  void SetSyncUpdateCheckPlayer(const bool enabled);
  bool GetSyncUpdateOutOfRange() const;
  void SetSyncUpdateOutOfRange(const bool enabled);
  bool GetSyncUpdateOutOfRoot() const;
  void SetSyncUpdateOutOfRoot(const bool enabled);
  bool GetSyncUpdateWaitPlayer() const;
  void SetSyncUpdateWaitPlayer(const bool enabled);
  bool GetSyncGoToNowPlayingRecognized() const;
  void SetSyncGoToNowPlayingRecognized(const bool enabled);
  bool GetSyncGoToNowPlayingNotRecognized() const;
  void SetSyncGoToNowPlayingNotRecognized(const bool enabled);
  bool GetSyncNotifyRecognized() const;
  void SetSyncNotifyRecognized(const bool enabled);
  bool GetSyncNotifyNotRecognized() const;
  void SetSyncNotifyNotRecognized(const bool enabled);
  std::wstring GetSyncNotifyFormat() const;
  void SetSyncNotifyFormat(const std::wstring& format);
  bool GetStreamAnimelab() const;
  void SetStreamAnimelab(const bool enabled);
  bool GetStreamAdn() const;
  void SetStreamAdn(const bool enabled);
  bool GetStreamAnn() const;
  void SetStreamAnn(const bool enabled);
  bool GetStreamBilibili() const;
  void SetStreamBilibili(const bool enabled);
  bool GetStreamCrunchyroll() const;
  void SetStreamCrunchyroll(const bool enabled);
  bool GetStreamFunimation() const;
  void SetStreamFunimation(const bool enabled);
  bool GetStreamHidive() const;
  void SetStreamHidive(const bool enabled);
  bool GetStreamJellyfin() const;
  void SetStreamJellyfin(const bool enabled);
  bool GetStreamPlex() const;
  void SetStreamPlex(const bool enabled);
  bool GetStreamRokuChannel() const;
  void SetStreamRokuChannel(const bool enabled);
  bool GetStreamTubi() const;
  void SetStreamTubi(const bool enabled);
  bool GetStreamVeoh() const;
  void SetStreamVeoh(const bool enabled);
  bool GetStreamViz() const;
  void SetStreamViz(const bool enabled);
  bool GetStreamVrv() const;
  void SetStreamVrv(const bool enabled);
  bool GetStreamWakanim() const;
  void SetStreamWakanim(const bool enabled);
  bool GetStreamYahoo() const;
  void SetStreamYahoo(const bool enabled);
  bool GetStreamYoutube() const;
  void SetStreamYoutube(const bool enabled);

  // Sharing
  std::wstring GetShareDiscordApplicationId() const;
  void SetShareDiscordApplicationId(const std::wstring& application_id);
  bool GetShareDiscordEnabled() const;
  void SetShareDiscordEnabled(const bool enabled);
  bool GetShareDiscordGroupEnabled() const;
  void SetShareDiscordGroupEnabled(const bool enabled);
  bool GetShareDiscordTimeEnabled() const;
  void SetShareDiscordTimeEnabled(const bool enabled);
  bool GetShareDiscordUsernameEnabled() const;
  void SetShareDiscordUsernameEnabled(const bool enabled);
  bool GetShareHttpEnabled() const;
  void SetShareHttpEnabled(const bool enabled);
  std::wstring GetShareHttpFormat() const;
  void SetShareHttpFormat(const std::wstring& format);
  std::wstring GetShareHttpUrl() const;
  void SetShareHttpUrl(const std::wstring& url);
  bool GetShareMircEnabled() const;
  void SetShareMircEnabled(const bool enabled);
  bool GetShareMircMultiServer() const;
  void SetShareMircMultiServer(const bool enabled);
  bool GetShareMircUseMeAction() const;
  void SetShareMircUseMeAction(const bool enabled);
  int GetShareMircMode() const;
  void SetShareMircMode(const int mode);
  std::wstring GetShareMircChannels() const;
  void SetShareMircChannels(const std::wstring& channels);
  std::wstring GetShareMircFormat() const;
  void SetShareMircFormat(const std::wstring& format);
  std::wstring GetShareMircService() const;
  void SetShareMircService(const std::wstring& service);
  bool GetShareTwitterEnabled() const;
  void SetShareTwitterEnabled(const bool enabled);
  std::wstring GetShareTwitterFormat() const;
  void SetShareTwitterFormat(const std::wstring& format);
  std::wstring GetShareTwitterOauthToken() const;
  void SetShareTwitterOauthToken(const std::wstring& oauth_token);
  std::wstring GetShareTwitterOauthSecret() const;
  void SetShareTwitterOauthSecret(const std::wstring& oauth_secret);
  std::wstring GetShareTwitterReplyTo() const;
  void SetShareTwitterReplyTo(const std::wstring& status_id);
  std::wstring GetShareTwitterUsername() const;
  void SetShareTwitterUsername(const std::wstring& username);

  // Torrents
  std::wstring GetTorrentDiscoverySource() const;
  void SetTorrentDiscoverySource(const std::wstring& url);
  std::wstring GetTorrentDiscoverySearchUrl() const;
  void SetTorrentDiscoverySearchUrl(const std::wstring& url);
  bool GetTorrentDiscoveryAutoCheckEnabled() const;
  void SetTorrentDiscoveryAutoCheckEnabled(const bool enabled);
  int GetTorrentDiscoveryAutoCheckInterval() const;
  void SetTorrentDiscoveryAutoCheckInterval(const int minutes);
  track::TorrentAction GetTorrentDiscoveryNewAction() const;
  void SetTorrentDiscoveryNewAction(const track::TorrentAction action);
  track::TorrentApp GetTorrentDownloadAppMode() const;
  void SetTorrentDownloadAppMode(const track::TorrentApp mode);
  bool GetTorrentDownloadAppOpen() const;
  void SetTorrentDownloadAppOpen(const bool enabled);
  std::wstring GetTorrentDownloadAppPath() const;
  void SetTorrentDownloadAppPath(const std::wstring& path);
  std::wstring GetTorrentDownloadLocation() const;
  void SetTorrentDownloadLocation(const std::wstring& path);
  std::wstring GetTorrentDownloadFileLocation() const;
  void SetTorrentDownloadFileLocation(const std::wstring& path);
  bool GetTorrentDownloadUseAnimeFolder() const;
  void SetTorrentDownloadUseAnimeFolder(const bool enabled);
  bool GetTorrentDownloadFallbackOnFolder() const;
  void SetTorrentDownloadFallbackOnFolder(const bool enabled);
  bool GetTorrentDownloadCreateSubfolder() const;
  void SetTorrentDownloadCreateSubfolder(const bool enabled);
  std::wstring GetTorrentDownloadSortBy() const;
  void SetTorrentDownloadSortBy(const std::wstring& sort);
  std::wstring GetTorrentDownloadSortOrder() const;
  void SetTorrentDownloadSortOrder(const std::wstring& order);
  bool GetTorrentDownloadUseMagnet() const;
  void SetTorrentDownloadUseMagnet(const bool enabled);
  bool GetTorrentFilterEnabled() const;
  void SetTorrentFilterEnabled(const bool enabled);
  int GetTorrentFilterArchiveMaxCount() const;
  void SetTorrentFilterArchiveMaxCount(const int count);

  // Internal
  int GetAppPositionX() const;
  void SetAppPositionX(const int x);
  int GetAppPositionY() const;
  void SetAppPositionY(const int y);
  int GetAppPositionW() const;
  void SetAppPositionW(const int width);
  int GetAppPositionH() const;
  void SetAppPositionH(const int height);
  bool GetAppPositionMaximized() const;
  void SetAppPositionMaximized(const bool enabled);
  bool GetAppPositionRemember() const;
  void SetAppPositionRemember(const bool enabled);
  bool GetAppOptionHideSidebar() const;
  void SetAppOptionHideSidebar(const bool enabled);
  bool GetAppOptionEnableRecognition() const;
  void SetAppOptionEnableRecognition(const bool enabled);
  bool GetAppOptionEnableSharing() const;
  void SetAppOptionEnableSharing(const bool enabled);
  bool GetAppOptionEnableSync() const;
  void SetAppOptionEnableSync(const bool enabled);
  std::wstring GetAppSeasonsLastSeason() const;
  void SetAppSeasonsLastSeason(const std::wstring& season);
  int GetAppSeasonsGroupBy() const;
  void SetAppSeasonsGroupBy(const int group_by);
  int GetAppSeasonsSortBy() const;
  void SetAppSeasonsSortBy(const int sort_by);
  int GetAppSeasonsViewAs() const;
  void SetAppSeasonsViewAs(const int view_as);

  std::vector<std::wstring> GetLibraryFolders() const;
  void SetLibraryFolders(const std::vector<std::wstring>& folders);

  bool GetMediaPlayerEnabled(const std::wstring& player) const;
  void SetMediaPlayerEnabled(const std::wstring& player, const bool enabled);

  std::optional<AnimeListColumn> GetAnimeListColumn(const std::wstring& key) const;
  void SetAnimeListColumn(const std::wstring& key, const AnimeListColumn& column);

  std::wstring GetAnimeFolder(const int id) const;
  void SetAnimeFolder(const int id, const std::wstring& folder);
  bool GetAnimeUseAlternative(const int id) const;
  void SetAnimeUseAlternative(const int id, const bool enabled);
  std::vector<std::wstring> GetAnimeUserSynonyms(const int id) const;
  void SetAnimeUserSynonyms(const int id, const std::vector<std::wstring>& synonyms);

private:
  struct AppSetting {
    base::Settings::key_t key;
    base::Settings::value_t default_value;
  };

  template <typename T>
  T value(const AppSettingKey key) const;

  template <typename T>
  bool set_value(const AppSettingKey key, T&& value);

  void InitKeyMap() const;

  bool DeserializeFromXml(const std::wstring& path);
  bool SerializeToXml(const std::wstring& path) const;

  std::vector<std::wstring> library_folders_;
  std::map<std::wstring, bool> media_players_enabled_;
  std::map<std::wstring, AnimeListColumn> anime_list_columns_;
  std::map<int, AnimeSettings> anime_settings_;

  mutable std::map<AppSettingKey, AppSetting> key_map_;
  mutable std::mutex mutex_;

  bool changed_account_or_service_ = false;
  bool modified_ = false;

  base::Settings settings_;
};

std::wstring GetCurrentUserDisplayName();
std::wstring GetCurrentUsername();

inline Settings settings;

}  // namespace taiga
