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

#include "settings.h"

#include "dlg/dlg_anime_list.h"
#include "dlg/dlg_main.h"
#include "dlg/dlg_history.h"
#include "dlg/dlg_search.h"
#include "dlg/dlg_settings.h"
#include "dlg/dlg_stats.h"

#include "anime.h"
#include "anime_db.h"
#include "anime_filter.h"
#include "common.h"
#include "foreach.h"
#include "gfx.h"
#include "history.h"
#include "http.h"
#include "media.h"
#include "monitor.h"
#include "stats.h"
#include "string.h"
#include "taiga.h"
#include "theme.h"
#include "xml.h"

#include "win32/win_registry.h"

#define DEFAULT_EXTERNALLINKS    L"Anime Recommendation Finder|http://www.animerecs.com\r\nMALgraph|http://mal.oko.im\r\n-\r\nAnime Season Discussion Group|http://myanimelist.net/clubs.php?cid=743\r\nMahou Showtime Schedule|http://www.mahou.org/Showtime/?o=ET#Current\r\nThe Fansub Wiki|http://www.fansubwiki.com"
#define DEFAULT_FORMAT_HTTP      L"user=%user%&name=%title%&ep=%episode%&eptotal=$if(%total%,%total%,?)&score=%score%&picurl=%image%&playstatus=%playstatus%"
#define DEFAULT_FORMAT_MESSENGER L"Watching: %title%$if(%episode%, #%episode%$if(%total%,/%total%)) ~ www.myanimelist.net/anime/%id%"
#define DEFAULT_FORMAT_MIRC      L"\00304$if($greater(%episode%,%watched%),Watching,Re-watching):\003 %title%$if(%episode%, \00303%episode%$if(%total%,/%total%))\003 $if(%score%,\00314[Score: %score%/10]\003) \00312www.myanimelist.net/anime/%id%"
#define DEFAULT_FORMAT_SKYPE     L"Watching: <a href=\"http://myanimelist.net/anime/%id%\">%title%</a>$if(%episode%, #%episode%$if(%total%,/%total%))"
#define DEFAULT_FORMAT_TWITTER   L"$ifequal(%episode%,%total%,Just completed: %title%$if(%score%, (Score: %score%/10)) www.myanimelist.net/anime/%id%)"
#define DEFAULT_FORMAT_BALLOON   L"$if(%title%,%title%)\\n$if(%name%,%name%)\\n$if(%episode%,Episode %episode%$if(%total%,/%total%) )$if(%group%,by %group%)"
#define DEFAULT_TORRENT_APPPATH  L"C:\\Program Files\\uTorrent\\uTorrent.exe"
#define DEFAULT_TORRENT_SEARCH   L"http://www.nyaa.eu/?page=rss&cats=1_37&filter=2&term=%title%"
#define DEFAULT_TORRENT_SOURCE   L"http://tokyotosho.info/rss.php?filter=1,11&zwnj=0"

class Settings Settings;

// =============================================================================

