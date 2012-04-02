/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

namespace win32 {

// =============================================================================

Thread::Thread() : 
  m_dwThreadId(0), m_hThread(NULL)
{
}

Thread::~Thread() {
  CloseThreadHandle();
}

bool Thread::CloseThreadHandle() {
  BOOL value = TRUE;
  if (m_hThread) {
    value = ::CloseHandle(m_hThread);
    m_hThread = NULL;
  }
  return value != FALSE;
}

bool Thread::CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, DWORD dwCreationFlags) {
  m_hThread = ::CreateThread(lpThreadAttributes, dwStackSize, ThreadProcStatic, this, dwCreationFlags, &m_dwThreadId);
  return m_hThread != NULL;
}

DWORD WINAPI Thread::ThreadProcStatic(LPVOID lpParam) {
  Thread* pThread = reinterpret_cast<Thread*>(lpParam);
  return pThread->ThreadProc();
}

// =============================================================================

CriticalSection::CriticalSection() {
  ::InitializeCriticalSectionAndSpinCount(&m_CriticalSection, 0);
}

CriticalSection::~CriticalSection() {
  ::DeleteCriticalSection(&m_CriticalSection);
}

void CriticalSection::Enter() {
  ::EnterCriticalSection(&m_CriticalSection);
}

void CriticalSection::Leave() {
  ::LeaveCriticalSection(&m_CriticalSection);
}

bool CriticalSection::TryEnter() {
  return ::TryEnterCriticalSection(&m_CriticalSection) != FALSE;
}

void CriticalSection::Wait() {
  while (!TryEnter()) {
    ::Sleep(1);
  }
}

// =============================================================================

Event::Event() :
  m_hEvent(NULL)
{
}

Event::~Event() {
  if (m_hEvent) ::CloseHandle(m_hEvent);
}

HANDLE Event::Create(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName) {
  m_hEvent = ::CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);
  return m_hEvent;
}

// =============================================================================

Mutex::Mutex() :
  m_hMutex(NULL)
{
}

Mutex::~Mutex() {
  if (m_hMutex) ::CloseHandle(m_hMutex);
}

HANDLE Mutex::Create(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTSTR lpName) {
  m_hMutex = ::CreateMutex(lpMutexAttributes, bInitialOwner, lpName);
  return m_hMutex;
}

HANDLE Mutex::Open(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName) {
  m_hMutex = ::OpenMutex(dwDesiredAccess, bInheritHandle, lpName);
  return m_hMutex;
}

bool Mutex::Release() {
  BOOL value = TRUE;
  if (m_hMutex) {
    BOOL value = ::ReleaseMutex(m_hMutex);
    m_hMutex = NULL;
  }
  return value != FALSE;
}

} // namespace win32