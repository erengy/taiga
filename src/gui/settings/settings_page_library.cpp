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
#include "settings_page_library.hpp"

#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QListWidget>
#include <QPushButton>
#include <string>
#include <vector>

#include "taiga/settings.hpp"
#include "ui_settings_dialog.h"

namespace gui {

SettingsPageLibrary::SettingsPageLibrary(Ui::SettingsDialog* ui, QDialog* dialog)
    : SettingsPage(ui, dialog) {
  connect(ui_->libraryAddFolderButton, &QPushButton::clicked, this, [this]() {
    const auto folder =
        QFileDialog::getExistingDirectory(dialog_, tr("Select anime folder"), QString{});
    if (folder.isEmpty()) return;

    const auto path = QDir::toNativeSeparators(QDir::cleanPath(folder));
    if (ui_->libraryFoldersList->findItems(path, Qt::MatchFixedString).isEmpty()) {
      ui_->libraryFoldersList->addItem(path);
    }
  });

  connect(ui_->libraryRemoveFolderButton, &QPushButton::clicked, this,
          [this]() { qDeleteAll(ui_->libraryFoldersList->selectedItems()); });

  connect(ui_->libraryFoldersList, &QListWidget::itemSelectionChanged, this, [this]() {
    ui_->libraryRemoveFolderButton->setEnabled(!ui_->libraryFoldersList->selectedItems().isEmpty());
  });
}

void SettingsPageLibrary::load() {
  for (const auto& folder : taiga::settings.libraryFolders()) {
    ui_->libraryFoldersList->addItem(QDir::toNativeSeparators(QString::fromStdString(folder)));
  }
}

void SettingsPageLibrary::apply() const {
  std::vector<std::string> folders;
  for (int i = 0; i < ui_->libraryFoldersList->count(); ++i) {
    folders.push_back(ui_->libraryFoldersList->item(i)->text().toStdString());
  }
  taiga::settings.setLibraryFolders(std::move(folders));
}

}  // namespace gui