bool Settings::Load() {
  // Initialize
  folder_ = Taiga.GetDataPath();
  file_ = folder_ + L"settings.xml";
  CreateDirectory(folder_.c_str(), NULL);
  
  // Load XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file_.c_str());
  
  // Read settings
  xml_node settings = doc.child(L"settings");
  
  // Account
  xml_node account = settings.child(L"account");
    // MyAnimeList
    xml_node mal = account.child(L"myanimelist");
    Account.MAL.auto_sync = mal.attribute(L"login").as_int();
    Account.MAL.password = SimpleDecrypt(mal.attribute(L"password").value());
    Account.MAL.user = mal.attribute(L"username").value();
    // Update
    xml_node update = account.child(L"update");
    Account.Update.ask_to_confirm = update.attribute(L"asktoconfirm").as_int(TRUE);
    Account.Update.check_player = update.attribute(L"checkplayer").as_int();
    Account.Update.delay = update.attribute(L"delay").as_int(120);
    Account.Update.go_to_nowplaying = update.attribute(L"gotonowplaying").as_int(TRUE);
    Account.Update.out_of_range = update.attribute(L"outofrange").as_int();
    Account.Update.out_of_root = update.attribute(L"outofroot").as_int();
    Account.Update.wait_mp = update.attribute(L"waitplayer").as_int();

  // Announcements
  xml_node announce = settings.child(L"announce");
    // HTTP
    xml_node http = announce.child(L"http");
    Announce.HTTP.enabled = http.attribute(L"enabled").as_int();
    Announce.HTTP.format = http.attribute(L"format").as_string(DEFAULT_FORMAT_HTTP);
    Announce.HTTP.url = http.attribute(L"url").value();
    // MSN
    xml_node messenger = announce.child(L"messenger");
    Announce.MSN.enabled = messenger.attribute(L"enabled").as_int();
    Announce.MSN.format = messenger.attribute(L"format").as_string(DEFAULT_FORMAT_MESSENGER);
    // mIRC
    xml_node mirc = announce.child(L"mirc");
    Announce.MIRC.channels = mirc.attribute(L"channels").as_string(L"#myanimelist, #taiga");
    Announce.MIRC.enabled = mirc.attribute(L"enabled").as_int();
    Announce.MIRC.format = mirc.attribute(L"format").as_string(DEFAULT_FORMAT_MIRC);
    Announce.MIRC.mode = mirc.attribute(L"mode").as_int(1);
    Announce.MIRC.multi_server = mirc.attribute(L"multiserver").as_int(FALSE);
    Announce.MIRC.service = mirc.attribute(L"service").as_string(L"mIRC");
    Announce.MIRC.use_action = mirc.attribute(L"useaction").as_int(TRUE);
    // Skype
    xml_node skype = announce.child(L"skype");
    Announce.Skype.enabled = skype.attribute(L"enabled").as_int();
    Announce.Skype.format = skype.attribute(L"format").as_string(DEFAULT_FORMAT_SKYPE);
    // Twitter
    xml_node twitter = announce.child(L"twitter");
    Announce.Twitter.enabled = twitter.attribute(L"enabled").as_int();
    Announce.Twitter.format = twitter.attribute(L"format").as_string(DEFAULT_FORMAT_TWITTER);
    Announce.Twitter.oauth_key = twitter.attribute(L"oauth_token").value();
    Announce.Twitter.oauth_secret = twitter.attribute(L"oauth_secret").value();
    Announce.Twitter.user = twitter.attribute(L"user").value();

  // Folders
  Folders.root.clear();
  xml_node folders = settings.child(L"anime").child(L"folders");
  for (xml_node folder = folders.child(L"root"); folder; folder = folder.next_sibling(L"root")) {
    Folders.root.push_back(folder.attribute(L"folder").value());
  }
  xml_node watch = folders.child(L"watch");
  Folders.watch_enabled = watch.attribute(L"enabled").as_int(TRUE);

  // Anime items
  Anime.items.clear();
  wstring folder, titles;
  xml_node items = settings.child(L"anime").child(L"items");
  for (xml_node item = items.child(L"item"); item; item = item.next_sibling(L"item")) {
    folder = item.attribute(L"folder").value();
    titles = item.attribute(L"titles").value();
    Anime.SetItem(item.attribute(L"id").as_int(), 
      folder.empty() ? Optional<wstring>() : folder, 
      titles.empty() ? Optional<wstring>() : titles);
  }

  // Program
  xml_node program = settings.child(L"program");
    // General
    xml_node general = program.child(L"general");
    Program.General.auto_start = general.attribute(L"autostart").as_int();
    Program.General.close = general.attribute(L"close").as_int();
    Program.General.minimize = general.attribute(L"minimize").as_int();
    Program.General.theme = general.attribute(L"theme").as_string(L"Default");
    Program.General.external_links = general.attribute(L"externallinks").as_string(DEFAULT_EXTERNALLINKS);
    Program.General.hide_sidebar = general.attribute(L"hidesidebar").as_int();
    Program.General.enable_recognition = general.attribute(L"enablerecognition").as_int(TRUE);
    Program.General.enable_sharing = general.attribute(L"enablesharing").as_int(TRUE);
    Program.General.enable_sync = general.attribute(L"enablesync").as_int(TRUE);
    // Position
    xml_node position = program.child(L"position");
    Program.Position.x = position.attribute(L"x").as_int(-1);
    Program.Position.y = position.attribute(L"y").as_int(-1);
    Program.Position.w = position.attribute(L"w").as_int(960);
    Program.Position.h = position.attribute(L"h").as_int(640);
    Program.Position.maximized = position.attribute(L"maximized").as_int();
    // Start-up
    xml_node startup = program.child(L"startup");
    Program.StartUp.check_new_episodes = startup.attribute(L"checkeps").as_int();
    Program.StartUp.check_new_version = startup.attribute(L"checkversion").as_int(TRUE);
    Program.StartUp.minimize = startup.attribute(L"minimize").as_int();
    // Exit
    xml_node exit = program.child(L"exit");
    Program.Exit.remember_pos_size = exit.attribute(L"remember_pos_size").as_int(TRUE);
    // Proxy
    xml_node proxy = program.child(L"proxy");
    Program.Proxy.host = proxy.attribute(L"host").value();
    Program.Proxy.password = SimpleDecrypt(proxy.attribute(L"password").value());
    Program.Proxy.user = proxy.attribute(L"username").value();
    SetProxies(Program.Proxy.host, Program.Proxy.user, Program.Proxy.password);
    // List
    xml_node list = program.child(L"list");
    Program.List.double_click = list.child(L"action").attribute(L"doubleclick").as_int(4);
    Program.List.middle_click = list.child(L"action").attribute(L"middleclick").as_int(3);
    Program.List.english_titles = list.child(L"action").attribute(L"englishtitles").as_bool();
    Program.List.highlight = list.child(L"filter").child(L"episodes").attribute(L"highlight").as_int(TRUE);
    Program.List.progress_show_aired = list.child(L"progress").attribute(L"showaired").as_int(TRUE);
    Program.List.progress_show_available = list.child(L"progress").attribute(L"showavailable").as_int(TRUE);
    Program.List.progress_show_eps = list.child(L"progress").attribute(L"showeps").as_int(TRUE);
    // Notifications
    xml_node notifications = program.child(L"notifications");
    Program.Notifications.recognized = notifications.child(L"balloon").attribute(L"recognized").as_int(TRUE);
    Program.Notifications.notrecognized = notifications.child(L"balloon").attribute(L"notrecognized").as_int(TRUE);
    Program.Notifications.format = notifications.child(L"balloon").attribute(L"format").as_string(DEFAULT_FORMAT_BALLOON);

  // Recognition
  xml_node recognition = settings.child(L"recognition");
    // Media players
    xml_node mediaplayers = recognition.child(L"mediaplayers");
      for (xml_node player = mediaplayers.child(L"player"); player; player = player.next_sibling(L"player")) {
        wstring name = player.attribute(L"name").value();
        bool enabled = player.attribute(L"enabled").as_bool();
        foreach_(it, MediaPlayers.items) {
          if (it->name == name) {
            it->enabled = enabled;
            break;
          }
        }
      }
    // Streaming
    xml_node streaming = recognition.child(L"streaming");
      Recognition.Streaming.ann_enabled = streaming.child(L"providers").attribute(L"ann").as_bool();
      Recognition.Streaming.crunchyroll_enabled = streaming.child(L"providers").attribute(L"crunchyroll").as_bool();
      Recognition.Streaming.veoh_enabled = streaming.child(L"providers").attribute(L"veoh").as_bool();
      Recognition.Streaming.viz_enabled = streaming.child(L"providers").attribute(L"viz").as_bool();
      Recognition.Streaming.youtube_enabled = streaming.child(L"providers").attribute(L"youtube").as_bool();

  // RSS
  xml_node rss = settings.child(L"rss");
    // Torrent  
    xml_node torrent = rss.child(L"torrent");
      // General
      RSS.Torrent.app_mode = torrent.child(L"application").attribute(L"mode").as_int(1);
      RSS.Torrent.app_path = torrent.child(L"application").attribute(L"path").value();
      if (RSS.Torrent.app_path.empty()) {
        RSS.Torrent.app_path = GetDefaultAppPath(L".torrent", DEFAULT_TORRENT_APPPATH);
      }
      RSS.Torrent.check_enabled = torrent.child(L"options").attribute(L"autocheck").as_int(TRUE);
      RSS.Torrent.check_interval = torrent.child(L"options").attribute(L"checkinterval").as_int(60);
      RSS.Torrent.create_folder = torrent.child(L"options").attribute(L"autocreatefolder").as_int(FALSE);
      RSS.Torrent.download_path = torrent.child(L"options").attribute(L"downloadpath").as_string();
      RSS.Torrent.hide_unidentified = torrent.child(L"options").attribute(L"hideunidentified").as_int(FALSE);
      RSS.Torrent.new_action = torrent.child(L"options").attribute(L"newaction").as_int(1);
      RSS.Torrent.set_folder = torrent.child(L"options").attribute(L"autosetfolder").as_int(TRUE);
      RSS.Torrent.search_url = torrent.child(L"search").attribute(L"address").as_string(DEFAULT_TORRENT_SEARCH);
      RSS.Torrent.source = torrent.child(L"source").attribute(L"address").as_string(DEFAULT_TORRENT_SOURCE);
      // Filters
      xml_node filter = torrent.child(L"filter");
      RSS.Torrent.Filters.global_enabled = filter.attribute(L"enabled").as_int(TRUE);
      Aggregator.filter_manager.filters.clear();
      for (xml_node item = filter.child(L"item"); item; item = item.next_sibling(L"item")) {
        Aggregator.filter_manager.AddFilter(
          item.attribute(L"action").as_int(), 
          item.attribute(L"match").as_int(), 
          item.attribute(L"enabled").as_bool(), 
          item.attribute(L"name").value());
        for (xml_node anime = item.child(L"anime"); anime; anime = anime.next_sibling(L"anime")) {
          Aggregator.filter_manager.filters.back().anime_ids.push_back(anime.attribute(L"id").as_int());
        }
        for (xml_node condition = item.child(L"condition"); condition; condition = condition.next_sibling(L"condition")) {
          Aggregator.filter_manager.filters.back().AddCondition(
            condition.attribute(L"element").as_int(), 
            condition.attribute(L"op").as_int(), 
            condition.attribute(L"value").value());
        }
      }
      if (Aggregator.filter_manager.filters.empty()) {
        Aggregator.filter_manager.AddPresets();
      }
      // Torrent source
      Feed* feed = Aggregator.Get(FEED_CATEGORY_LINK);
      if (feed) feed->link = RSS.Torrent.source;
      // File archive
      Aggregator.file_archive.clear();
      PopulateFiles(Aggregator.file_archive, Taiga.GetDataPath() + L"feed\\", L"torrent", true, true);

  return result.status == status_ok;
}

