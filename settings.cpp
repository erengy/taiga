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

CSettings Settings;

// =============================================================================

bool CSettings::Read() {
  // Initialize
  m_Folder = Taiga.GetDataPath();
  m_File = m_Folder + L"Settings.xml";
  CreateDirectory(m_Folder.c_str(), NULL);
  
  // Read XML file
  xml_document doc;
  xml_parse_result result = doc.load_file(m_File.c_str());
  
  // Read settings
  xml_node settings = doc.child(L"settings");
  
  // Account
  xml_node account = settings.child(L"account");
    // MyAnimeList
    xml_node mal = account.child(L"myanimelist");
    Account.MAL.API = mal.attribute(L"api").as_int(2);
    Account.MAL.AutoLogin = mal.attribute(L"login").as_int();
    Account.MAL.Password = SimpleDecrypt(mal.attribute(L"password").value());
    Account.MAL.User = mal.attribute(L"username").value();
    // Update
    xml_node update = account.child(L"update");
    Account.Update.CheckPlayer = update.attribute(L"checkplayer").as_int(TRUE);
    Account.Update.Delay = update.attribute(L"delay").as_int(60);
    Account.Update.Mode = update.attribute(L"mode").as_int(3);
    Account.Update.OutOfRange = update.attribute(L"outofrange").as_int();
    Account.Update.Time = update.attribute(L"time").as_int(2);

  // Announcements
  xml_node announce = settings.child(L"announce");
    // HTTP
    xml_node http = announce.child(L"http");
    Announce.HTTP.Enabled = http.attribute(L"enabled").as_int();
    Announce.HTTP.Format = http.attribute(L"format").value(DEFAULT_FORMAT_HTTP);
    Announce.HTTP.URL = http.attribute(L"url").value();
    // MSN
    xml_node messenger = announce.child(L"messenger");
    Announce.MSN.Enabled = messenger.attribute(L"enabled").as_int();
    Announce.MSN.Format = messenger.attribute(L"format").value(DEFAULT_FORMAT_MESSENGER);
    // mIRC
    xml_node mirc = announce.child(L"mirc");
    Announce.MIRC.Channels = mirc.attribute(L"channels").value(L"#myanimelist, #taiga");
    Announce.MIRC.Enabled = mirc.attribute(L"enabled").as_int();
    Announce.MIRC.Format = mirc.attribute(L"format").value(DEFAULT_FORMAT_MIRC);
    Announce.MIRC.Mode = mirc.attribute(L"mode").as_int(1);
    Announce.MIRC.MultiServer = mirc.attribute(L"multiserver").as_int(FALSE);
    Announce.MIRC.Service = mirc.attribute(L"service").value(L"mIRC");
    Announce.MIRC.UseAction = mirc.attribute(L"useaction").as_int(TRUE);
    // Skype
    xml_node skype = announce.child(L"skype");
    Announce.Skype.Enabled = skype.attribute(L"enabled").as_int();
    Announce.Skype.Format = skype.attribute(L"format").value(DEFAULT_FORMAT_SKYPE);
    // Twitter
    xml_node twitter = announce.child(L"twitter");
    Announce.Twitter.Enabled = twitter.attribute(L"enabled").as_int();
    Announce.Twitter.Format = twitter.attribute(L"format").value(DEFAULT_FORMAT_TWITTER);
    Announce.Twitter.OAuthKey = twitter.attribute(L"oauth_token").value();
    Announce.Twitter.OAuthSecret = twitter.attribute(L"oauth_secret").value();
    Announce.Twitter.User = twitter.attribute(L"user").value();

  // Folders
  Folders.Root.clear();
  xml_node folders = settings.child(L"anime").child(L"folders");
  for (xml_node folder = folders.child(L"root"); folder; folder = folder.next_sibling(L"root")) {
    Folders.Root.push_back(folder.attribute(L"folder").value());
  }
  xml_node watch = folders.child(L"watch");
  Folders.WatchEnabled = watch.attribute(L"enabled").as_int();

  // Anime items
  Anime.Item.clear();
  wstring fansub, folder, titles;
  xml_node items = settings.child(L"anime").child(L"items");
  for (xml_node item = items.child(L"item"); item; item = item.next_sibling(L"item")) {
    fansub = item.attribute(L"fansub").value(EMPTY_STR);
    folder = item.attribute(L"folder").value(EMPTY_STR);
    titles = item.attribute(L"titles").value(EMPTY_STR);
    Anime.SetItem(item.attribute(L"id").as_int(), fansub, folder, titles);
  }

  // Program
  xml_node program = settings.child(L"program");
    // General
    xml_node general = program.child(L"general");
    Program.General.AutoStart = general.attribute(L"autostart").as_int();
    Program.General.Close = general.attribute(L"close").as_int();
    Program.General.Minimize = general.attribute(L"minimize").as_int();
    Program.General.SearchIndex = general.attribute(L"searchindex").as_int();
    Program.General.SizeX = general.attribute(L"sizex").as_int();
    Program.General.SizeY = general.attribute(L"sizey").as_int();
    Program.General.Theme = general.attribute(L"theme").value(L"Default");
    // Start-up
    xml_node startup = program.child(L"startup");
    Program.StartUp.CheckNewEpisodes = startup.attribute(L"checkeps").as_int();
    Program.StartUp.CheckNewVersion = startup.attribute(L"checkversion").as_int(TRUE);
    Program.StartUp.Minimize = startup.attribute(L"minimize").as_int();
    // Exit
    xml_node exit = program.child(L"exit");
    Program.Exit.Ask = exit.attribute(L"ask").as_int();
    Program.Exit.SaveBuffer = exit.attribute(L"savebuffer").as_int(1);
    // Proxy
    xml_node proxy = program.child(L"proxy");
    Program.Proxy.Host = proxy.attribute(L"host").value();
    Program.Proxy.Password = SimpleDecrypt(proxy.attribute(L"password").value());
    Program.Proxy.User = proxy.attribute(L"username").value();
    HTTPClient.SetProxy(Program.Proxy.Host, Program.Proxy.User, Program.Proxy.Password);
    ImageClient.SetProxy(Program.Proxy.Host, Program.Proxy.User, Program.Proxy.Password);
    MainClient.SetProxy(Program.Proxy.Host, Program.Proxy.User, Program.Proxy.Password);
    SearchClient.SetProxy(Program.Proxy.Host, Program.Proxy.User, Program.Proxy.Password);
    TorrentClient.SetProxy(Program.Proxy.Host, Program.Proxy.User, Program.Proxy.Password);
    TwitterClient.SetProxy(Program.Proxy.Host, Program.Proxy.User, Program.Proxy.Password);
    VersionClient.SetProxy(Program.Proxy.Host, Program.Proxy.User, Program.Proxy.Password);
    // List
    xml_node list = program.child(L"list");
    Program.List.DoubleClick = list.child(L"action").attribute(L"doubleclick").as_int(4);
    Program.List.MiddleClick = list.child(L"action").attribute(L"middleclick").as_int(3);
    for (int i = 0; i < 3; i++) {
      wstring name = L"s" + ToWSTR(i + 1);
      AnimeList.Filter.Status[i] = list.child(L"filter").child(L"status").attribute(name.c_str()).as_int(1);
    }
    for (int i = 0; i < 6; i++) {
      wstring name = L"t" + ToWSTR(i + 1);
      AnimeList.Filter.Type[i] = list.child(L"filter").child(L"type").attribute(name.c_str()).as_int(1);
    }
    Program.List.Highlight = list.child(L"filter").child(L"episodes").attribute(L"highlight").as_int(TRUE);
    AnimeList.Filter.NewEps = list.child(L"filter").child(L"episodes").attribute(L"new").as_int();
    Program.List.ProgressMode = list.child(L"progress").attribute(L"mode").as_int(1);
    Program.List.ProgressShowEps = list.child(L"progress").attribute(L"showeps").as_int();
    // Notifications
    xml_node notifications = program.child(L"notifications");
    Program.Balloon.Enabled = notifications.child(L"balloon").attribute(L"enabled").as_int(TRUE);
    Program.Balloon.Format = notifications.child(L"balloon").attribute(L"format").value(DEFAULT_FORMAT_BALLOON);

  // RSS
  xml_node rss = settings.child(L"rss");
    // Torrent  
    xml_node torrent = rss.child(L"torrent");
      // General
      RSS.Torrent.AppMode = torrent.child(L"application").attribute(L"mode").as_int(1);
      RSS.Torrent.AppPath = torrent.child(L"application").attribute(L"path").value();
      if (RSS.Torrent.AppPath.empty()) {
        RSS.Torrent.AppPath = GetDefaultAppPath(L".torrent", DEFAULT_TORRENT_APPPATH);
      }
      RSS.Torrent.CheckEnabled = torrent.child(L"options").attribute(L"autocheck").as_int(TRUE);
      RSS.Torrent.CheckInterval = torrent.child(L"options").attribute(L"checkinterval").as_int(60);
      RSS.Torrent.HideUnidentified = torrent.child(L"options").attribute(L"hideunidentified").as_int(FALSE);
      RSS.Torrent.NewAction = torrent.child(L"options").attribute(L"newaction").as_int(1);
      RSS.Torrent.SetFolder = torrent.child(L"options").attribute(L"autosetfolder").as_int(TRUE);
      RSS.Torrent.Source = torrent.child(L"source").attribute(L"address").value(DEFAULT_TORRENT_SOURCE);
      // Filters
      RSS.Torrent.Filters.Global.clear();
      xml_node filter = torrent.child(L"filter");
      xml_node global = filter.child(L"global");
      RSS.Torrent.Filters.GlobalEnabled = global.attribute(L"enabled").as_int(TRUE);
      for (xml_node item = global.child(L"item"); item; item = item.next_sibling(L"item")) {
        Torrents.AddFilter(item.attribute(L"option").as_int(), item.attribute(L"type").as_int(), item.attribute(L"value").value());
      }
      if (RSS.Torrent.Filters.Global.empty() && RSS.Torrent.Filters.GlobalEnabled == TRUE) {
        Torrents.AddFilter(0, 2, L"2");
        Torrents.AddFilter(0, 2, L"4");
        Torrents.AddFilter(2, 0, L"720p");
        Torrents.AddFilter(2, 0, L"HD");
        Torrents.AddFilter(2, 0, L"MKV");
      }
      // Archive
      Torrents.Archive.clear();
      PopulateFiles(Torrents.Archive, Taiga.GetDataPath() + L"Torrents\\");
    
  // Events
  EventQueue.List.clear();
  for (xml_node user = doc.child(L"events").child(L"user"); user; user = user.next_sibling(L"user")) {
    for (xml_node item = user.child(L"event"); item; item = item.next_sibling(L"event")) {
      CEventItem event_item;
      event_item.AnimeIndex = item.attribute(L"index").as_int(-1);
      event_item.AnimeID = item.attribute(L"id").as_int(-1);
      event_item.episode = item.attribute(L"episode").as_int(-1);
      event_item.score = item.attribute(L"score").as_int(-1);
      event_item.status = item.attribute(L"status").as_int(-1);
      event_item.enable_rewatching = item.attribute(L"rewatch").as_int(-1);
      event_item.tags = item.attribute(L"tags").value();
      event_item.Time = item.attribute(L"time").value();
      event_item.Mode = item.attribute(L"mode").as_int();
      EventQueue.Add(event_item, false, user.attribute(L"name").value());
    }
  }

  return result.status == status_ok;
}

