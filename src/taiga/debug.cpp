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

#include <chrono>

#include "taiga/debug.h"

#include "base/format.h"
#include "base/log.h"
#include "ui/dlg/dlg_main.h"

namespace taiga::debug {

class Tester {
public:
  using clock_t = std::chrono::steady_clock;
  using duration_t =
      std::chrono::duration<float, std::chrono::milliseconds::period>;

  void Stop(std::wstring str) {
    const auto duration =
        std::chrono::duration_cast<duration_t>(clock_t::now() - t0_);

    str = L"{:.2f}ms | [{}]"_format(duration.count(), str);
    LOGD(str);
    ui::DlgMain.SetText(str);
  }

private:
  clock_t::time_point t0_{clock_t::now()};
};

////////////////////////////////////////////////////////////////////////////////

void Test() {
  std::wstring str;

  Tester tester;

  for (int i = 0; i < 10000; ++i) {
    // Do some tests here
    //       ___
    //      {o,o}
    //      |)__)
    //     --"-"--
    //      O RLY?
  }

  tester.Stop(str);
}

}  // namespace taiga::debug
