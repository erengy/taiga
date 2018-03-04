#include "discord-RPC/discord_rpc_functions.h"
#include <string>

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

  void updateDiscordPresence()
  {
    //char buffer[256];
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));
    discordPresence.state = "Taiga Discord Test";
    //sprintf(buffer, "Frustration level: %d", 1);
    discordPresence.details = "Watching something";
    //discordPresence.startTimestamp = 0;
    //discordPresence.endTimestamp = 250;
    //discordPresence.largeImageKey = "canary-large";
    //discordPresence.smallImageKey = "ptb-small";
    //discordPresence.partyId = "party1234";
    //discordPresence.partySize = 1;
    //discordPresence.partyMax = 6;
    //discordPresence.matchSecret = "xyzzy";
    //discordPresence.joinSecret = "join";
    //discordPresence.spectateSecret = "look";
    //discordPresence.instance = 0;
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
}