// =============================================================================

bool CSettings::Write() {
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
    mal.append_attribute(L"username") = Account.MAL.User.c_str();
    mal.append_attribute(L"password") = SimpleEncrypt(Account.MAL.Password).c_str();
    mal.append_attribute(L"api") = Account.MAL.API;
    mal.append_attribute(L"login") = Account.MAL.AutoLogin;
    // Update
    xml_node update = account.append_child();
    update.set_name(L"update");
    update.append_attribute(L"mode") = Account.Update.Mode;
    update.append_attribute(L"time") = Account.Update.Time;
    update.append_attribute(L"delay") = Account.Update.Delay;
    update.append_attribute(L"checkplayer") = Account.Update.CheckPlayer;
    update.append_attribute(L"outofrange") = Account.Update.OutOfRange;
  
  // Anime
  settings.append_child(node_comment).set_value(L" Anime list ");
  xml_node anime = settings.append_child();
  anime.set_name(L"anime");
    // Root folders  
    xml_node folders = anime.append_child();
    folders.set_name(L"folders");  
    for (size_t i = 0; i < Folders.Root.size(); i++) {
      xml_node root = folders.append_child();
      root.set_name(L"root");
      root.append_attribute(L"folder") = Folders.Root[i].c_str();
    }
    xml_node watch = folders.append_child();
    watch.set_name(L"watch");
    watch.append_attribute(L"enabled") = Folders.WatchEnabled;
    // Items
    xml_node items = anime.append_child();
    items.set_name(L"items");
    for (size_t i = 0; i < Anime.Item.size(); i++) {
      xml_node item = items.append_child();
      item.set_name(L"item");
      item.append_attribute(L"id") = Anime.Item[i].ID;
      if (!Anime.Item[i].FansubGroup.empty())
        item.append_attribute(L"fansub") = Anime.Item[i].FansubGroup.c_str();
      if (!Anime.Item[i].Folder.empty())
        item.append_attribute(L"folder") = Anime.Item[i].Folder.c_str();
      if (!Anime.Item[i].Titles.empty())
        item.append_attribute(L"titles") = Anime.Item[i].Titles.c_str();
    }

  // Announcements
  settings.append_child(node_comment).set_value(L" Announcements ");
  xml_node announce = settings.append_child();
  announce.set_name(L"announce");
    // HTTP
    xml_node http = announce.append_child();
    http.set_name(L"http");
    http.append_attribute(L"enabled") = Announce.HTTP.Enabled;
    http.append_attribute(L"format") = Announce.HTTP.Format.c_str();
    http.append_attribute(L"url") = Announce.HTTP.URL.c_str();
    // Messenger
    settings.child(L"announce").append_child().set_name(L"messenger");
    settings.child(L"announce").child(L"messenger").append_attribute(L"enabled") = Announce.MSN.Enabled;
    settings.child(L"announce").child(L"messenger").append_attribute(L"format") = Announce.MSN.Format.c_str();
    // mIRC
    xml_node mirc = announce.append_child();
    mirc.set_name(L"mirc");
    mirc.append_attribute(L"enabled") = Announce.MIRC.Enabled;
    mirc.append_attribute(L"format") = Announce.MIRC.Format.c_str();
    mirc.append_attribute(L"service") = Announce.MIRC.Service.c_str();
    mirc.append_attribute(L"mode") = Announce.MIRC.Mode;
    mirc.append_attribute(L"useaction") = Announce.MIRC.UseAction;
    mirc.append_attribute(L"multiserver") = Announce.MIRC.MultiServer;
    mirc.append_attribute(L"channels") = Announce.MIRC.Channels.c_str();
    // Skype
    xml_node skype = announce.append_child();
    skype.set_name(L"http");
    skype.append_attribute(L"enabled") = Announce.Skype.Enabled;
    skype.append_attribute(L"format") = Announce.Skype.Format.c_str();
    // Twitter
    xml_node twitter = announce.append_child();
    twitter.set_name(L"twitter");
    twitter.append_attribute(L"enabled") = Announce.Twitter.Enabled;
    twitter.append_attribute(L"format") = Announce.Twitter.Format.c_str();
    twitter.append_attribute(L"oauth_token") = Announce.Twitter.OAuthKey.c_str();
    twitter.append_attribute(L"oauth_secret") = Announce.Twitter.OAuthSecret.c_str();
    twitter.append_attribute(L"user") = Announce.Twitter.User.c_str();

  // Program
  settings.append_child(node_comment).set_value(L" Program ");
  xml_node program = settings.append_child();
  program.set_name(L"program");
    // General
    xml_node general = program.append_child();
    general.set_name(L"general");
    general.append_attribute(L"autostart") = Program.General.AutoStart;
    general.append_attribute(L"close") = Program.General.Close;
    general.append_attribute(L"minimize") = Program.General.Minimize;
    general.append_attribute(L"searchindex") = Program.General.SearchIndex;
    general.append_attribute(L"sizex") = Program.General.SizeX;
    general.append_attribute(L"sizey") = Program.General.SizeY;
    general.append_attribute(L"theme") = Program.General.Theme.c_str();
    // Startup
    xml_node startup = program.append_child();
    startup.set_name(L"startup");
    startup.append_attribute(L"checkversion") = Program.StartUp.CheckNewVersion;
    startup.append_attribute(L"checkeps") = Program.StartUp.CheckNewEpisodes;
    startup.append_attribute(L"minimize") = Program.StartUp.Minimize;
    // Exit
    xml_node exit = program.append_child();
    exit.set_name(L"exit");
    exit.append_attribute(L"ask") = Program.Exit.Ask;
    exit.append_attribute(L"savebuffer") = Program.Exit.SaveBuffer;
    // Proxy
    xml_node proxy = program.append_child();
    proxy.set_name(L"proxy");
    proxy.append_attribute(L"host") = Program.Proxy.Host.c_str();
    proxy.append_attribute(L"username") = Program.Proxy.User.c_str();
    proxy.append_attribute(L"password") = SimpleEncrypt(Program.Proxy.Password).c_str();
    // List
    xml_node list = program.append_child();
    list.set_name(L"list");
      // Actions  
      xml_node action = list.append_child();
      action.set_name(L"action");
      action.append_attribute(L"doubleclick") = Program.List.DoubleClick;
      action.append_attribute(L"middleclick") = Program.List.MiddleClick;
      // Filter
      xml_node filter = list.append_child();
      filter.set_name(L"filter");
      filter.append_child().set_name(L"status");
      filter.child(L"status").append_attribute(L"s1") = AnimeList.Filter.Status[0];
      filter.child(L"status").append_attribute(L"s2") = AnimeList.Filter.Status[1];
      filter.child(L"status").append_attribute(L"s3") = AnimeList.Filter.Status[2];
      filter.append_child().set_name(L"type");
      filter.child(L"type").append_attribute(L"t1") = AnimeList.Filter.Type[0];
      filter.child(L"type").append_attribute(L"t2") = AnimeList.Filter.Type[1];
      filter.child(L"type").append_attribute(L"t3") = AnimeList.Filter.Type[2];
      filter.child(L"type").append_attribute(L"t4") = AnimeList.Filter.Type[3];
      filter.child(L"type").append_attribute(L"t5") = AnimeList.Filter.Type[4];
      filter.child(L"type").append_attribute(L"t6") = AnimeList.Filter.Type[5];
      filter.append_child().set_name(L"episodes");
      filter.child(L"episodes").append_attribute(L"highlight") = Program.List.Highlight;
      filter.child(L"episodes").append_attribute(L"new") = AnimeList.Filter.NewEps;
      // Progress
      xml_node progress = list.append_child();
      progress.set_name(L"progress");
      progress.append_attribute(L"mode") = Program.List.ProgressMode;
      progress.append_attribute(L"showeps") = Program.List.ProgressShowEps;
    // Notifications
    xml_node notifications = program.append_child();
    notifications.set_name(L"notifications");
    notifications.append_child().set_name(L"balloon");
    notifications.child(L"balloon").append_attribute(L"enabled") = Program.Balloon.Enabled;
    notifications.child(L"balloon").append_attribute(L"format") = Program.Balloon.Format.c_str();

  // RSS
  settings.append_child(node_comment).set_value(L" RSS ");
  xml_node rss = settings.append_child();
  rss.set_name(L"rss");
    // Torrent
    xml_node torrent = rss.append_child();
    torrent.set_name(L"torrent");
      // General
      torrent.append_child().set_name(L"application");
      torrent.child(L"application").append_attribute(L"mode") = RSS.Torrent.AppMode;
      torrent.child(L"application").append_attribute(L"path") = RSS.Torrent.AppPath.c_str();
      torrent.append_child().set_name(L"source");
      torrent.child(L"source").append_attribute(L"address") = RSS.Torrent.Source.c_str();
      torrent.append_child().set_name(L"options");
      torrent.child(L"options").append_attribute(L"autocheck") = RSS.Torrent.CheckEnabled;
      torrent.child(L"options").append_attribute(L"checkinterval") = RSS.Torrent.CheckInterval;
      torrent.child(L"options").append_attribute(L"autosetfolder") = RSS.Torrent.SetFolder;
      torrent.child(L"options").append_attribute(L"hideunidentified") = RSS.Torrent.HideUnidentified;
      torrent.child(L"options").append_attribute(L"newaction") = RSS.Torrent.NewAction;
      // Filter
      torrent.append_child().set_name(L"filter");
      xml_node global = torrent.child(L"filter").append_child();
      global.set_name(L"global");
      global.append_attribute(L"enabled") = RSS.Torrent.Filters.GlobalEnabled;
      for (size_t i = 0; i < RSS.Torrent.Filters.Global.size(); i++) {
        xml_node item = global.append_child();
        item.set_name(L"item");
        item.append_attribute(L"option") = RSS.Torrent.Filters.Global[i].Option;
        item.append_attribute(L"type") = RSS.Torrent.Filters.Global[i].Type;
        item.append_attribute(L"value") = RSS.Torrent.Filters.Global[i].Value.c_str();
      }
  
  // Write registry
  CRegistry reg;
  reg.OpenKey(HKEY_CURRENT_USER, 
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE);
  if (Program.General.AutoStart) {
    wstring app_path = Taiga.GetModulePath();
    reg.SetValue(APP_NAME, app_path.c_str());
  } else {
    reg.DeleteValue(APP_NAME);
  }
  reg.CloseKey();

  // Write event buffer
  if (Program.Exit.SaveBuffer && !EventQueue.IsEmpty()) {
    xml_node events = doc.append_child();
    events.set_name(L"events");
    for (size_t i = 0; i < EventQueue.List.size(); i++) {
      if (EventQueue.List[i].Item.empty()) continue;
      xml_node user = events.append_child();
      user.set_name(L"user");
      user.append_attribute(L"name") = EventQueue.List[i].User.c_str();
      for (size_t j = 0; j < EventQueue.List[i].Item.size(); j++) {
        xml_node item = user.append_child();
        item.set_name(L"event");
        #define APPEND_ATTRIBUTE_INT(x, y) \
          if (EventQueue.List[i].Item[j].y > -1) \
          item.append_attribute(x) = EventQueue.List[i].Item[j].y;
        APPEND_ATTRIBUTE_INT(L"mode", Mode);
        APPEND_ATTRIBUTE_INT(L"index", AnimeIndex);
        APPEND_ATTRIBUTE_INT(L"id", AnimeID);
        APPEND_ATTRIBUTE_INT(L"episode", episode);
        APPEND_ATTRIBUTE_INT(L"score", score);
        APPEND_ATTRIBUTE_INT(L"status", status);
        APPEND_ATTRIBUTE_INT(L"rewatch", enable_rewatching);
        item.append_attribute(L"tags") = EventQueue.List[i].Item[j].tags.c_str();
        item.append_attribute(L"time") = EventQueue.List[i].Item[j].Time.c_str();
        #undef APPEND_ATTRIBUTE
      }
    }
  }
    
  // Save file
  ::CreateDirectory(m_Folder.c_str(), NULL);
  return doc.save_file(m_File.c_str(), L"\x09", format_default | format_write_bom);
}

// =============================================================================

void CSettings::CSettingsAnime::SetItem(int id, wstring fansub, wstring folder, wstring titles) {
  int index = -1;
  for (unsigned int i = 0; i < Item.size(); i++) {
    if (Item[i].ID == id) {
      index = static_cast<int>(i);
      break;
    }
  }
  if (index == -1) {
    index = Item.size();
    Item.resize(index + 1);
  }
  Item[index].ID = id;
  if (fansub != EMPTY_STR) Item[index].FansubGroup = fansub;
  if (folder != EMPTY_STR) Item[index].Folder = folder;
  if (titles != EMPTY_STR) Item[index].Titles = titles;
}