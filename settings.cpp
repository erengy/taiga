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
#include "animelist.h"
#include "common.h"
#include "event.h"
#include "gfx.h"
#include "http.h"
#include "settings.h"
#include "string.h"
#include "taiga.h"
#include "win32/win_registry.h"
#include "xml.h"

#define DEFAULT_FORMAT_HTTP      L"user=%user%&name=%title%&ep=%episode%&eptotal=$if(%total%,%total%,?)&score=%score%&picurl=%image%&playstatus=%playstatus%"
#define DEFAULT_FORMAT_MESSENGER L"Watching: %title%$if(%episode%, #%episode%$if(%total%,/%total%)) ~ www.myanimelist.net/anime/%id%"
#define DEFAULT_FORMAT_MIRC      L"\00304$if($greater(%episode%,%watched%),Watching,Re-watching):\003 %title%$if(%episode%, \00303%episode%$if(%total%,/%total%))\003 $if(%score%,\00314[Score: %score%/10]\003) \00312www.myanimelist.net/anime/%id%"
#define DEFAULT_FORMAT_SKYPE     L"Watching: <a href=\"http://myanimelist.net/anime/%id%\">%title%</a>$if(%episode%, #%episode%$if(%total%,/%total%))"
#define DEFAULT_FORMAT_TWITTER   L"$ifequal(%episode%,%total%,Just completed: %title%$if(%score%, (Score: %score%/10)) www.myanimelist.net/anime/%id%)"
#define DEFAULT_FORMAT_BALLOON   L"$if(%title%,Title: \\t%title%)\\n$if(%name%,Name: \\t%name%)\\n$if(%episode%,Episode: \\t%episode%$if(%total%,/%total%))\\n$if(%group%,Group: \\t%group%)\\n$if(%resolution%,Res.: \\t%resolution%)\\n$if(%video%,Video: \\t%video%)\\n$if(%audio%,Audio: \\t%audio%)\\n$if(%extra%,Extra: \\t%extra%)"
#define DEFAULT_TORRENT_APPPATH  L"C:\\Program Files\\uTorrent\\uTorrent.exe"
#define DEFAULT_TORRENT_SOURCE   L"http://tokyotosho.info/rss.php?filter=1,11&zwnj=0"

class Settings Settings;

// =============================================================================

