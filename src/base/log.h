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

#ifndef TAIGA_BASE_LOG_H
#define TAIGA_BASE_LOG_H

#include <string>

#include "win/win_thread.h"

enum SeverityLevels {
  LevelEmergency,
  LevelAlert,
  LevelCritical,
  LevelError,
  LevelWarning,
  LevelNotice,
  LevelInformational,
  LevelDebug
};

class Logger {
public:
  Logger();
  virtual ~Logger() {}

  void Log(int severity_level, const std::wstring& file, int line,
           const std::wstring& function, std::wstring text, bool raw);
  void SetOutputPath(const std::wstring& path);
  void SetSeverityLevel(int severity_level);

  static std::wstring FormatError(DWORD error, LPCWSTR source = nullptr);

private:
  win::CriticalSection critical_section_;
  std::wstring output_path_;
  int severity_level_;
};

extern class Logger Logger;

#ifndef LOG
#define LOG(level, text) \
  Logger.Log(level, __FILEW__, __LINE__, __FUNCTIONW__, text, false)
#endif
#ifndef LOGR
#define LOGR(level, text) \
  Logger.Log(level, __FILEW__, __LINE__, __FUNCTIONW__, text, true)
#endif

#endif  // TAIGA_BASE_LOG_H