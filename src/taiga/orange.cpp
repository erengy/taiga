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

#include <array>
#include <cmath>

#include "taiga/orange.h"

namespace taiga {

constexpr std::array<std::pair<int, float>, 32> notes{{
      {84, 1/2.f}, {84, 1/4.f}, {86, 1/8.f}, {84, 1/4.f},
      {82, 1/4.f}, {81, 1/4.f}, {77, 1/8.f}, {79, 1/8.f},
      {72, 1/8.f}, {77, 1/2.f}, {76, 1/8.f}, {77, 1/8.f},
      {79, 1/8.f}, {81, 1/4.f}, {79, 1/4.f}, {77, 1/4.f},
      {79, 1/4.f}, {81, 1/8.f}, {84, 1/2.f}, {84, 1/4.f},
      {86, 1/8.f}, {84, 1/4.f}, {82, 1/4.f}, {81, 1/4.f},
      {77, 1/8.f}, {79, 1/8.f}, {72, 1/8.f}, {77, 1/2.f},
      {76, 1/8.f}, {77, 1/8.f}, {76, 1/8.f}, {74, 1/2.f},
    }};

Orange::~Orange() {
  Stop();
}

void Orange::Start() {
  Stop();
  playing_ = true;

  if (!GetThreadHandle())
    CreateThread(nullptr, 0, 0);
}

void Orange::Stop() {
  playing_ = false;

  if (GetThreadHandle()) {
    ::WaitForSingleObject(GetThreadHandle(), INFINITE);
    CloseThreadHandle();
  }
}

DWORD Orange::ThreadProc() {
  constexpr auto get_frequency = [](const int note) {
    if (note < 0 || note > 119)
      return -1.0f;
    return 440.0f * std::pow(2.0f, static_cast<float>(note - 57) / 12.0f);
  };

  constexpr auto get_duration = [](const float duration) {
    return 1600 * duration;
  };

  for (const auto& [note, duration] : notes) {
    if (!playing_)
      break;

    ::Beep(static_cast<DWORD>(get_frequency(note)),
           static_cast<DWORD>(get_duration(duration)));
  }

  return 0;
}

}  // namespace taiga