// =============================================================================

bool Settings::Save() {
  // Initialize
  xml_document doc;
  xml_node settings = doc.append_child(L"settings");

  // Account
  settings.append_child(node_comment).set_value(L" Account ");
  xml_node account = settings.append_child(L"account");
    // MyAnimeList
    xml_node mal = account.append_child(L"myanimelist");
    mal.append_attribute(L"username") = Account.MAL.user.c_str();
    mal.append_attribute(L"password") = SimpleEncrypt(Account.MAL.password).c_str();
    mal.append_attribute(L"login") = Account.MAL.auto_sync;
    // Update
    xml_node update = account.append_child(L"update");
    update.append_attribute(L"asktoconfirm") = Account.Update.ask_to_confirm;
    update.append_attribute(L"delay") = Account.Update.delay;
    update.append_attribute(L"checkplayer") = Account.Update.check_player;
    update.append_attribute(L"gotonowplaying") = Account.Update.go_to_nowplaying;
    update.append_attribute(L"outofrange") = Account.Update.out_of_range;
    update.append_attribute(L"outofroot") = Account.Update.out_of_root;
    update.append_attribute(L"waitplayer") = Account.Update.wait_mp;
  
  // Anime
  settings.append_child(node_comment).set_value(L" Anime list ");
  xml_node anime = settings.append_child(L"anime");
    // Root folders  
    xml_node folders = anime.append_child(L"folders");  
    for (size_t i = 0; i < Folders.root.size(); i++) {
      xml_node root = folders.append_child(L"root");
      root.append_attribute(L"folder") = Folders.root[i].c_str();
    }
    xml_node watch = folders.append_child(L"watch");
    watch.append_attribute(L"enabled") = Folders.watch_enabled;
    // Items
    xml_node items = anime.append_child(L"items");
    for (size_t i = 0; i < Anime.items.size(); i++) {
      if (Anime.items[i].folder.empty() && Anime.items[i].titles.empty()) continue;
      xml_node item = items.append_child(L"item");
      item.append_attribute(L"id") = Anime.items[i].id;
      if (!Anime.items[i].folder.empty())
        item.append_attribute(L"folder") = Anime.items[i].folder.c_str();
      if (!Anime.items[i].titles.empty())
        item.append_attribute(L"titles") = Anime.items[i].titles.c_str();
    }

  // Announcements
  settings.append_child(node_comment).set_value(L" Announcements ");
  xml_node announce = settings.append_child(L"announce");
    // HTTP
    xml_node http = announce.append_child(L"http");
    http.append_attribute(L"enabled") = Announce.HTTP.enabled;
    http.append_attribute(L"format") = Announce.HTTP.format.c_str();
    http.append_attribute(L"url") = Announce.HTTP.url.c_str();
    // Messenger
    settings.child(L"announce").append_child(L"messenger");
    settings.child(L"announce").child(L"messenger").append_attribute(L"enabled") = Announce.MSN.enabled;
    settings.child(L"announce").child(L"messenger").append_attribute(L"format") = Announce.MSN.format.c_str();
    // mIRC
    xml_node mirc = announce.append_child(L"mirc");
    mirc.append_attribute(L"enabled") = Announce.MIRC.enabled;
    mirc.append_attribute(L"format") = Announce.MIRC.format.c_str();
    mirc.append_attribute(L"service") = Announce.MIRC.service.c_str();
    mirc.append_attribute(L"mode") = Announce.MIRC.mode;
    mirc.append_attribute(L"useaction") = Announce.MIRC.use_action;
    mirc.append_attribute(L"multiserver") = Announce.MIRC.multi_server;
    mirc.append_attribute(L"channels") = Announce.MIRC.channels.c_str();
    // Skype
    xml_node skype = announce.append_child(L"skype");
    skype.append_attribute(L"enabled") = Announce.Skype.enabled;
    skype.append_attribute(L"format") = Announce.Skype.format.c_str();
    // Twitter
    xml_node twitter = announce.append_child(L"twitter");
    twitter.append_attribute(L"enabled") = Announce.Twitter.enabled;
    twitter.append_attribute(L"format") = Announce.Twitter.format.c_str();
    twitter.append_attribute(L"oauth_token") = Announce.Twitter.oauth_key.c_str();
    twitter.append_attribute(L"oauth_secret") = Announce.Twitter.oauth_secret.c_str();
    twitter.append_attribute(L"user") = Announce.Twitter.user.c_str();

  // Program
  settings.append_child(node_comment).set_value(L" Program ");
  xml_node program = settings.append_child(L"program");
    // General
    xml_node general = program.append_child(L"general");
    general.append_attribute(L"autostart") = Program.General.auto_start;
    general.append_attribute(L"close") = Program.General.close;
    general.append_attribute(L"minimize") = Program.General.minimize;
    general.append_attribute(L"theme") = Program.General.theme.c_str();
    general.append_attribute(L"externallinks") = Program.General.external_links.c_str();
    general.append_attribute(L"hidesidebar") = Program.General.hide_sidebar;
    general.append_attribute(L"enablerecognition") = Program.General.enable_recognition;
    general.append_attribute(L"enablesharing") = Program.General.enable_sharing;
    general.append_attribute(L"enablesync") = Program.General.enable_sync;
    // Position
    xml_node position = program.append_child(L"position");
    position.append_attribute(L"x") = Program.Position.x;
    position.append_attribute(L"y") = Program.Position.y;
    position.append_attribute(L"w") = Program.Position.w;
    position.append_attribute(L"h") = Program.Position.h;
    position.append_attribute(L"maximized") = Program.Position.maximized;
    // Startup
    xml_node startup = program.append_child(L"startup");
    startup.append_attribute(L"checkversion") = Program.StartUp.check_new_version;
    startup.append_attribute(L"checkeps") = Program.StartUp.check_new_episodes;
    startup.append_attribute(L"minimize") = Program.StartUp.minimize;
    // Exit
    xml_node exit = program.append_child(L"exit");
    exit.append_attribute(L"remember_pos_size") = Program.Exit.remember_pos_size;
    // Proxy
    xml_node proxy = program.append_child(L"proxy");
    proxy.append_attribute(L"host") = Program.Proxy.host.c_str();
    proxy.append_attribute(L"username") = Program.Proxy.user.c_str();
    proxy.append_attribute(L"password") = SimpleEncrypt(Program.Proxy.password).c_str();
    // List
    xml_node list = program.append_child(L"list");
      // Actions  
      xml_node action = list.append_child(L"action");
      action.append_attribute(L"doubleclick") = Program.List.double_click;
      action.append_attribute(L"middleclick") = Program.List.middle_click;
      action.append_attribute(L"englishtitles") = Program.List.english_titles;
      // Filter
      xml_node filter = list.append_child(L"filter");
      filter.append_child(L"episodes");
      filter.child(L"episodes").append_attribute(L"highlight") = Program.List.highlight;
      // Progress
      xml_node progress = list.append_child(L"progress");
      progress.append_attribute(L"showaired") = Program.List.progress_show_aired;
      progress.append_attribute(L"showavailable") = Program.List.progress_show_available;
      progress.append_attribute(L"showeps") = Program.List.progress_show_eps;
    // Notifications
    xml_node notifications = program.append_child(L"notifications");
    notifications.append_child(L"balloon");
    notifications.child(L"balloon").append_attribute(L"recognized") = Program.Notifications.recognized;
    notifications.child(L"balloon").append_attribute(L"notrecognized") = Program.Notifications.notrecognized;
    notifications.child(L"balloon").append_attribute(L"format") = Program.Notifications.format.c_str();

  // Recognition
  settings.append_child(node_comment).set_value(L" Recognition ");
  xml_node recognition = settings.append_child(L"recognition");
    // Media players
    xml_node mediaplayers = recognition.append_child(L"mediaplayers");
      foreach_(it, MediaPlayers.items) {
        xml_node player = mediaplayers.append_child(L"player");
        player.append_attribute(L"name") = it->name.c_str();
        player.append_attribute(L"enabled") = it->enabled;
      }
    // Streaming
    xml_node streaming = recognition.append_child(L"streaming");
      // Providers
      xml_node providers = streaming.append_child(L"providers");
        providers.append_attribute(L"ann") = Recognition.Streaming.ann_enabled;
        providers.append_attribute(L"crunchyroll") = Recognition.Streaming.crunchyroll_enabled;
        providers.append_attribute(L"veoh") = Recognition.Streaming.veoh_enabled;
        providers.append_attribute(L"viz") = Recognition.Streaming.viz_enabled;
        providers.append_attribute(L"youtube") = Recognition.Streaming.youtube_enabled;

  // RSS
  settings.append_child(node_comment).set_value(L" RSS ");
  xml_node rss = settings.append_child(L"rss");
    // Torrent
    xml_node torrent = rss.append_child(L"torrent");
      // General
      torrent.append_child(L"application");
      torrent.child(L"application").append_attribute(L"mode") = RSS.Torrent.app_mode;
      torrent.child(L"application").append_attribute(L"path") = RSS.Torrent.app_path.c_str();
      torrent.append_child(L"search");
      torrent.child(L"search").append_attribute(L"address") = RSS.Torrent.search_url.c_str();
      torrent.append_child(L"source");
      torrent.child(L"source").append_attribute(L"address") = RSS.Torrent.source.c_str();
      torrent.append_child(L"options");
      torrent.child(L"options").append_attribute(L"autocheck") = RSS.Torrent.check_enabled;
      torrent.child(L"options").append_attribute(L"checkinterval") = RSS.Torrent.check_interval;
      torrent.child(L"options").append_attribute(L"autosetfolder") = RSS.Torrent.set_folder;
      torrent.child(L"options").append_attribute(L"autocreatefolder") = RSS.Torrent.create_folder;
      torrent.child(L"options").append_attribute(L"downloadpath") = RSS.Torrent.download_path.c_str();
      torrent.child(L"options").append_attribute(L"hideunidentified") = RSS.Torrent.hide_unidentified;
      torrent.child(L"options").append_attribute(L"newaction") = RSS.Torrent.new_action;
      // Filter
      xml_node torrent_filter = torrent.append_child(L"filter");
      torrent_filter.append_attribute(L"enabled") = RSS.Torrent.Filters.global_enabled;
      for (auto it = Aggregator.filter_manager.filters.begin(); it != Aggregator.filter_manager.filters.end(); ++it) {
        xml_node item = torrent_filter.append_child(L"item");
        item.append_attribute(L"action") = it->action;
        item.append_attribute(L"match") = it->match;
        item.append_attribute(L"enabled") = it->enabled;
        item.append_attribute(L"name") = it->name.c_str();
        for (auto ita = it->anime_ids.begin(); ita != it->anime_ids.end(); ++ita) {
          xml_node anime = item.append_child(L"anime");
          anime.append_attribute(L"id") = *ita;
        }
        for (auto itc = it->conditions.begin(); itc != it->conditions.end(); ++itc) {
          xml_node condition = item.append_child(L"condition");
          condition.append_attribute(L"element") = itc->element;
          condition.append_attribute(L"op") = itc->op;
          condition.append_attribute(L"value") = itc->value.c_str();
        }
      }
  
  // Write registry
  win32::Registry reg;
  reg.OpenKey(HKEY_CURRENT_USER, 
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE);
  if (Program.General.auto_start) {
    wstring app_path = Taiga.GetModulePath();
    reg.SetValue(APP_NAME, app_path.c_str());
  } else {
    reg.DeleteValue(APP_NAME);
  }
  reg.CloseKey();

  // Save file
  ::CreateDirectory(folder_.c_str(), NULL);
  return doc.save_file(file_.c_str(), L"\x09", format_default | format_write_bom);
}

