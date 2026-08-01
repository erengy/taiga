/*
** Taiga
** Copyright (C) 2010-2021, Eren Okka
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

#include "base/command_line.h"

#include "base/string.h"

namespace base {

std::vector<std::wstring> ParseCommandLineArgs() {
  std::vector<std::wstring> args;

  int argc = 0;
  const auto argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
  if (argc && argv) {
    for (int i = 1; i < argc; i++) {
      std::wstring arg = argv[i];
      TrimLeft(arg, L" -");
      args.emplace_back(arg);
    }
    ::LocalFree(argv);
  }

  return args;
}

}  // namespace base
