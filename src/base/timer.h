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

#pragma once

#include <map>
#include <windows.h>

namespace base {

class Timer {
public:
  Timer(unsigned int id, int interval, bool repeat);
  virtual ~Timer() {}

  bool enabled() const;
  unsigned int id() const;
  int interval() const;
  bool repeat() const;
  int ticks() const;

  void set_enabled(bool enabled);
  void set_id(unsigned int id);
  void set_interval(int interval);
  void set_repeat(bool repeat);
  void set_ticks(int ticks);

  // Resets ticks to interval
  void Reset();

  // Counts down from the interval
  void Tick();

protected:
  // What to do when timer reaches zero
  virtual void OnTimeout() = 0;

private:
  bool enabled_;
  unsigned int id_;
  int interval_;
  bool repeat_;
  int ticks_;
};

class TimerManager {
public:
  TimerManager();
  virtual ~TimerManager();

  Timer* timer(unsigned int id);

  virtual bool Initialize(HWND hwnd, TIMERPROC proc);
  virtual void InsertTimer(const Timer* timer);

  virtual void OnTick() = 0;

protected:
  HWND hwnd_;
  UINT_PTR id_;
  std::map<unsigned int, Timer*> timers_;
};

}  // namespace base