bool Settings::Read() {
  // Initialize
  folder_ = Taiga.GetDataPath();
  file_ = folder_ + L"Settings.xml";
  CreateDirectory(folder_.c_str(), NULL);
  
  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(file_.c_str());
  
  // Read settings
  xml_node settings = doc.child(L"settings");
  
  // Account
  xml_node account = settings.child(L"account");
    // MyAnimeList
    xml_node mal = account.child(L"myanimelist");
    Account.MAL.api = mal.attribute(L"api").as_int(2);
    Account.MAL.auto_login = mal.attribute(L"login").as_int();
    Account.MAL.password = SimpleDecrypt(mal.attribute(L"password").value());
    Account.MAL.user = mal.attribute(L"username").value();
    // Update
    xml_node update = account.child(L"update");
    Account.Update.check_player = update.attribute(L"checkplayer").as_int(TRUE);
    Account.Update.delay = update.attribute(L"delay").as_int(60);
    Account.Update.mode = update.attribute(L"mode").as_int(3);
    Account.Update.out_of_range = update.attribute(L"outofrange").as_int();
    Account.Update.time = update.attribute(L"time").as_int(2);

  // Announcements
  xml_node announce = settings.child(L"announce");
    // HTTP
    xml_node http = announce.child(L"http");
    Announce.HTTP.enabled = http.attribute(L"enabled").as_int();
    Announce.HTTP.format = http.attribute(L"format").value(DEFAULT_FORMAT_HTTP);
    Announce.HTTP.url = http.attribute(L"url").value();
    // MSN
    xml_node messenger = announce.child(L"messenger");
    Announce.MSN.enabled = messenger.attribute(L"enabled").as_int();
    Announce.MSN.format = messenger.attribute(L"format").value(DEFAULT_FORMAT_MESSENGER);
    // mIRC
    xml_node mirc = announce.child(L"mirc");
    Announce.MIRC.channels = mirc.attribute(L"channels").value(L"#myanimelist, #taiga");
    Announce.MIRC.enabled = mirc.attribute(L"enabled").as_int();
    Announce.MIRC.format = mirc.attribute(L"format").value(DEFAULT_FORMAT_MIRC);
    Announce.MIRC.mode = mirc.attribute(L"mode").as_int(1);
    Announce.MIRC.multi_server = mirc.attribute(L"multiserver").as_int(FALSE);
    Announce.MIRC.service = mirc.attribute(L"service").value(L"mIRC");
    Announce.MIRC.use_action = mirc.attribute(L"useaction").as_int(TRUE);
    // Skype
    xml_node skype = announce.child(L"skype");
    Announce.Skype.enabled = skype.attribute(L"enabled").as_int();
    Announce.Skype.format = skype.attribute(L"format").value(DEFAULT_FORMAT_SKYPE);
    // Twitter
    xml_node twitter = announce.child(L"twitter");
    Announce.Twitter.enabled = twitter.attribute(L"enabled").as_int();
    Announce.Twitter.format = twitter.attribute(L"format").value(DEFAULT_FORMAT_TWITTER);
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
  Folders.watch_enabled = watch.attribute(L"enabled").as_int();

  // Anime items
  Anime.items.clear();
  wstring folder, titles;
  xml_node items = settings.child(L"anime").child(L"items");
  for (xml_node item = items.child(L"item"); item; item = item.next_sibling(L"item")) {
    folder = item.attribute(L"folder").value(EMPTY_STR);
    titles = item.attribute(L"titles").value(EMPTY_STR);
    Anime.SetItem(item.attribute(L"id").as_int(), folder, titles);
  }

  // Program
  xml_node program = settings.child(L"program");
    // General
    xml_node general = program.child(L"general");
    Program.General.auto_start = general.attribute(L"autostart").as_int();
    Program.General.close = general.attribute(L"close").as_int();
    Program.General.minimize = general.attribute(L"minimize").as_int();
    Program.General.search_index = general.attribute(L"searchindex").as_int();
    Program.General.size_x = general.attribute(L"sizex").as_int();
    Program.General.size_y = general.attribute(L"sizey").as_int();
    Program.General.theme = general.attribute(L"theme").value(L"Default");
    // Start-up
    xml_node startup = program.child(L"startup");
    Program.StartUp.check_new_episodes = startup.attribute(L"checkeps").as_int();
    Program.StartUp.check_new_version = startup.attribute(L"checkversion").as_int(TRUE);
    Program.StartUp.minimize = startup.attribute(L"minimize").as_int();
    // Exit
    xml_node exit = program.child(L"exit");
    Program.Exit.ask = exit.attribute(L"ask").as_int();
    Program.Exit.save_event_queue = exit.attribute(L"savebuffer").as_int(1);
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
    for (int i = 0; i < 3; i++) {
      wstring name = L"s" + ToWSTR(i + 1);
      AnimeList.filters.status[i] = list.child(L"filter").child(L"status").attribute(name.c_str()).as_bool(true);
    }
    for (int i = 0; i < 6; i++) {
      wstring name = L"t" + ToWSTR(i + 1);
      AnimeList.filters.type[i] = list.child(L"filter").child(L"type").attribute(name.c_str()).as_bool(true);
    }
    Program.List.highlight = list.child(L"filter").child(L"episodes").attribute(L"highlight").as_int(TRUE);
    AnimeList.filters.new_episodes = list.child(L"filter").child(L"episodes").attribute(L"new").as_bool();
    Program.List.progress_mode = list.child(L"progress").attribute(L"mode").as_int(1);
    Program.List.progress_show_eps = list.child(L"progress").attribute(L"showeps").as_int();
    // Notifications
    xml_node notifications = program.child(L"notifications");
    Program.Balloon.enabled = notifications.child(L"balloon").attribute(L"enabled").as_int(TRUE);
    Program.Balloon.format = notifications.child(L"balloon").attribute(L"format").value(DEFAULT_FORMAT_BALLOON);

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
      RSS.Torrent.hide_unidentified = torrent.child(L"options").attribute(L"hideunidentified").as_int(FALSE);
      RSS.Torrent.new_action = torrent.child(L"options").attribute(L"newaction").as_int(1);
      RSS.Torrent.set_folder = torrent.child(L"options").attribute(L"autosetfolder").as_int(TRUE);
      RSS.Torrent.source = torrent.child(L"source").attribute(L"address").value(DEFAULT_TORRENT_SOURCE);
      // Filters
      xml_node filter = torrent.child(L"filter");
      RSS.Torrent.Filters.global_enabled = filter.attribute(L"enabled").as_int(TRUE);
      Aggregator.FilterManager.Filters.clear();
      for (xml_node item = filter.child(L"item"); item; item = item.next_sibling(L"item")) {
        Aggregator.FilterManager.AddFilter(
          item.attribute(L"action").as_int(), 
          item.attribute(L"match").as_int(), 
          item.attribute(L"enabled").as_bool(), 
          item.attribute(L"name").value());
        for (xml_node anime = item.child(L"anime"); anime; anime = anime.next_sibling(L"anime")) {
          Aggregator.FilterManager.Filters.back().AnimeIds.push_back(anime.attribute(L"id").as_int());
        }
        for (xml_node condition = item.child(L"condition"); condition; condition = condition.next_sibling(L"condition")) {
          Aggregator.FilterManager.Filters.back().AddCondition(
            condition.attribute(L"element").as_int(), 
            condition.attribute(L"op").as_int(), 
            condition.attribute(L"value").value());
        }
      }
      if (Aggregator.FilterManager.Filters.empty()) {
        Aggregator.FilterManager.AddPresets();
      }
      // Torrent source
      Feed* feed = Aggregator.Get(FEED_CATEGORY_LINK);
      if (feed) feed->Link = RSS.Torrent.source;
      // File archive
      Aggregator.FileArchive.clear();
      PopulateFiles(Aggregator.FileArchive, Taiga.GetDataPath() + L"Feed\\", L"torrent", true, true);
    
  // Events
  EventQueue.list.clear();
  for (xml_node user = doc.child(L"events").child(L"user"); user; user = user.next_sibling(L"user")) {
    for (xml_node item = user.child(L"event"); item; item = item.next_sibling(L"event")) {
      EventItem event_item;
      event_item.anime_id = item.attribute(L"id").as_int(ANIMEID_NOTINLIST);
      event_item.episode = item.attribute(L"episode").as_int(-1);
      event_item.score = item.attribute(L"score").as_int(-1);
      event_item.status = item.attribute(L"status").as_int(-1);
      event_item.enable_rewatching = item.attribute(L"rewatch").as_int(-1);
      event_item.tags = item.attribute(L"tags").value(EMPTY_STR);
      event_item.time = item.attribute(L"time").value();
      event_item.mode = item.attribute(L"mode").as_int();
      EventQueue.Add(event_item, false, user.attribute(L"name").value());
    }
  }

  return result.status == status_ok;
}

