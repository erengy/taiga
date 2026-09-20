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

#include "settings_dialog.hpp"

#include <algorithm>

#include "base/string.hpp"
#include "gui/utils/theme.hpp"
#include "taiga/settings.hpp"
#include "ui_settings_dialog.h"

#ifdef Q_OS_WINDOWS
#include "gui/platforms/windows.hpp"
#endif

namespace gui {

constexpr int kPageRole = Qt::UserRole;

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent), ui_(new Ui::SettingsDialog) {
  ui_->setupUi(this);

#ifdef Q_OS_WINDOWS
  enableMicaBackground(this);
#endif

  ui_->treeWidget->setIndentation(22);

  const auto add_item = [this](QString icon, QString text, QWidget* page = nullptr) {
    auto item = new QTreeWidgetItem(ui_->treeWidget, QStringList(text));
    item->setIcon(0, theme.getIcon(icon));
    item->setSizeHint(0, QSize{0, 24});
    item->setData(0, kPageRole, QVariant::fromValue(page));
    return item;
  };

  const auto add_child = [this](QTreeWidgetItem* parent, QString text) {
    new QTreeWidgetItem(parent, QStringList(text));
  };

  add_item("account_circle", "Accounts");
  add_item("web_asset", "Application", ui_->applicationPage);
  add_item("list_alt", "Anime List");
  add_item("folder", "Library");
  {
    auto item = add_item("check_circle", "Recognition");
    add_child(item, "Media players");
    add_child(item, "Streaming");
  }
  {
    auto item = add_item("share", "Sharing");
    add_child(item, "Discord");
    add_child(item, "HTTP");
    add_child(item, "mIRC");
  }
  {
    auto item = add_item("rss_feed", "Torrents");
    add_child(item, "Downloads");
    add_child(item, "Filters");
  }
  {
    auto item = add_item("warning", "Advanced");
    add_child(item, "Cache");
  }

  ui_->treeWidget->expandAll();

  connect(ui_->treeWidget, &QTreeWidget::currentItemChanged, this,
          [this](QTreeWidgetItem* current, QTreeWidgetItem*) {
            if (current) {
              auto text = current->text(0);
              if (current->parent()) {
                text = u"%1 / %2"_s.arg(current->parent()->text(0), text);
              }
              ui_->titleLabel->setText(text);

              auto page = current->data(0, kPageRole).value<QWidget*>();
              ui_->stackedWidget->setCurrentWidget(page ? page : ui_->todoPage);
            }
          });

  ui_->treeWidget->setCurrentItem(ui_->treeWidget->topLevelItem(0));

  load();
}

void SettingsDialog::show(QWidget* parent) {
  auto dlg = new SettingsDialog(parent);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->setModal(true);
  dlg->QDialog::show();
}

void SettingsDialog::accept() {
  apply();
  QDialog::accept();
}

void SettingsDialog::load() {
  // Application
  {
    auto combo = ui_->colorSchemeComboBox;
    combo->addItem(tr("System"), static_cast<int>(Qt::ColorScheme::Unknown));
    combo->addItem(tr("Light"), static_cast<int>(Qt::ColorScheme::Light));
    combo->addItem(tr("Dark"), static_cast<int>(Qt::ColorScheme::Dark));
    combo->setCurrentIndex(
        std::max(0, combo->findData(static_cast<int>(taiga::settings.appColorScheme()))));
  }
  {
    using anime::TitleLanguage;
    auto combo = ui_->titleLanguageComboBox;
    combo->addItem(tr("Romaji"), static_cast<int>(TitleLanguage::Romaji));
    combo->addItem(tr("English"), static_cast<int>(TitleLanguage::English));
    combo->addItem(tr("Native"), static_cast<int>(TitleLanguage::Native));
    combo->setCurrentIndex(
        std::max(0, combo->findData(static_cast<int>(taiga::settings.titleLanguage()))));
  }
}

void SettingsDialog::apply() const {
  // Application
  taiga::settings.setAppColorScheme(
      static_cast<Qt::ColorScheme>(ui_->colorSchemeComboBox->currentData().toInt()));
  taiga::settings.setTitleLanguage(
      static_cast<anime::TitleLanguage>(ui_->titleLanguageComboBox->currentData().toInt()));
}

}  // namespace gui
