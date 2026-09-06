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

#include "library_menu.hpp"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QUrl>

#include "gui/media/media_dialog.hpp"
#include "gui/media/media_menu.hpp"
#include "gui/utils/theme.hpp"
#include "gui/utils/widgets.hpp"
#include "media/anime_db.hpp"

namespace gui {

LibraryMenu::LibraryMenu(QWidget* parent, const QString& path, int anime_id)
    : QMenu(parent), m_path(path), m_anime_id(anime_id) {
  setAttribute(Qt::WA_DeleteOnClose);
}

void LibraryMenu::popup() {
  if (m_path.isEmpty()) return;

  addAction(QIcon(m_path), tr("Open"), tr("Enter"), this, &LibraryMenu::open);
  addSeparator();
  addAction(theme.getIcon("delete"), tr("Delete"), tr("Del"), this, &LibraryMenu::remove);
  addAction(theme.getIcon("edit"), tr("Rename"), tr("F2"), this, &LibraryMenu::rename);

  if (const auto item = anime::db.item(m_anime_id)) {
    addSeparator();
    addAction(theme.getIcon("info"), tr("Details"), this, &LibraryMenu::viewDetails);
  }

  QMenu::popup(QCursor::pos());
}

void LibraryMenu::open() const {
  openPath(m_path);
}

void LibraryMenu::remove() const {
  deletePath(parentWidget(), m_path);
}

void LibraryMenu::rename() const {
  renamePath(parentWidget(), m_path);
}

void LibraryMenu::openPath(const QString& path) {
  QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void LibraryMenu::deletePath(QWidget* parent, const QString& path) {
  const QFileInfo info(path);
  const bool isDir = info.isDir();

  const auto text =
      isDir ? tr("Do you want to delete this folder?") : tr("Do you want to delete this file?");
  const auto informativeText =
      isDir ? tr("\"%1\" and everything inside it will be moved to the trash.").arg(info.fileName())
            : tr("\"%1\" will be moved to the trash.").arg(info.fileName());

  if (!confirm(parent, text, informativeText, tr("Delete"))) return;

  if (!QFile::moveToTrash(path)) {
    QMessageBox::warning(parent, tr("Delete"), tr("Could not delete \"%1\".").arg(path));
  }
}

void LibraryMenu::renamePath(QWidget* parent, const QString& path) {
  const QFileInfo info(path);

  bool ok = false;
  const auto newName = QInputDialog::getText(parent, tr("Rename"), tr("New name:"),
                                             QLineEdit::Normal, info.fileName(), &ok);
  if (!ok || newName.isEmpty() || newName == info.fileName()) return;

  const auto newPath = info.dir().filePath(newName);

  if (!QDir().rename(path, newPath)) {
    QMessageBox::warning(parent, tr("Rename"), tr("Could not rename \"%1\".").arg(path));
  }
}

void LibraryMenu::viewDetails() const {
  const auto item = anime::db.item(m_anime_id);
  if (!item) return;

  MediaDialog::show(parentWidget(), MediaDialogPage::Details, *item);
}

}  // namespace gui
