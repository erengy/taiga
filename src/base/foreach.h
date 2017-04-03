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

#pragma once

// iterator
#define foreach_(item, container) \
    for (auto item = (container).begin(); item != (container).end(); ++item)

// reverse iterator
#define foreach_r_(item, container) \
    for (auto item = (container).rbegin(); item != (container).rend(); ++item)

// const iterator
#define foreach_c_(item, container) \
    for (auto item = (container).cbegin(); item != (container).cend(); ++item)

// const reverse iterator
#define foreach_cr_(item, container) \
    for (auto item = (container).crbegin(); item != (container).crend(); ++item)
