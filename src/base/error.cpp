/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
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

#include <Windows.h>

#include "error.h"
#include "string.h"

namespace base {

ErrorMode::ErrorMode(unsigned int mode) {
  error_mode_ = SetErrorMode(mode);
  SetErrorMode(error_mode_ | mode);
};

ErrorMode::~ErrorMode() {
  SetErrorMode(error_mode_);
};

std::wstring FormatError(DWORD error, const std::wstring* source) {
  DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS;
  HMODULE module_handle = nullptr;

  const DWORD size = 101;
  WCHAR buffer[size];

  if (source) {
    flags |= FORMAT_MESSAGE_FROM_HMODULE;
    module_handle = LoadLibrary(source->c_str());
    if (!module_handle)
      return std::wstring();
  } else {
    flags |= FORMAT_MESSAGE_FROM_SYSTEM;
  }

  DWORD language_id = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

  if (FormatMessage(flags, module_handle, error, language_id,
                    buffer, size, nullptr)) {
      if (module_handle)
        FreeLibrary(module_handle);
      return buffer;
  } else {
    if (module_handle)
      FreeLibrary(module_handle);
    return ToWstr(static_cast<UINT>(error));
  }
}

}  // namespace base