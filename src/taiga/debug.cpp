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

#include "base/string.h"
#include "library/anime_db.h"
#include "taiga/debug.h"
#include "ui/dlg/dlg_main.h"
#include "ui/dialog.h"

namespace debug {

Tester::Tester()
    : t0_(clock_t::duration::zero()) {
}

void Tester::Start() {
  t0_ = clock_t::now();
}

void Tester::Stop(std::wstring str, bool display_result) {
  using duration_t =
      std::chrono::duration<float, std::chrono::milliseconds::period>;

  const auto now = clock_t::now();
  const auto duration = std::chrono::duration_cast<duration_t>(now - t0_);
  t0_ = now;

  if (display_result) {
    str = ToWstr(duration.count(), 2) + L"ms | Text: [" + str + L"]";
    ui::DlgMain.SetText(str);
  }
}

////////////////////////////////////////////////////////////////////////////////

void Print(std::wstring text) {
#ifdef _DEBUG
  ::OutputDebugString(text.c_str());
#else
  UNREFERENCED_PARAMETER(text);
#endif
}

void Test() {
  // Define variables
  std::wstring str;

  // Start ticking
  Tester test;
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
  test.Stop(str, true);
}

} // namespace debug
