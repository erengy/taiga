#include "discord-RPC/discord_rpc_functions.h"
#include <string>
#include <ctime>

namespace discord
{
  void discordInit()
  {
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));
    handlers.ready = handleDiscordReady;
    handlers.disconnected = handleDiscordDisconnected;
    handlers.errored = handleDiscordError;
    handlers.joinGame = handleDiscordJoin;
    handlers.spectateGame = handleDiscordSpectate;
    handlers.joinRequest = handleDiscordJoinRequest;
    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
  }

  //void updateDiscordPresence()
  //{
  //  DiscordRichPresence discordPresence;
  //  memset(&discordPresence, 0, sizeof(discordPresence));
  //  discordPresence.state = "Taiga Discord Test";
  //  discordPresence.details = "Watching something";
  //  Discord_UpdatePresence(&discordPresence);
  //}

  void updateDiscordPresence(bool idling, const std::wstring& anime_name)
  {
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    //discordPresence.state = "Taiga Discord Test";
    //discordPresence.details = "Watching something";

    if (idling)
    {
      discordPresence.details = "Idling";
    }
    else
    {
      discordPresence.details = "Watching Anime";
      char* tmpString = new char[255];
      sprintf_s(tmpString, sizeof(char) * 255, "%ls", anime_name.c_str());
      discordPresence.state = tmpString;
      discordPresence.startTimestamp = time(0);
    }

    Discord_UpdatePresence(&discordPresence);
  }

  static void handleDiscordReady(void)
  {
    printf("\nDiscord: ready\n");
  }

  static void handleDiscordDisconnected(int errcode, const char* message)
  {
    printf("\nDiscord: disconnected (%d: %s)\n", errcode, message);
  }

  static void handleDiscordError(int errcode, const char* message)
  {
    printf("\nDiscord: error (%d: %s)\n", errcode, message);
  }

  static void handleDiscordJoin(const char* secret)
  {
    printf("\nDiscord: join (%s)\n", secret);
  }

  static void handleDiscordSpectate(const char* secret)
  {
    printf("\nDiscord: spectate (%s)\n", secret);
  }

  static void handleDiscordJoinRequest(const DiscordJoinRequest* request)
  {

  }
} // namespace discord