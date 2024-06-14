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

namespace taiga {

enum class AppSettingKey {
  // Meta
  MetaVersion,

  // Services
  SyncActiveService,
  SyncAutoOnStart,
  SyncServiceMalUsername,
  SyncServiceMalAccessToken,
  SyncServiceMalRefreshToken,
  SyncServiceKitsuDisplayName,
  SyncServiceKitsuEmail,
  SyncServiceKitsuUsername,
  SyncServiceKitsuPassword,
  SyncServiceKitsuPartialLibrary,
  SyncServiceKitsuRatingSystem,
  SyncServiceAniListUsername,
  SyncServiceAniListRatingSystem,
  SyncServiceAniListToken,

  // Library
  LibraryFileSizeThreshold,
  LibraryMediaPlayerPath,
  LibraryWatchFolders,

  // Application
  AppListDoubleClickAction,
  AppListMiddleClickAction,
  AppListDisplayEnglishTitles,
  AppListHighlightNewEpisodes,
  AppListDisplayHighlightedOnTop,
  AppListProgressDisplayAired,
  AppListProgressDisplayAvailable,
  AppListSortColumnPrimary,
  AppListSortColumnSecondary,
  AppListSortOrderPrimary,
  AppListSortOrderSecondary,
  AppListTitleLanguagePreference,
  AppBehaviorAutostart,
  AppBehaviorStartMinimized,
  AppBehaviorCheckForUpdates,
  AppBehaviorScanAvailableEpisodes,
  AppBehaviorCloseToTray,
  AppBehaviorMinimizeToTray,
  AppConnectionProxyHost,
  AppConnectionProxyUsername,
  AppConnectionProxyPassword,
  AppConnectionNoRevoke,
  AppConnectionReuseActive,
  AppInterfaceTheme,
  AppInterfaceExternalLinks,

  // Recognition
  RecognitionDetectionInterval,
  RecognitionDetectMediaPlayers,
  RecognitionDetectStreamingMedia,
  RecognitionIgnoredStrings,
  RecognitionLookupParentDirectories,
  RecognitionRelationsLastModified,
  SyncUpdateDelay,
  SyncUpdateAskToConfirm,
  SyncUpdateCheckPlayer,
  SyncUpdateOutOfRange,
  SyncUpdateOutOfRoot,
  SyncUpdateWaitPlayer,
  SyncGoToNowPlayingRecognized,
  SyncGoToNowPlayingNotRecognized,
  SyncNotifyRecognized,
  SyncNotifyNotRecognized,
  SyncNotifyFormat,
  StreamAnimelab,
  StreamAdn,
  StreamAnn,
  StreamBilibili,
  StreamCrunchyroll,
  StreamFunimation,
  StreamHidive,
  StreamJellyfin,
  StreamPlex,
  StreamRokuChannel,
  StreamTubi,
  StreamVeoh,
  StreamViz,
  StreamVrv,
  StreamWakanim,
  StreamYahoo,
  StreamYoutube,

  // Sharing
  ShareDiscordApplicationId,
  ShareDiscordEnabled,
  ShareDiscordGroupEnabled,
  ShareDiscordTimeEnabled,
  ShareDiscordUsernameEnabled,
  ShareHttpEnabled,
  ShareHttpFormat,
  ShareHttpUrl,
  ShareMircEnabled,
  ShareMircMultiServer,
  ShareMircUseMeAction,
  ShareMircMode,
  ShareMircChannels,
  ShareMircFormat,
  ShareMircService,
  ShareTwitterEnabled,
  ShareTwitterFormat,
  ShareTwitterOauthToken,
  ShareTwitterOauthSecret,
  ShareTwitterReplyTo,
  ShareTwitterUsername,

  // Torrents
  TorrentDiscoverySource,
  TorrentDiscoverySearchUrl,
  TorrentDiscoveryAutoCheckEnabled,
  TorrentDiscoveryAutoCheckInterval,
  TorrentDiscoveryNewAction,
  TorrentDownloadAppMode,
  TorrentDownloadAppOpen,
  TorrentDownloadAppPath,
  TorrentDownloadLocation,
  TorrentDownloadFileLocation,
  TorrentDownloadUseAnimeFolder,
  TorrentDownloadFallbackOnFolder,
  TorrentDownloadCreateSubfolder,
  TorrentDownloadSortBy,
  TorrentDownloadSortOrder,
  TorrentDownloadUseMagnet,
  TorrentFilterEnabled,
  TorrentFilterArchiveMaxCount,

  // Internal
  AppPositionX,
  AppPositionY,
  AppPositionW,
  AppPositionH,
  AppPositionMaximized,
  AppPositionRemember,
  AppOptionHideSidebar,
  AppOptionEnableRecognition,
  AppOptionEnableSharing,
  AppOptionEnableSync,
  AppSeasonsLastSeason,
  AppSeasonsGroupBy,
  AppSeasonsSortBy,
  AppSeasonsViewAs,
};

}  // namespace taiga
