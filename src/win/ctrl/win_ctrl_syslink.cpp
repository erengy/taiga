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

#include "win_ctrl.h"

namespace win {

SysLink::SysLink(HWND hwnd) {
  SetWindowHandle(hwnd);
}

void SysLink::SetItemState(int item, UINT states) {
  LITEM li = {0};

  li.iLink = item;
  li.mask = LIF_ITEMINDEX | LIF_STATE;
  li.state = states;
  li.stateMask = states;

  SendMessage(LM_SETITEM, 0, reinterpret_cast<LPARAM>(&li));
}

}  // namespace win