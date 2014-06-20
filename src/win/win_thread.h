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

#ifndef TAIGA_WIN_THREAD_H
#define TAIGA_WIN_THREAD_H

#include "win_main.h"

namespace win {

class Thread {
public:
  Thread();
  virtual ~Thread();

  virtual DWORD ThreadProc();

  bool CloseThreadHandle();
  bool CreateThread(
      LPSECURITY_ATTRIBUTES thread_attributes,
      SIZE_T stack_size,
      DWORD creation_flags);

  HANDLE GetThreadHandle() const;
  DWORD GetThreadId() const;

private:
  static DWORD WINAPI ThreadProcStatic(LPVOID thread);

  HANDLE thread_;
  DWORD thread_id_;
};

////////////////////////////////////////////////////////////////////////////////

class CriticalSection {
public:
  CriticalSection();
  virtual ~CriticalSection();

  void Enter();
  void Leave();
  bool TryEnter();
  void Wait();

private:
  CRITICAL_SECTION critical_section_;
};

class Event {
public:
  Event();
  virtual ~Event();

  HANDLE Create(
      LPSECURITY_ATTRIBUTES event_attributes,
      BOOL manual_reset,
      BOOL initial_state,
      LPCTSTR name);

private:
  HANDLE event_;
};

class Lock {
public:
  Lock(CriticalSection& critical_section);
  virtual ~Lock();

private:
  CriticalSection& critical_section_;
};

class Mutex {
public:
  Mutex();
  virtual ~Mutex();

  HANDLE Create(
      LPSECURITY_ATTRIBUTES mutex_attributes,
      BOOL initial_owner,
      LPCTSTR name);
  HANDLE Open(
      DWORD desired_access,
      BOOL inherit_handle,
      LPCTSTR name);
  bool Release();

private:
  HANDLE mutex_;
};

}  // namespace win

#endif  // TAIGA_WIN_THREAD_H