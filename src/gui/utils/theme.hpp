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

#include <QHash>
#include <QIcon>
#include <QObject>

class QApplication;

namespace gui {

class Theme final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Theme)

public:
  Theme();

  const QIcon& getIcon(const QString& key, const QString& extension = u"svg"_qs,
                       bool useSvgIconEngine = true);
  bool isDark(QApplication* application) const;

private:
  QString readStylesheet(const QString& name) const;

  QHash<QString, QIcon> m_icons;
};

inline Theme theme;

}  // namespace gui
