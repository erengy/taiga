/**
 * Taiga
 * Copyright (C) 2010-2026, Eren Okka
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

#pragma once

#include <QFileInfo>
#include <anitomy.hpp>
#include <string_view>

namespace track {
class Episode;
}

namespace track::recognition {

Episode parse(std::string_view input, const anitomy::Options options = {});
Episode parseFileInfo(const QFileInfo& info, const anitomy::Options options = {});

int identify(Episode& episode);

bool isVideoFile(const Episode& episode);

}  // namespace track::recognition
