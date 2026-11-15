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

#include <QColor>
#include <QHash>
#include <QIcon>
#include <QObject>

namespace gui {

// Colors that differ between light and dark themes. Also used for `@name` placeholders in
// theme.qss.
struct ThemeColors {
  QColor background;
  QColor border;
  QColor splitter;
  QColor itemAlternate;
  QColor itemHover;
  QColor itemSelected;
  QColor itemSelectedInactive;
  QColor gridLine;
  QColor secondaryText;
  QColor progress;
  QColor success;
  QColor warning;
};

class Theme final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(Theme)

public:
  explicit Theme(QObject* parent);

  const QIcon& getIcon(const QString& key, const QString& extension = QString{u"svg"},
                       bool useSvgIconEngine = true);
  void initStyle();
  bool isDark() const;
  bool isFusion() const;

  const ThemeColors& colors() const;

  // Same for both themes, for things drawn outside the app (e.g. the tray icon).
  static QColor errorColor();
  static QColor successColor();

private:
  void applyStylesheet() const;
  QString readStylesheet(const QString& name) const;

  QHash<QString, QIcon> icons_;
  bool isFusion_ = false;
};

Theme* theme();

}  // namespace gui
