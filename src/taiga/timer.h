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

#include "base/timer.h"

namespace taiga {

enum TimerId {
  kTimerAnimeList = 1,
  kTimerDetection,
  kTimerHistory,
  kTimerLibrary,
  kTimerMedia,
  kTimerMemory,
  kTimerStats,
  kTimerTorrents
};

class Timer : public base::Timer {
public:
  Timer(unsigned int id, int interval = 1 /*second*/, bool repeat = true);
  ~Timer() {}

protected:
  void OnTimeout() override;
};

class TimerManager : public base::TimerManager {
public:
  void Initialize();

  void UpdateEnabledState();
  void UpdateIntervalsFromSettings();
  void UpdateUi();

protected:
  void OnTick() override;

private:
  static void CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent,
                                 DWORD dwTime);
};

inline TimerManager timers;

}  // namespace taiga
