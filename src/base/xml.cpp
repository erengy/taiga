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

#include "xml.hpp"

namespace base {

const QFile& XmlFileReader::file() const {
  return file_;
}

bool XmlFileReader::open(const QString& name, const std::function<void(QString&)>& preprocessor) {
  file_.setFileName(name);

  if (!file_.open(QIODevice::ReadOnly)) return false;

  QString data{file_.readAll()};
  if (preprocessor) preprocessor(data);

  QXmlStreamReader::addData(data);

  return true;
}

bool XmlFileReader::readElement(QAnyStringView name) {
  return QXmlStreamReader::readNextStartElement() && QXmlStreamReader::name() == name;
}

////////////////////////////////////////////////////////////////////////////////

const QFile& XmlFileWriter::file() const {
  return file_;
}

bool XmlFileWriter::open(const QString& name) {
  file_.setFileName(name);

  if (!file_.open(QIODevice::WriteOnly)) return false;

  QXmlStreamWriter::setDevice(&file_);

  return true;
}

void XmlFileWriter::writeNumberElement(QAnyStringView name, int number) {
  writeTextElement(name, QString::number(number));
}

}  // namespace base
