#pragma once
#include "discord-RPC/include/discord_rpc.h"
#include <string>

namespace discord
{
  inline const char* APPLICATION_ID = "419764858976993280";

  void discordInit();
  void discordShutDown();
  void updateDiscordPresence(bool idling = true, const std::wstring& animeName = L"");
  static void handleDiscordReady(void);
  static void handleDiscordDisconnected(int errcode, const char* message);
  static void handleDiscordError(int errcode, const char* message);
  static void handleDiscordJoin(const char* secret);
  static void handleDiscordSpectate(const char* secret);
  static void handleDiscordJoinRequest(const DiscordJoinRequest* request);
} // namespace discord