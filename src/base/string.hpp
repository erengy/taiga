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

#include <QList>
#include <QString>
#include <string>
#include <string_view>
#include <vector>

using namespace Qt::Literals::StringLiterals;

int compareStrings(const std::string_view a, const std::string_view b,
                   Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive);
QString joinStrings(const std::vector<std::string>& list, QString placeholder = "?");
void removeHtmlTags(QString& str);
QString& replaceWholeWord(QString& str, const QString& before, const QString& after);

int toInt(const std::string& value);
std::string toStdString(const QString& s);
std::vector<std::string> toVector(const QStringList& list);