// =============================================================================

bool Settings::Save() {
  // Initialize
  xml_document doc;
  xml_node settings = doc.append_child();
  settings.set_name(L"settings");

  // Account
  settings.append_child(node_comment).set_value(L" Account ");
  xml_node account = settings.append_child();
  account.set_name(L"account");
    // MyAnimeList
    xml_node mal = account.append_child();
    mal.set_name(L"myanimelist");
    mal.append_attribute(L"username") = Account.MAL.user.c_str();
    mal.append_attribute(L"password") = SimpleEncrypt(Account.MAL.password).c_str();
    mal.append_attribute(L"api") = Account.MAL.api;
    mal.append_attribute(L"login") = Account.MAL.auto_login;
    // Update
    xml_node update = account.append_child();
    update.set_name(L"update");
    update.append_attribute(L"mode") = Account.Update.mode;
    update.append_attribute(L"time") = Account.Update.time;
    update.append_attribute(L"delay") = Account.Update.delay;
    update.append_attribute(L"checkplayer") = Account.Update.check_player;
    update.append_attribute(L"outofrange") = Account.Update.out_of_range;
  
  // Anime
  settings.append_child(node_comment).set_value(L" Anime list ");
  xml_node anime = settings.append_child();
  anime.set_name(L"anime");
    // Root folders  
    xml_node folders = anime.append_child();
    folders.set_name(L"folders");  
    for (size_t i = 0; i < Folders.root.size(); i++) {
      xml_node root = folders.append_child();
      root.set_name(L"root");
      root.append_attribute(L"folder") = Folders.root[i].c_str();
    }
    xml_node watch = folders.append_child();
    watch.set_name(L"watch");
    watch.append_attribute(L"enabled") = Folders.watch_enabled;
    // Items
    xml_node items = anime.append_child();
    items.set_name(L"items");
    for (size_t i = 0; i < Anime.items.size(); i++) {
      xml_node item = items.append_child();
      item.set_name(L"item");
      item.append_attribute(L"id") = Anime.items[i].id;
      if (!Anime.items[i].folder.empty())
        item.append_attribute(L"folder") = Anime.items[i].folder.c_str();
      if (!Anime.items[i].titles.empty())
        item.append_attribute(L"titles") = Anime.items[i].titles.c_str();
    }

  // Announcements
  settings.append_child(node_comment).set_value(L" Announcements ");
  xml_node announce = settings.append_child();
  announce.set_name(L"announce");
    // HTTP
    xml_node http = announce.append_child();
    http.set_name(L"http");
    http.append_attribute(L"enabled") = Announce.HTTP.enabled;
    http.append_attribute(L"format") = Announce.HTTP.format.c_str();
    http.append_attribute(L"url") = Announce.HTTP.url.c_str();
    // Messenger
    settings.child(L"announce").append_child().set_name(L"messenger");
    settings.child(L"announce").child(L"messenger").append_attribute(L"enabled") = Announce.MSN.enabled;
    settings.child(L"announce").child(L"messenger").append_attribute(L"format") = Announce.MSN.format.c_str();
    // mIRC
    xml_node mirc = announce.append_child();
    mirc.set_name(L"mirc");
    mirc.append_attribute(L"enabled") = Announce.MIRC.enabled;
    mirc.append_attribute(L"format") = Announce.MIRC.format.c_str();
    mirc.append_attribute(L"service") = Announce.MIRC.service.c_str();
    mirc.append_attribute(L"mode") = Announce.MIRC.mode;
    mirc.append_attribute(L"useaction") = Announce.MIRC.use_action;
    mirc.append_attribute(L"multiserver") = Announce.MIRC.multi_server;
    mirc.append_attribute(L"channels") = Announce.MIRC.channels.c_str();
    // Skype
    xml_node skype = announce.append_child();
    skype.set_name(L"http");
    skype.append_attribute(L"enabled") = Announce.Skype.enabled;
    skype.append_attribute(L"format") = Announce.Skype.format.c_str();
    // Twitter
    xml_node twitter = announce.append_child();
    twitter.set_name(L"twitter");
    twitter.append_attribute(L"enabled") = Announce.Twitter.enabled;
    twitter.append_attribute(L"format") = Announce.Twitter.format.c_str();
    twitter.append_attribute(L"oauth_token") = Announce.Twitter.oauth_key.c_str();
    twitter.append_attribute(L"oauth_secret") = Announce.Twitter.oauth_secret.c_str();
    twitter.append_attribute(L"user") = Announce.Twitter.user.c_str();

  // Program
  settings.append_child(node_comment).set_value(L" Program ");
  xml_node program = settings.append_child();
  program.set_name(L"program");
    // General
    xml_node general = program.append_child();
    general.set_name(L"general");
    general.append_attribute(L"autostart") = Program.General.auto_start;
    general.append_attribute(L"close") = Program.General.close;
    general.append_attribute(L"minimize") = Program.General.minimize;
    general.append_attribute(L"searchindex") = Program.General.search_index;
    general.append_attribute(L"sizex") = Program.General.size_x;
    general.append_attribute(L"sizey") = Program.General.size_y;
    general.append_attribute(L"theme") = Program.General.theme.c_str();
    // Startup
    xml_node startup = program.append_child();
    startup.set_name(L"startup");
    startup.append_attribute(L"checkversion") = Program.StartUp.check_new_version;
    startup.append_attribute(L"checkeps") = Program.StartUp.check_new_episodes;
    startup.append_attribute(L"minimize") = Program.StartUp.minimize;
    // Exit
    xml_node exit = program.append_child();
    exit.set_name(L"exit");
    exit.append_attribute(L"ask") = Program.Exit.ask;
    exit.append_attribute(L"savebuffer") = Program.Exit.save_event_queue;
    // Proxy
    xml_node proxy = program.append_child();
    proxy.set_name(L"proxy");
    proxy.append_attribute(L"host") = Program.Proxy.host.c_str();
    proxy.append_attribute(L"username") = Program.Proxy.user.c_str();
    proxy.append_attribute(L"password") = SimpleEncrypt(Program.Proxy.password).c_str();
    // List
    xml_node list = program.append_child();
    list.set_name(L"list");
      // Actions  
      xml_node action = list.append_child();
      action.set_name(L"action");
      action.append_attribute(L"doubleclick") = Program.List.double_click;
      action.append_attribute(L"middleclick") = Program.List.middle_click;
      // Filter
      xml_node filter = list.append_child();
      filter.set_name(L"filter");
      filter.append_child().set_name(L"status");
      filter.child(L"status").append_attribute(L"s1") = AnimeList.filters.status[0];
      filter.child(L"status").append_attribute(L"s2") = AnimeList.filters.status[1];
      filter.child(L"status").append_attribute(L"s3") = AnimeList.filters.status[2];
      filter.append_child().set_name(L"type");
      filter.child(L"type").append_attribute(L"t1") = AnimeList.filters.type[0];
      filter.child(L"type").append_attribute(L"t2") = AnimeList.filters.type[1];
      filter.child(L"type").append_attribute(L"t3") = AnimeList.filters.type[2];
      filter.child(L"type").append_attribute(L"t4") = AnimeList.filters.type[3];
      filter.child(L"type").append_attribute(L"t5") = AnimeList.filters.type[4];
      filter.child(L"type").append_attribute(L"t6") = AnimeList.filters.type[5];
      filter.append_child().set_name(L"episodes");
      filter.child(L"episodes").append_attribute(L"highlight") = Program.List.highlight;
      filter.child(L"episodes").append_attribute(L"new") = AnimeList.filters.new_episodes;
      // Progress
      xml_node progress = list.append_child();
      progress.set_name(L"progress");
      progress.append_attribute(L"mode") = Program.List.progress_mode;
      progress.append_attribute(L"showeps") = Program.List.progress_show_eps;
    // Notifications
    xml_node notifications = program.append_child();
    notifications.set_name(L"notifications");
    notifications.append_child().set_name(L"balloon");
    notifications.child(L"balloon").append_attribute(L"enabled") = Program.Balloon.enabled;
    notifications.child(L"balloon").append_attribute(L"format") = Program.Balloon.format.c_str();

  // RSS
  settings.append_child(node_comment).set_value(L" RSS ");
  xml_node rss = settings.append_child();
  rss.set_name(L"rss");
    // Torrent
    xml_node torrent = rss.append_child();
    torrent.set_name(L"torrent");
      // General
      torrent.append_child().set_name(L"application");
      torrent.child(L"application").append_attribute(L"mode") = RSS.Torrent.app_mode;
      torrent.child(L"application").append_attribute(L"path") = RSS.Torrent.app_path.c_str();
      torrent.append_child().set_name(L"source");
      torrent.child(L"source").append_attribute(L"address") = RSS.Torrent.source.c_str();
      torrent.append_child().set_name(L"options");
      torrent.child(L"options").append_attribute(L"autocheck") = RSS.Torrent.check_enabled;
      torrent.child(L"options").append_attribute(L"checkinterval") = RSS.Torrent.check_interval;
      torrent.child(L"options").append_attribute(L"autosetfolder") = RSS.Torrent.set_folder;
      torrent.child(L"options").append_attribute(L"hideunidentified") = RSS.Torrent.hide_unidentified;
      torrent.child(L"options").append_attribute(L"newaction") = RSS.Torrent.new_action;
      // Filter
      xml_node torrent_filter = torrent.append_child();
      torrent_filter.set_name(L"filter");
      torrent_filter.append_attribute(L"enabled") = RSS.Torrent.Filters.global_enabled;
      for (auto it = Aggregator.FilterManager.Filters.begin(); it != Aggregator.FilterManager.Filters.end(); ++it) {
        xml_node item = torrent_filter.append_child();
        item.set_name(L"item");
        item.append_attribute(L"action") = it->Action;
        item.append_attribute(L"match") = it->Match;
        item.append_attribute(L"enabled") = it->Enabled;
        item.append_attribute(L"name") = it->Name.c_str();
        for (auto ita = it->AnimeIds.begin(); ita != it->AnimeIds.end(); ++ita) {
          xml_node anime = item.append_child();
          anime.set_name(L"anime");
          anime.append_attribute(L"id") = *ita;
        }
        for (auto itc = it->Conditions.begin(); itc != it->Conditions.end(); ++itc) {
          xml_node condition = item.append_child();
          condition.set_name(L"condition");
          condition.append_attribute(L"element") = itc->Element;
          condition.append_attribute(L"op") = itc->Operator;
          condition.append_attribute(L"value") = itc->Value.c_str();
        }
      }
  
  // Write registry
  CRegistry reg;
  reg.OpenKey(HKEY_CURRENT_USER, 
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE);
  if (Program.General.auto_start) {
    wstring app_path = Taiga.GetModulePath();
    reg.SetValue(APP_NAME, app_path.c_str());
  } else {
    reg.DeleteValue(APP_NAME);
  }
  reg.CloseKey();

  // Write event queue
  if (Program.Exit.save_event_queue && !EventQueue.IsEmpty()) {
    xml_node events = doc.append_child();
    events.set_name(L"events");
    for (auto i = EventQueue.list.begin(); i != EventQueue.list.end(); ++i) {
      if (i->items.empty()) continue;
      xml_node user = events.append_child();
      user.set_name(L"user");
      user.append_attribute(L"name") = i->user.c_str();
      for (auto j = i->items.begin(); j != i->items.end(); ++j) {
        xml_node item = user.append_child();
        item.set_name(L"event");
        #define APPEND_ATTRIBUTE_INT(x, y) \
          if (j->y > -1) item.append_attribute(x) = j->y;
        #define APPEND_ATTRIBUTE_STR(x, y) \
          if (j->y != EMPTY_STR) item.append_attribute(x) = j->y.c_str();
        APPEND_ATTRIBUTE_INT(L"mode", mode);
        APPEND_ATTRIBUTE_INT(L"id", anime_id);
        APPEND_ATTRIBUTE_INT(L"episode", episode);
        APPEND_ATTRIBUTE_INT(L"score", score);
        APPEND_ATTRIBUTE_INT(L"status", status);
        APPEND_ATTRIBUTE_INT(L"rewatch", enable_rewatching);
        APPEND_ATTRIBUTE_STR(L"tags", tags);
        APPEND_ATTRIBUTE_STR(L"time", time);
        #undef APPEND_ATTRIBUTE_STR
        #undef APPEND_ATTRIBUTE_INT
      }
    }
  }

  // Save file
  ::CreateDirectory(folder_.c_str(), NULL);
  return doc.save_file(file_.c_str(), L"\x09", format_default | format_write_bom);
}

// =============================================================================

void Settings::Anime::SetItem(int id, wstring folder, wstring titles) {
  int index = -1;
  for (unsigned int i = 0; i < items.size(); i++) {
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
  if (folder != EMPTY_STR) items[index].folder = folder;
  if (titles != EMPTY_STR) items[index].titles = titles;
}