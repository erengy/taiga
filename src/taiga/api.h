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

#ifndef TAIGA_TAIGA_API_H
#define TAIGA_TAIGA_API_H

#include <map>
#include <string>
#include <vector>

#include <windows/win/window.h>

namespace anime {
class Episode;
}

namespace taiga {

class Api {
public:
  Api();
  ~Api();

  void Announce(anime::Episode& episode);
  void BroadcastMessage(UINT message);
  void Create();

private:
  std::map<HWND, std::wstring> handles;

  UINT wm_attach_;
  UINT wm_detach_;
  UINT wm_ready_;
  UINT wm_quit_;

  class Window : public win::Window {
  private:
    void PreRegisterClass(WNDCLASSEX& wc);
    void PreCreate(CREATESTRUCT& cs);
    LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
  } window;
};

}  // namespace taiga

extern taiga::Api TaigaApi;

#endif  // TAIGA_TAIGA_API_H