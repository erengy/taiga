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

#include "std.h"
#include "common.h"
#include "debug.h"
#include "dlg/dlg_main.h"
#include "string.h"

// =============================================================================

DebugTester::DebugTester() : 
  m_Freq(0.0), m_Value(0)
{
}

void DebugTester::Start() {
  LARGE_INTEGER li;
  
  if (m_Freq == 0.0) {
    QueryPerformanceFrequency(&li);
    m_Freq = double(li.QuadPart) / 1000.0;
  }
  
  QueryPerformanceCounter(&li);
  m_Value = li.QuadPart;
}

void DebugTester::End(wstring str, BOOL show) {
  LARGE_INTEGER li;

  QueryPerformanceCounter(&li);
  double value = double(li.QuadPart - m_Value) / m_Freq;

  if (show) {
    str = ToWSTR(value, 2) + L" ms | Text: [" + str + L"]";
    MainDialog.SetText(str.c_str());
  }
}

// =============================================================================

void DebugPrint(wstring text) {
  #ifdef _DEBUG
  ::OutputDebugString(text.c_str());
  #else
  UNREFERENCED_PARAMETER(text);
  #endif
}

void DebugTest() {
  // Define variables
  wstring str;

  // Start ticking
  DebugTester test;
  test.Start();

  for (int i = 0; i < 10000; i++) {
    // Do some tests here
    //       ___
    //      {o,o}
    //      |)__)
    //     --"-"--
    //      O RLY?
  }

  // Show result
  test.End(str, 0);

  // Default action
  ExecuteAction(L"RecognitionTest");
}