/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010, Eren Okka
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
  xml_parse_result result = doc.load_file(ToANSI(m_File));
  
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
    Account.Update.Delay = update.attribute(L"delay").as_int(10);
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
    Announce.Twitter.Password = SimpleDecrypt(twitter.attribute(L"password").value());
    Announce.Twitter.User = twitter.attribute(L"username").value();

  // Folders
  xml_node folders = settings.child(L"anime").child(L"folders");
  for (xml_node folder = folders.child(L"root"); folder; folder = folder.next_sibling(L"root")) {
    Folders.Root.push_back(folder.attribute(L"folder").value());
  }

  // Anime items
  wstring folder, titles;
  xml_node items = settings.child(L"anime").child(L"items");
  for (xml_node item = items.child(L"item"); item; item = item.next_sibling(L"item")) {
    folder = item.attribute(L"folder").value(); if (folder.empty()) folder = L"%empty%";
    titles = item.attribute(L"titles").value(); if (titles.empty()) titles = L"%empty%";
    Anime.SetItem(item.attribute(L"id").as_int(), folder, titles);
  }

  // Program
  xml_node program = settings.child(L"program");
    // General
    xml_node general = program.child(L"general");
    Program.General.AutoStart = general.attribute(L"autostart").as_int();
    Program.General.Close = general.attribute(L"close").as_int();
    Program.General.Minimize = general.attribute(L"minimize").as_int();
    Program.General.Theme = general.attribute(L"theme").value(L"Default");
    // Start-up
    xml_node startup = program.child(L"startup");
    Program.StartUp.CheckNewEpisodes = startup.attribute(L"checkeps").as_int();
    Program.StartUp.CheckNewVersion = startup.attribute(L"checkversion").as_int();
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
      wstring name = L"s" + WSTR(i + 1);
      AnimeList.Filter.Status[i] = list.child(L"filter").child(L"status").attribute(name.c_str()).as_int(1);
    }
    for (int i = 0; i < 6; i++) {
      wstring name = L"t" + WSTR(i + 1);
      AnimeList.Filter.Type[i] = list.child(L"filter").child(L"type").attribute(name.c_str()).as_int(1);
    }
    Program.List.Highlight = list.child(L"filter").child(L"episodes").attribute(L"highlight").as_int(TRUE);
    AnimeList.Filter.NewEps = list.child(L"filter").child(L"episodes").attribute(L"new").as_int();
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
      PopulateFiles(Torrents.Archive, Taiga.GetDataPath() + L"Torrents\\");
    
  // Events
  for (xml_node user = doc.child(L"events").child(L"user"); user; user = user.next_sibling(L"user")) {
    for (xml_node item = user.child(L"event"); item; item = item.next_sibling(L"event")) {
      EventBuffer.Add(user.attribute(L"name").value(),
        item.attribute(L"index").as_int(),
        item.attribute(L"id").as_int(),
        item.attribute(L"episode").as_int(),
        item.attribute(L"score").as_int(),
        item.attribute(L"status").as_int(),
        item.attribute(L"tags").value(),
        item.attribute(L"time").value(),
        item.attribute(L"mode").as_int());
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
    // Items
    xml_node items = anime.append_child();
    items.set_name(L"items");
    for (size_t i = 0; i < Anime.Item.size(); i++) {
      xml_node item = items.append_child();
      item.set_name(L"item");
      item.append_attribute(L"id") = Anime.Item[i].ID;
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
    twitter.append_attribute(L"username") = Announce.Twitter.User.c_str();
    twitter.append_attribute(L"password") = SimpleEncrypt(Announce.Twitter.Password).c_str();

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
  if (Program.General.AutoStart) {
    wstring app_path = Taiga.GetModulePath();
    Registry_SetValue(HKEY_CURRENT_USER, 
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", APP_NAME, 
      app_path.c_str(), app_path.size());
  } else {
    Registry_DeleteValue(HKEY_CURRENT_USER, 
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", APP_NAME);
  }

  // Write event buffer
  if (Program.Exit.SaveBuffer && !EventBuffer.IsEmpty()) {
    xml_node events = doc.append_child();
    events.set_name(L"events");
    for (size_t i = 0; i < EventBuffer.List.size(); i++) {
      if (EventBuffer.List[i].Item.empty()) continue;
      xml_node user = events.append_child();
      user.set_name(L"user");
      user.append_attribute(L"name") = EventBuffer.List[i].User.c_str();
      for (size_t j = 0; j < EventBuffer.List[i].Item.size(); j++) {
        xml_node item = user.append_child();
        item.set_name(L"event");
        item.append_attribute(L"index")   = EventBuffer.List[i].Item[j].Index;
        item.append_attribute(L"id")      = EventBuffer.List[i].Item[j].ID;
        item.append_attribute(L"episode") = EventBuffer.List[i].Item[j].Episode;
        item.append_attribute(L"score")   = EventBuffer.List[i].Item[j].Score;
        item.append_attribute(L"status")  = EventBuffer.List[i].Item[j].Status;
        item.append_attribute(L"tags")    = EventBuffer.List[i].Item[j].Tags.c_str();
        item.append_attribute(L"time")    = EventBuffer.List[i].Item[j].Time.c_str();
        item.append_attribute(L"mode")    = EventBuffer.List[i].Item[j].Mode;
      }
    }
  }
    
  // Save file
  ::CreateDirectory(m_Folder.c_str(), NULL);
  return doc.save_file(ToANSI(m_File), L"\x09", format_default | format_write_bom_utf8);
}

// =============================================================================

void CSettings::CSettingsAnime::SetItem(int id, wstring folder, wstring titles) {
  int index = -1;
  for (unsigned int i = 0; i < Item.size(); i++) {
    if (Item[i].ID == id) {
      index = static_cast<int>(i);
      break;
    }
  }
  if (index == -1) {
    if (folder.empty() || titles.empty()) return;
    index = Item.size();
    Item.resize(index + 1);
  }
  Item[index].ID = id;
  if (folder != L"%empty%") Item[index].Folder = folder;
  if (titles != L"%empty%") Item[index].Titles = titles;
}