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

#pragma once

#include <string>

#include "track/feed_filter.h"

namespace track::util {

enum class Shortcode {
  Action,
  Element,
  Match,
  Operator,
  Option,
};

std::wstring CreateNameFromConditions(const FeedFilter& filter);
std::wstring TranslateCondition(const FeedFilterCondition& condition);
std::wstring TranslateConditions(const FeedFilter& filter, const size_t index);
std::wstring TranslateElement(const int element);
std::wstring TranslateOperator(const int op);
std::wstring TranslateValue(const FeedFilterCondition& condition);
std::wstring TranslateMatching(const int match);
std::wstring TranslateAction(const int action);
std::wstring TranslateOption(const int option);

std::wstring GetShortcodeFromIndex(const Shortcode type, const int index);
int GetIndexFromShortcode(const Shortcode type, const std::wstring& shortcode);

}  // namespace track::util
