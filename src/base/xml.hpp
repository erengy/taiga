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

#include <QFile>
#include <QString>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <functional>

namespace base {

class XmlFileReader : public QXmlStreamReader {
public:
  const QFile& file() const;

  bool open(const QString& name, const std::function<void(QString&)>& preprocessor = {});
  bool readElement(QAnyStringView name);

private:
  QFile file_;
};

class XmlFileWriter : public QXmlStreamWriter {
public:
  const QFile& file() const;

  bool open(const QString& name);
  void writeNumberElement(QAnyStringView name, int number);

private:
  QFile file_;
};

}  // namespace base
