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

// =============================================================================

CThread::CThread() : 
  m_dwThreadId(0), m_hThread(NULL)
{
}

CThread::~CThread() {
  CloseThreadHandle();
}

bool CThread::CloseThreadHandle() {
  BOOL value = TRUE;
  if (m_hThread) {
    value = ::CloseHandle(m_hThread);
    m_hThread = NULL;
  }
  return value != FALSE;
}

bool CThread::CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, DWORD dwCreationFlags) {
  m_hThread = ::CreateThread(lpThreadAttributes, dwStackSize, ThreadProcStatic, this, dwCreationFlags, &m_dwThreadId);
  return m_hThread != NULL;
}

DWORD WINAPI CThread::ThreadProcStatic(LPVOID lpParam) {
  CThread* pThread = reinterpret_cast<CThread*>(lpParam);
  return pThread->ThreadProc();
}

// =============================================================================

CCriticalSection::CCriticalSection() {
  ::InitializeCriticalSectionAndSpinCount(&m_CriticalSection, 0);
}

CCriticalSection::~CCriticalSection() {
  ::DeleteCriticalSection(&m_CriticalSection);
}

void CCriticalSection::Enter() {
  ::EnterCriticalSection(&m_CriticalSection);
}

void CCriticalSection::Leave() {
  ::LeaveCriticalSection(&m_CriticalSection);
}

bool CCriticalSection::TryEnter() {
  return ::TryEnterCriticalSection(&m_CriticalSection) != FALSE;
}

void CCriticalSection::Wait() {
  while (!TryEnter()) {
    ::Sleep(1);
  }
}

// =============================================================================

CEvent::CEvent() :
  m_hEvent(NULL)
{
}

CEvent::~CEvent() {
  if (m_hEvent) ::CloseHandle(m_hEvent);
}

HANDLE CEvent::Create(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName) {
  m_hEvent = ::CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);
  return m_hEvent;
}

// =============================================================================

CMutex::CMutex() :
  m_hMutex(NULL)
{
}

CMutex::~CMutex() {
  if (m_hMutex) ::CloseHandle(m_hMutex);
}

HANDLE CMutex::Create(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTSTR lpName) {
  m_hMutex = ::CreateMutex(lpMutexAttributes, bInitialOwner, lpName);
  return m_hMutex;
}

HANDLE CMutex::Open(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName) {
  m_hMutex = ::OpenMutex(dwDesiredAccess, bInheritHandle, lpName);
  return m_hMutex;
}

bool CMutex::Release() {
  BOOL value = TRUE;
  if (m_hMutex) {
    BOOL value = ::ReleaseMutex(m_hMutex);
    m_hMutex = NULL;
  }
  return value != FALSE;
}