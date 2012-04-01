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

#ifndef WIN_THREAD_H
#define WIN_THREAD_H

#include "win_main.h"

// =============================================================================

class CThread {
public:
  CThread();
  virtual ~CThread();

  virtual DWORD ThreadProc() { return 0; }

  bool CloseThreadHandle();
  bool CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, 
    SIZE_T dwStackSize, DWORD dwCreationFlags);
  
  HANDLE GetThreadHandle() const { return m_hThread; }
  int GetThreadId() const { return m_dwThreadId; }

private:
  static DWORD WINAPI ThreadProcStatic(LPVOID pCThread);

  HANDLE m_hThread;
  DWORD m_dwThreadId;
};

// =============================================================================

class CCriticalSection {
public:
  CCriticalSection();
  virtual ~CCriticalSection();

  void Enter();
  void Leave();
  bool TryEnter();
  void Wait();

private:
  CRITICAL_SECTION m_CriticalSection;
};

// =============================================================================

class CEvent {
public:
  CEvent();
  virtual ~CEvent();

  HANDLE Create(LPSECURITY_ATTRIBUTES lpEventAttributes, 
    BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName);

private:
  HANDLE m_hEvent;
};

// =============================================================================

class CMutex {
public:
  CMutex();
  virtual ~CMutex();

  HANDLE Create(LPSECURITY_ATTRIBUTES lpMutexAttributes, 
    BOOL bInitialOwner, LPCTSTR lpName);
  HANDLE Open(DWORD dwDesiredAccess, 
    BOOL bInheritHandle, LPCTSTR lpName);
  bool Release();

private:
  HANDLE m_hMutex;
};

#endif // WIN_THREAD_H