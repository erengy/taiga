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

#include <QMenu>
#include <QString>

namespace gui {

class LibraryMenu final : public QMenu {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(LibraryMenu)

public:
  LibraryMenu(QWidget* parent, const QString& path, int anime_id);
  ~LibraryMenu() = default;

  void popup();

  static void openPath(const QString& path);
  static void deletePath(QWidget* parent, const QString& path);
  static void renamePath(QWidget* parent, const QString& path);

private slots:
  void open() const;
  void remove() const;
  void rename() const;
  void viewDetails() const;

private:
  int m_anime_id = 0;
  QString m_path;
};

}  // namespace gui
