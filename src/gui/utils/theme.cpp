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

#include "theme.hpp"

#include <QApplication>
#include <QFile>
#include <QStyleHints>

#include "gui/utils/svg_icon_engine.hpp"

namespace gui {

Theme::Theme() : QObject() {
}

const QIcon& Theme::getIcon(const QString& key, const QString& extension, bool useSvgIconEngine) {
  if (!icons_.contains(key)) {
    if (extension == "svg" && useSvgIconEngine) {
      icons_[key] = QIcon(new SvgIconEngine(key));
    } else {
      icons_[key] = QIcon(u":/icons/%1.%2"_qs.arg(key, extension));
    }
  }

  return icons_[key];
}

bool Theme::isDark(QApplication* application) const {
  return application->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

QString Theme::readStylesheet(const QString& name) const {
  QFile styleFile(u":/styles/%1.qss"_qs.arg(name));
  styleFile.open(QFile::ReadOnly);
  return styleFile.readAll();
}

}  // namespace gui
