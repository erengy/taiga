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

#include "windows.hpp"

#include <dwmapi.h>

namespace gui {

void enableDarkMode(HWND hwnd) {
  // Allow the window frame to be drawn in dark mode colors
  const BOOL value = TRUE;
  ::DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
}

void enableMicaBackground(HWND hwnd) {
  // Render the client area as a solid surface with no window border
  const MARGINS margins = {-1};
  ::DwmExtendFrameIntoClientArea(hwnd, &margins);

  // Enable transparency effects
  const BOOL value = TRUE;
  ::DwmSetWindowAttribute(hwnd, DWMWA_USE_HOSTBACKDROPBRUSH, &value, sizeof(value));

  // Use Mica as backdrop material
  const DWM_SYSTEMBACKDROP_TYPE backdrop_type = DWMSBT_MAINWINDOW;
  ::DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop_type, sizeof(backdrop_type));
}

}  // namespace gui
