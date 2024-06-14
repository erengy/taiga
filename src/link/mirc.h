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

#include <string>
#include <vector>

namespace link::mirc {

enum ChannelMode {
  kChannelModeActive = 1,
  kChannelModeAll,
  kChannelModeCustom,
};

bool IsRunning();
bool GetChannels(const std::wstring& service,
                 std::vector<std::wstring>& channels);
bool Send(const std::wstring& service, std::wstring channels,
          const std::wstring& data, int mode, bool use_action,
          bool multi_server);

}  // namespace link::mirc
