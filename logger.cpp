/*
** Taiga, a lightweight client for MyAnimeList
** Copyright (C) 2010-2012, Eren Okka
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

#include "std.h"
#include <iostream>
#include <fstream>
#include "logger.h"
#include "string.h"
#include "time.h"

const char* SeverityLevels[] = {
  "Emergency",
  "Alert",
  "Critical",
  "Error",
  "Warning",
  "Notice",
  "Informational",
  "Debug"
};

class Logger Logger;

Logger::Logger()
    : severity_level_(LevelDebug) {
}

void Logger::Log(int severity_level, const wstring& file, int line,
                 const wstring& function, const wstring& text) {
  critical_section_.Enter();

  if (severity_level <= severity_level_) {
    string output_text;

    output_text += ToANSI(wstring(GetDate()) + L" " + GetTime() + L" ");
    output_text += "[" + string(SeverityLevels[severity_level]) + "] ";
    output_text += ToANSI(file + L":" + ToWstr(line) + L" " + function + L" | ");
    output_text += ToANSI(text + L"\r\n");

#ifdef _DEBUG
    OutputDebugStringA(output_text.c_str());
#endif

    if (!output_path_.empty()) {
      std::ofstream stream;
      stream.open(output_path_, std::ofstream::app |
                                std::ios::binary |
                                std::ofstream::out);
      if (stream.is_open()) {
        stream.write(output_text.c_str(), output_text.size());
        stream.close();
      }
    }
  }

  critical_section_.Leave();
}

void Logger::SetOutputPath(const wstring& path) {
  output_path_ = path;
}

void Logger::SetSeverityLevel(int severity_level) {
  severity_level_ = severity_level;
}