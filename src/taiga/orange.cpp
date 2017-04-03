/*
** Taiga
** Copyright (C) 2010-2017, Eren Okka
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

#include <cmath>

#include "taiga/orange.h"

namespace taiga {

Orange orange;

int Orange::note_list_[][2] = {
  {84, 1},  // 1/2
  {84, 2},  // 1/4
  {86, 4},  // 1/8
  {84, 2},  // 1/4
  {82, 2},  // 1/4
  {81, 2},  // 1/4
  {77, 4},  // 1/8
  {79, 4},  // 1/8
  {72, 4},  // 1/8
  {77, 1},  // 1/2
  {76, 4},  // 1/8
  {77, 4},  // 1/8
  {79, 4},  // 1/8
  {81, 2},  // 1/4
  {79, 2},  // 1/4
  {77, 2},  // 1/4
  {79, 2},  // 1/4
  {81, 4},  // 1/8
  {84, 1},  // 1/2
  {84, 2},  // 1/2
  {86, 4},  // 1/8
  {84, 2},  // 1/4
  {82, 2},  // 1/4
  {81, 2},  // 1/4
  {77, 4},  // 1/8
  {79, 4},  // 1/8
  {72, 4},  // 1/8
  {77, 1},  // 1/2
  {76, 4},  // 1/8
  {77, 4},  // 1/8
  {76, 4},  // 1/8
  {74, 1}   // 1/1
};

Orange::Orange()
    : play_(false) {
}

Orange::~Orange() {
  Stop();
}

void Orange::Start() {
  Stop();
  play_ = true;

  if (!GetThreadHandle())
    CreateThread(nullptr, 0, 0);
}

void Orange::Stop() {
  play_ = false;

  if (GetThreadHandle()) {
    WaitForSingleObject(GetThreadHandle(), INFINITE);
    CloseThreadHandle();
  }
}

DWORD Orange::ThreadProc() {
  size_t note_count = 32;
  size_t note_index = 0;

  while (play_ && note_index < note_count) {
    int note = note_list_[note_index][0];

    DWORD frequency = static_cast<DWORD>(NoteToFrequency(note));
    DWORD duration = 800 / note_list_[note_index][1];

    Beep(frequency, duration);

    note_index++;
  };

  return 0;
}

float Orange::NoteToFrequency(int n) {
  if (n < 0 || n > 119)
    return -1.0f;

  return 440.0f * std::pow(2.0f, static_cast<float>(n - 57) / 12.0f);
}

}  // namespace taiga