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

#ifndef TAIGA_API_H
#define TAIGA_API_H

#include "std.h"
#include "win32/win_main.h"

namespace anime {
class Episode;
}

class Api {
public:
  Api();
  virtual ~Api();

  void Announce(anime::Episode& episode);
  void BroadcastMessage(UINT message);

private:
  std::map<HWND, wstring> handles;

  UINT wm_attach;
  UINT wm_detach;
  UINT wm_ready;
  UINT wm_quit;

  class Window : public win32::Window {
  public:
    Window() {}
    virtual ~Window() {}
  private:
    void PreRegisterClass(WNDCLASSEX& wc);
    void PreCreate(CREATESTRUCT& cs);
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } window;
};

extern class Api TaigaApi;

#endif // TAIGA_API_H