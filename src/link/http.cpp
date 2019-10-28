/*
** Taiga
** Copyright (C) 2010-2019, Eren Okka
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

#include "link/http.h"

#include "base/string.h"
#include "taiga/http_new.h"

namespace link::http {

bool Send(const std::wstring& url, const std::wstring& data) {
  if (url.empty() || data.empty())
    return false;

  taiga::http::Request request;
  request.set_method("POST");
  request.set_target(WstrToStr(url));
  request.set_header("Content-Type", "application/x-www-form-urlencoded");
  request.set_body(hypr::Body{WstrToStr(data)});

  taiga::http::Send(request, nullptr, nullptr);

  return true;
}

}  // namespace link::http
