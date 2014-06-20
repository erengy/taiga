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

#include "win_main.h"
#include "win_thread.h"

namespace win {

////////////////////////////////////////////////////////////////////////////////

Thread::Thread()
    : thread_(nullptr),
      thread_id_(0) {
}

Thread::~Thread() {
  CloseThreadHandle();
}

bool Thread::CloseThreadHandle() {
  BOOL value = TRUE;

  if (thread_) {
    value = ::CloseHandle(thread_);
    thread_ = nullptr;
  }

  return value != FALSE;
}

bool Thread::CreateThread(LPSECURITY_ATTRIBUTES thread_attributes,
                          SIZE_T stack_size,
                          DWORD creation_flags) {
  thread_ = ::CreateThread(thread_attributes, stack_size, ThreadProcStatic,
                           this, creation_flags, &thread_id_);

  return thread_ != nullptr;
}

HANDLE Thread::GetThreadHandle() const {
  return thread_;
}

DWORD Thread::GetThreadId() const {
  return thread_id_;
}

DWORD Thread::ThreadProc() {
  return 0;
}

DWORD WINAPI Thread::ThreadProcStatic(LPVOID lpParam) {
  Thread* thread = reinterpret_cast<Thread*>(lpParam);
  return thread->ThreadProc();
}

////////////////////////////////////////////////////////////////////////////////

CriticalSection::CriticalSection() {
  ::InitializeCriticalSectionAndSpinCount(&critical_section_, 0);
}

CriticalSection::~CriticalSection() {
  ::DeleteCriticalSection(&critical_section_);
}

void CriticalSection::Enter() {
  ::EnterCriticalSection(&critical_section_);
}

void CriticalSection::Leave() {
  ::LeaveCriticalSection(&critical_section_);
}

bool CriticalSection::TryEnter() {
  return ::TryEnterCriticalSection(&critical_section_) != FALSE;
}

void CriticalSection::Wait() {
  while (!TryEnter())
    ::Sleep(1);
}

////////////////////////////////////////////////////////////////////////////////

Event::Event()
    : event_(nullptr) {
}

Event::~Event() {
  if (event_)
    ::CloseHandle(event_);
}

HANDLE Event::Create(LPSECURITY_ATTRIBUTES event_attributes, BOOL manual_reset,
                     BOOL initial_state, LPCTSTR name) {
  event_ = ::CreateEvent(event_attributes, manual_reset, initial_state, name);

  return event_;
}

////////////////////////////////////////////////////////////////////////////////

Lock::Lock(CriticalSection& critical_section)
    : critical_section_(critical_section) {
  critical_section_.Enter();
}

Lock::~Lock() {
  critical_section_.Leave();
}

////////////////////////////////////////////////////////////////////////////////

Mutex::Mutex()
    : mutex_(nullptr) {
}

Mutex::~Mutex() {
  if (mutex_)
    ::CloseHandle(mutex_);
}

HANDLE Mutex::Create(LPSECURITY_ATTRIBUTES mutex_attributes,
                     BOOL initial_owner, LPCTSTR name) {
  mutex_ = ::CreateMutex(mutex_attributes, initial_owner, name);

  return mutex_;
}

HANDLE Mutex::Open(DWORD desired_access, BOOL inherit_handle, LPCTSTR name) {
  mutex_ = ::OpenMutex(desired_access, inherit_handle, name);

  return mutex_;
}

bool Mutex::Release() {
  BOOL value = TRUE;

  if (mutex_) {
    value = ::ReleaseMutex(mutex_);
    mutex_ = nullptr;
  }

  return value != FALSE;
}

}  // namespace win