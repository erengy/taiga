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

#include <windows/win/dde.h>

#include "link/mirc.h"

#include "base/string.h"
#include "ui/ui.h"

namespace link::mirc {

bool IsRunning() {
  return ::FindWindow(L"mIRC", nullptr) != nullptr;
}

bool GetChannels(const std::wstring& service,
                 std::vector<std::wstring>& channels) {
  win::DynamicDataExchange dde;

  if (!dde.Initialize() || !dde.Connect(service, L"CHANNELS"))
    return false;

  std::wstring data;
  bool success = dde.ClientTransaction(L" ", L"", &data, XTYP_REQUEST) != FALSE;
  Split(data, L" ", channels);

  return success;
}

static bool SendCommands(const std::wstring& service,
                         const std::vector<std::wstring>& commands) {
  win::DynamicDataExchange dde;

  if (!dde.Initialize() || !dde.Connect(service, L"COMMAND"))
    return false;

  for (const auto& command : commands) {
    if (dde.ClientTransaction(L" ", command, nullptr, XTYP_POKE) == FALSE)
      return false;
  }

  return true;
}

bool Send(const std::wstring& service, std::wstring channels,
          const std::wstring& data, int mode, bool use_action,
          bool multi_server) {
  if (!IsRunning())
    return false;
  if (service.empty() || channels.empty() || data.empty())
    return false;

  // Initialize
  win::DynamicDataExchange dde;
  if (!dde.Initialize()) {
    ui::OnMircDdeInitFail();
    return false;
  }

  // List channels
  std::vector<std::wstring> channel_list;
  std::wstring active_channel;
  switch (mode) {
    case kChannelModeActive:
    case kChannelModeAll:
      GetChannels(service, channel_list);
      break;
    case kChannelModeCustom:
      Tokenize(channels, L" ,;", channel_list);
      break;
  }
  for (auto& channel : channel_list) {
    Trim(channel);
    if (channel.empty())  // ?
      continue;
    if (channel.front() == '*') {
      channel = channel.substr(1);
      active_channel = channel;
    } else if (channel.front() != '#') {
      channel.insert(channel.begin(), '#');
    }
  }

  // Send message to channels
  std::vector<std::wstring> commands;
  for (const auto& channel : channel_list) {
    if (mode == kChannelModeActive && channel != active_channel)
      continue;
    std::wstring command;
    if (multi_server)
      command += L"/scon -a ";
    command += use_action ? L"/describe " : L"/msg ";
    command += channel + L" " + data;
    commands.push_back(command);
  }
  return SendCommands(service, commands);
}

}  // namespace link::mirc
