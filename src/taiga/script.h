/*
** Taiga
** Copyright (C) 2010-2013, Eren Okka
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

#ifndef TAIGA_TAIGA_SCRIPT_H
#define TAIGA_TAIGA_SCRIPT_H

#include "base/std.h"

namespace anime {
class Episode;
}

wstring EvaluateFunction(const wstring& func_name, const wstring& func_body);

bool IsScriptFunction(const wstring& str);
bool IsScriptVariable(const wstring& str);

wstring ReplaceVariables(wstring str,
                         const anime::Episode& episode,
                         bool url_encode = false,
                         bool is_manual = false,
                         bool is_preview = false);

wstring EscapeScriptEntities(const wstring& str);
wstring UnescapeScriptEntities(const wstring& str);

#endif  // TAIGA_TAIGA_SCRIPT_H