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
#include <QFile>
#include <QFileInfo>
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
  QDesktopServices::openUrl(QUrl::fromLocalFile(m_path));
}

void LibraryMenu::remove() const {
  const QFileInfo info(m_path);
  const bool isDir = info.isDir();

  const auto text =
      isDir ? tr("Do you want to delete this folder?") : tr("Do you want to delete this file?");
  const auto informativeText =
      isDir ? tr("\"%1\" and everything inside it will be moved to the trash.").arg(info.fileName())
            : tr("\"%1\" will be moved to the trash.").arg(info.fileName());

  if (!confirm(parentWidget(), text, informativeText, tr("Delete"))) return;

  if (!QFile::moveToTrash(m_path)) {
    QMessageBox::warning(parentWidget(), tr("Delete"), tr("Could not delete \"%1\".").arg(m_path));
  }
}

void LibraryMenu::rename() const {
  // @TODO
}

void LibraryMenu::viewDetails() const {
  const auto item = anime::db.item(m_anime_id);
  if (!item) return;

  MediaDialog::show(parentWidget(), MediaDialogPage::Details, *item);
}

}  // namespace gui
