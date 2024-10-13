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

#include "orange.hpp"

#include <array>
#include <cmath>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#endif

namespace {

/* clang-format off */
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
/* clang-format on */

constexpr float get_frequency(const int note) {
  if (note < 0 || note > 119) return -1.0f;
  return 440.0f * std::pow(2.0f, static_cast<float>(note - 57) / 12.0f);
};

constexpr float get_duration(const float duration) {
  return 1600 * duration;
};

}  // namespace

namespace taiga {

Orange::Orange(QObject* parent) : QThread(parent) {}

Orange::~Orange() {
  requestInterruption();
  wait();
}

Orange* Orange::create(QObject* parent) {
  auto thread = new Orange(parent);
  connect(thread, &Orange::finished, thread, &QObject::deleteLater);
  return thread;
}

void Orange::run() {
  for (const auto& [note, duration] : notes) {
    if (isInterruptionRequested()) break;
#ifdef Q_OS_WINDOWS
    ::Beep(static_cast<DWORD>(get_frequency(note)), static_cast<DWORD>(get_duration(duration)));
#endif
  }
}

}  // namespace taiga