void Settings::ApplyChanges(const wstring& previous_user, const wstring& previous_theme) {
  if (Program.General.theme != previous_theme) {
    UI.Load(Program.General.theme);
    UI.LoadImages();
    MainDialog.rebar.RedrawWindow();
    UpdateAllMenus();
  }

  if (Account.MAL.user != previous_user) {
    AnimeDatabase.LoadList();
    History.Load();
    CurrentEpisode.Set(anime::ID_UNKNOWN);
    MainDialog.treeview.RefreshHistoryCounter();
    MainDialog.UpdateTitle();
    AnimeListDialog.RefreshList(mal::MYSTATUS_WATCHING);
    AnimeListDialog.RefreshTabs(mal::MYSTATUS_WATCHING);
    HistoryDialog.RefreshList();
    SearchDialog.RefreshList();
    Stats.CalculateAll();
    StatsDialog.Refresh();
    ExecuteAction(L"Logout(" + previous_user + L")");
  } else {
    AnimeListDialog.RefreshList();
  }

  FolderMonitor.Enable(Folders.watch_enabled == TRUE);

  SetProxies(Program.Proxy.host, 
             Program.Proxy.user, 
             Program.Proxy.password);

  UpdateExternalLinksMenu();
}

void Settings::RestoreDefaults() {
  // Take a backup
  wstring backup = file_ + L".bak";
  DWORD flags = MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH;
  MoveFileEx(file_.c_str(), backup.c_str(), flags);
  
  // Reload settings
  wstring previous_user = Account.MAL.user;
  wstring previous_theme = Program.General.theme;
  Load();
  ApplyChanges(previous_user, previous_theme);

  // Reload settings dialog
  if (SettingsDialog.IsWindow()) {
    SettingsDialog.Destroy();
    ExecuteAction(L"Settings");
  }
}

// =============================================================================

void Settings::Anime::SetItem(int id, Optional<wstring> folder, Optional<wstring> titles) {
  int index = -1;
  for (size_t i = 0; i < items.size(); i++) {
    if (items[i].id == id) {
      index = static_cast<int>(i);
      break;
    }
  }
  if (index == -1) {
    index = items.size();
    items.resize(index + 1);
  }
  items[index].id = id;
  if (folder) items[index].folder = *folder;
  if (titles) items[index].titles = *titles;
}