/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#include <string>
#include <vector>
#include <windows.h>

#include "taiga/command_line.h"

#include "base/log.h"

namespace taiga {

static std::vector<std::wstring> ParseCommandLineArgs() {
  std::vector<std::wstring> args;

  int argc = 0;
  const auto argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
  if (argc && argv) {
    for (int i = 1; i < argc; i++) {
      args.emplace_back(argv[i]);
    }
    ::LocalFree(argv);
  }

  return args;
}

CommandLineOptions ParseCommandLine() {
  CommandLineOptions options;

  const auto args = ParseCommandLineArgs();

  for (const auto& arg : args) {
    bool found = false;

    if (arg == L"-allowmultipleinstances") {
      options.allow_multiple_instances = true;
      found = true;
    } else if (arg == L"-debug") {
      options.debug_mode = true;
      found = true;
    }

    if (found) {
      LOGD(arg);
    } else {
      LOGW(L"Invalid argument: {}", arg);
    }
  }

  return options;
}

}  // namespace taiga
