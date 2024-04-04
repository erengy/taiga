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

#include "base/timer.h"

namespace base {

Timer::Timer(unsigned int id, int interval, bool repeat)
    : id_(id),
      interval_(interval),
      repeat_(repeat),
      ticks_(interval) {
}

bool Timer::enabled() const {
  return enabled_;
}

unsigned int Timer::id() const {
  return id_;
}

int Timer::interval() const {
  return interval_;
}

bool Timer::repeat() const {
  return repeat_;
}

int Timer::ticks() const {
  return ticks_;
}

void Timer::set_enabled(bool enabled) {
  enabled_ = enabled;
}

void Timer::set_id(unsigned int id) {
  id_ = id;
}

void Timer::set_interval(int interval) {
  int interval_difference = interval - interval_;
  interval_ = interval;
  ticks_ += interval_difference;

  if (ticks_ < 1)
    ticks_ = 1;  // So that it reaches 0 on next tick
}

void Timer::set_repeat(bool repeat) {
  repeat_ = repeat;
}

void Timer::set_ticks(int ticks) {
  ticks_ = ticks;

  if (ticks_ > interval_)
    ticks_ = interval_;
}

void Timer::Reset() {
  ticks_ = interval_;
  enabled_ = true;
}

void Timer::Tick() {
  if (!enabled_)
    return;

  if (interval_ > 0 && ticks_ > 0)
    ticks_ -= 1;

  if (ticks_ == 0) {
    OnTimeout();

    if (repeat_) {
      Reset();
    } else {
      enabled_ = false;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

TimerManager::~TimerManager() {
  if (id_)
    ::KillTimer(hwnd_, id_);
}

Timer* TimerManager::timer(unsigned int id) {
  const auto it = timers_.find(id);
  return it != timers_.end() ? it->second : nullptr;
}

bool TimerManager::Initialize(HWND hwnd, TIMERPROC proc) {
  hwnd_ = hwnd;
  id_ = ::SetTimer(hwnd, 0, 1000 /*milliseconds*/, proc);

  return id_ != 0;
}

void TimerManager::InsertTimer(const Timer* timer) {
  timers_.insert({timer->id(), const_cast<Timer*>(timer)});
}

}  // namespace base
