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

#include <iostream>
#include <fstream>

#include "foreach.h"
#include "log.h"
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

void Logger::Log(int severity_level, const std::wstring& file, int line,
                 const std::wstring& function, std::wstring text, bool raw) {
  win::Lock lock(critical_section_);

  if (severity_level <= severity_level_) {
    std::string output_text;

    if (raw) {
      output_text += WstrToStr(text);

    } else {
      Trim(text, L" \r\n");

      output_text +=
          WstrToStr(std::wstring(GetDate()) + L" " + GetTime() + L" ") +
          "[" + std::string(SeverityLevels[severity_level]) + "] " +
          WstrToStr(GetFileName(file) + L":" + ToWstr(line) + L" " + function + L" | ");

      std::string padding(output_text.length(), ' ');
      std::vector<std::wstring> lines;
      Split(text, L"\n", lines);
      foreach_(it, lines) {
        Trim(*it, L" \r");
        if (!it->empty()) {
          if (it != lines.begin())
            output_text += padding;
          output_text += WstrToStr(*it + L"\r\n");
        }
      }
    }

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
}

void Logger::SetOutputPath(const std::wstring& path) {
  output_path_ = path;
}

void Logger::SetSeverityLevel(int severity_level) {
  severity_level_ = severity_level;
}