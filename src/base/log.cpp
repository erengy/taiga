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

#include "log.hpp"

#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>

namespace base {

namespace {

QtMsgType g_minType = QtDebugMsg;
QString g_path;
QMutex g_mutex;
QtMessageHandler g_defaultHandler = nullptr;

// QtMsgType's declaration order doesn't match severity.
int rank(const QtMsgType type) {
  // clang-format off
  switch (type) {
    case QtDebugMsg: return 0;
    case QtInfoMsg: return 1;
    case QtWarningMsg: return 2;
    case QtCriticalMsg: return 3;
    case QtFatalMsg: return 4;
  }
  // clang-format on
  return 0;
}

void writeToFile(const QString& text) {
  if (g_path.isEmpty()) return;

  QFile file{g_path};
  if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    file.write(text.toUtf8());
    file.write("\n");
  }
}

void messageHandler(const QtMsgType type, const QMessageLogContext& context,
                    const QString& message) {
  QMutexLocker lock{&g_mutex};

  // Cannot use QT_NO_DEBUG_OUTPUT since the threshold is a runtime choice
  // (i.e. `--debug` command line argument).
  if (rank(type) < rank(g_minType)) return;

  // Shorten the file path to a basename before formatting/forwarding.
  const auto fileInfo = QFileInfo{QString::fromUtf8(context.file ? context.file : "")};
  const auto fileName = fileInfo.fileName().toUtf8();
  const QMessageLogContext shortContext{
      fileName.constData(),
      context.line,
      context.function,
      context.category,
  };

  writeToFile(qFormatLogMessage(type, shortContext, message));

  if (g_defaultHandler) {
    g_defaultHandler(type, shortContext, message);
  }
}

}  // namespace

void initLogging(const QString& path, const QtMsgType minType) {
  g_path = path;
  g_minType = minType;

  qSetMessagePattern(QStringLiteral(
      "%{time yyyy-MM-dd HH:mm:ss} [%{type}] %{file}:%{line} %{function} | %{message}"));

  g_defaultHandler = qInstallMessageHandler(messageHandler);
}

}  // namespace base
