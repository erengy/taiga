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
#include "settings_page_accounts.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QGroupBox>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <algorithm>
#include <memory>
#include <string>

#include "base/string.hpp"
#include "sync/anilist/anilist.hpp"
#include "sync/anilist/anilist_utils.hpp"
#include "sync/myanimelist/myanimelist.hpp"
#include "sync/myanimelist/myanimelist_utils.hpp"
#include "sync/service.hpp"
#include "taiga/accounts.hpp"
#include "taiga/settings.hpp"
#include "ui_settings_dialog.h"

namespace gui {

namespace {

QString toQString(const std::string& s) {
  return QString::fromStdString(s);
}

std::string toStdString(const QLineEdit* lineEdit) {
  return lineEdit->text().trimmed().toStdString();
}

QString authorizeButtonText(const std::string& token) {
  return token.empty() ? SettingsPageAccounts::tr("Authorize...")
                       : SettingsPageAccounts::tr("Re-authorize...");
}

}  // namespace

SettingsPageAccounts::SettingsPageAccounts(Ui::SettingsDialog* ui, QDialog* dialog)
    : SettingsPage(ui, dialog) {
  using sync::ServiceId;

  for (const auto id : {ServiceId::AniList, ServiceId::Kitsu, ServiceId::MyAnimeList}) {
    ui_->serviceComboBox->addItem(sync::serviceName(id), sync::serviceSlug(id));
  }

  connect(ui_->serviceComboBox, &QComboBox::currentIndexChanged, this,
          [this]() { updateVisibleGroup(); });

  connect(ui_->anilistAuthorizeButton, &QPushButton::clicked, this,
          &SettingsPageAccounts::authorizeAnilist);
  connect(ui_->myanimelistAuthorizeButton, &QPushButton::clicked, this,
          &SettingsPageAccounts::authorizeMyanimelist);
}

void SettingsPageAccounts::load() {
  const auto slug = QString::fromStdString(taiga::settings()->syncService());
  ui_->serviceComboBox->setCurrentIndex(std::max(0, ui_->serviceComboBox->findData(slug)));
  updateVisibleGroup();

  ui_->syncEnabledCheckBox->setChecked(taiga::settings()->syncEnabled());

  const auto& accounts = *taiga::accounts();

  ui_->anilistUsernameLineEdit->setText(toQString(accounts.anilistUsername()));
  ui_->anilistAuthorizeButton->setText(authorizeButtonText(accounts.anilistToken()));

  ui_->kitsuEmailLineEdit->setText(toQString(accounts.kitsuEmail()));
  ui_->kitsuUsernameLineEdit->setText(toQString(accounts.kitsuUsername()));
  ui_->kitsuPasswordLineEdit->setText(toQString(accounts.kitsuPassword()));

  ui_->myanimelistUsernameLineEdit->setText(toQString(accounts.myanimelistUsername()));
  ui_->myanimelistAuthorizeButton->setText(authorizeButtonText(accounts.myanimelistAccessToken()));
}

void SettingsPageAccounts::apply() const {
  taiga::settings()->setSyncService(ui_->serviceComboBox->currentData().toString().toStdString());
  taiga::settings()->setSyncEnabled(ui_->syncEnabledCheckBox->isChecked());

  auto& accounts = *taiga::accounts();

  accounts.setAnilistUsername(toStdString(ui_->anilistUsernameLineEdit));

  accounts.setKitsuCredentials(toStdString(ui_->kitsuEmailLineEdit),
                               toStdString(ui_->kitsuUsernameLineEdit),
                               ui_->kitsuPasswordLineEdit->text().toStdString());

  accounts.setMyanimelistUsername(toStdString(ui_->myanimelistUsernameLineEdit));
}

void SettingsPageAccounts::updateVisibleGroup() {
  using sync::ServiceId;

  const auto slug = ui_->serviceComboBox->currentData().toString();
  ui_->anilistGroupBox->setVisible(slug == sync::serviceSlug(ServiceId::AniList));
  ui_->kitsuGroupBox->setVisible(slug == sync::serviceSlug(ServiceId::Kitsu));
  ui_->myanimelistGroupBox->setVisible(slug == sync::serviceSlug(ServiceId::MyAnimeList));
}

void SettingsPageAccounts::authorizeAnilist() {
  QDesktopServices::openUrl(QUrl{toQString(sync::anilist::requestTokenUrl())});

  const auto token = promptAuthorizationCode(u"AniList"_s);
  if (token.isEmpty()) return;

  const auto service = sync::anilist::Service::instance();
  watchAuthorization(
      service, ui_->anilistAuthorizeButton, ui_->anilistAuthorizeStatusLabel, [this] {
        const auto& accounts = *taiga::accounts();
        ui_->anilistUsernameLineEdit->setText(toQString(accounts.anilistUsername()));
        ui_->anilistAuthorizeButton->setText(authorizeButtonText(accounts.anilistToken()));
        return accounts.anilistUsername();
      });
  service->authorize(token);
}

void SettingsPageAccounts::authorizeMyanimelist() {
  std::string codeVerifier;
  QDesktopServices::openUrl(QUrl{toQString(sync::myanimelist::authorizationCodeUrl(codeVerifier))});

  const auto code = promptAuthorizationCode(u"MyAnimeList"_s);
  if (code.isEmpty()) return;

  const auto service = sync::myanimelist::Service::instance();
  watchAuthorization(
      service, ui_->myanimelistAuthorizeButton, ui_->myanimelistAuthorizeStatusLabel, [this] {
        const auto& accounts = *taiga::accounts();
        ui_->myanimelistUsernameLineEdit->setText(toQString(accounts.myanimelistUsername()));
        ui_->myanimelistAuthorizeButton->setText(
            authorizeButtonText(accounts.myanimelistAccessToken()));
        return accounts.myanimelistUsername();
      });
  service->requestAccessToken(code, toQString(codeVerifier));
}

QString SettingsPageAccounts::promptAuthorizationCode(const QString& serviceName) const {
  bool ok = false;
  const auto code = QInputDialog::getText(
      dialog_, tr("%1 Authorization").arg(serviceName),
      tr("Please enter the code shown on the page after logging into %1:").arg(serviceName),
      QLineEdit::Normal, {}, &ok);
  return ok ? code.trimmed() : QString{};
}

void SettingsPageAccounts::watchAuthorization(sync::Service* service, QPushButton* button,
                                              QLabel* statusLabel,
                                              const std::function<std::string()>& onSuccess) {
  button->setEnabled(false);
  statusLabel->setText(tr("Authorizing..."));

  // Errors are reported before authentication completes, so we keep the last one to display it
  const auto lastError = std::make_shared<QString>();
  const auto errorConnection = std::make_shared<QMetaObject::Connection>();
  *errorConnection = connect(service, &sync::Service::errorOccurred, statusLabel,
                             [lastError](const QString& message) { *lastError = message; });

  connect(
      service, &sync::Service::authenticationCompleted, statusLabel,
      [=, this](bool authenticated) {
        disconnect(*errorConnection);
        button->setEnabled(true);
        if (authenticated) {
          statusLabel->setText(tr("Authorized as %1.").arg(toQString(onSuccess())));
          return;
        }
        // Error messages can be long, so they're shown in a message box rather than inline
        statusLabel->setText(tr("Authorization failed."));
        if (!lastError->isEmpty()) {
          QMessageBox::warning(
              dialog_, tr("%1 Authorization").arg(sync::serviceName(service->id())), *lastError);
        }
      },
      Qt::SingleShotConnection);
}

}  // namespace gui
