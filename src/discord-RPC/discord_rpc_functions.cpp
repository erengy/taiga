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

  void discordShutDown()
  {
    Discord_ClearPresence();
    Discord_Shutdown();
  }


  void updateDiscordPresence(bool idling, const std::wstring& anime_name)
  {
    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

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

    #ifdef DISCORD_DISABLE_IO_THREAD
        Discord_UpdateConnection();
    #endif
        Discord_RunCallbacks();
  }

  static void handleDiscordReady(void)
  {
    // Stub event
  }

  static void handleDiscordDisconnected(int errcode, const char* message)
  {
    // Stub event
  }

  static void handleDiscordError(int errcode, const char* message)
  {
    // Stub event
  }

  static void handleDiscordJoin(const char* secret)
  {
    // Stub event
  }

  static void handleDiscordSpectate(const char* secret)
  {
    // Stub event
  }

  static void handleDiscordJoinRequest(const DiscordJoinRequest* request)
  {
    // Stub event
  }
} // namespace discord