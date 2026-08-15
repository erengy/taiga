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

#include <QDebug>
#include <QString>

namespace base {

void initLogging(const QString& path, const QtMsgType minType);

}  // namespace base

// Redefining macros to capture context outside of debug builds and to disable
// automatic quotation of strings.
#undef qDebug
#define qDebug QMessageLogger(__FILE__, __LINE__, __FUNCTION__).debug().noquote
#undef qInfo
#define qInfo QMessageLogger(__FILE__, __LINE__, __FUNCTION__).info().noquote
#undef qWarning
#define qWarning QMessageLogger(__FILE__, __LINE__, __FUNCTION__).warning().noquote
#undef qCritical
#define qCritical QMessageLogger(__FILE__, __LINE__, __FUNCTION__).critical().noquote
