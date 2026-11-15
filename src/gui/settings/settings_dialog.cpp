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

#include "base/string.hpp"
#include "gui/settings/settings_page_accounts.hpp"
#include "gui/settings/settings_page_advanced.hpp"
#include "gui/settings/settings_page_application.hpp"
#include "gui/settings/settings_page_library.hpp"
#include "gui/settings/settings_page_media_players.hpp"
#include "gui/settings/settings_page_recognition.hpp"
#include "gui/settings/settings_page_sharing.hpp"
#include "gui/settings/settings_page_streaming.hpp"
#include "gui/utils/theme.hpp"
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

  // Keep the scroll area transparent, so that it doesn't paint over the Mica background
  ui_->scrollArea->viewport()->setAutoFillBackground(false);
  ui_->scrollAreaWidgetContents->setAutoFillBackground(false);

  const auto addItem = [this](const QString& icon, const QString& text, QWidget* page = nullptr) {
    auto item = new QTreeWidgetItem(ui_->treeWidget, QStringList(text));
    item->setIcon(0, theme()->getIcon(icon));
    item->setSizeHint(0, QSize{0, 24});
    item->setData(0, kPageRole, QVariant::fromValue(page));
    if (!page) item->setDisabled(true);
    return item;
  };

  const auto addChild = [this](QTreeWidgetItem* parent, const QString& text,
                               QWidget* page = nullptr) {
    auto item = new QTreeWidgetItem(parent, QStringList(text));
    item->setData(0, kPageRole, QVariant::fromValue(page));
    if (!page) item->setDisabled(true);
  };

  addItem("account_circle", "Accounts", ui_->accountsPage);
  addItem("web_asset", "Application", ui_->applicationPage);
  addItem("list_alt", "Anime List");
  addItem("folder", "Library", ui_->libraryPage);
  {
    auto item = addItem("check_circle", "Recognition", ui_->recognitionPage);
    addChild(item, "Media players", ui_->mediaPlayersPage);
    addChild(item, "Streaming", ui_->streamingPage);
  }
  addItem("share", "Sharing", ui_->sharingPage);
  {
    auto item = addItem("rss_feed", "Torrents");
    addChild(item, "Downloads");
    addChild(item, "Filters");
  }
  {
    auto item = addItem("warning", "Advanced", ui_->advancedPage);
    addChild(item, "Cache");
  }

  ui_->treeWidget->expandAll();

  // Only the current page should contribute to the minimum size of the stacked widget, so that
  // the scroll area doesn't scroll pages that fit
  connect(ui_->stackedWidget, &QStackedWidget::currentChanged, this, [this](int index) {
    for (int i = 0; i < ui_->stackedWidget->count(); ++i) {
      const auto policy = i == index ? QSizePolicy::Preferred : QSizePolicy::Ignored;
      ui_->stackedWidget->widget(i)->setSizePolicy(policy, policy);
    }
  });

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

  pages_ = {
      // clang-format off
      new SettingsPageAccounts(ui_, this),
      new SettingsPageAdvanced(ui_, this),
      new SettingsPageApplication(ui_, this),
      new SettingsPageLibrary(ui_, this),
      new SettingsPageMediaPlayers(ui_, this),
      new SettingsPageRecognition(ui_, this),
      new SettingsPageSharing(ui_, this),
      new SettingsPageStreaming(ui_, this),
      // clang-format on
  };
  for (auto page : pages_) {
    page->load();
  }
}

void SettingsDialog::show(QWidget* parent) {
  auto dlg = new SettingsDialog(parent);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->setModal(true);
  dlg->QDialog::show();
}

void SettingsDialog::accept() {
  for (const auto page : pages_) {
    page->apply();
  }
  QDialog::accept();
}

}  // namespace gui
