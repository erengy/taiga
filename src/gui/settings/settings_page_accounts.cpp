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
#include <QGroupBox>
#include <QLineEdit>
#include <algorithm>
#include <string>

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

}  // namespace

SettingsPageAccounts::SettingsPageAccounts(Ui::SettingsDialog* ui, QDialog* dialog)
    : SettingsPage(ui, dialog) {
  using sync::ServiceId;

  for (const auto id : {ServiceId::AniList, ServiceId::Kitsu, ServiceId::MyAnimeList}) {
    ui_->serviceComboBox->addItem(sync::serviceName(id), sync::serviceSlug(id));
  }

  connect(ui_->serviceComboBox, &QComboBox::currentIndexChanged, this,
          [this]() { updateVisibleGroup(); });
}

void SettingsPageAccounts::load() {
  const auto slug = QString::fromStdString(taiga::settings.service());
  ui_->serviceComboBox->setCurrentIndex(std::max(0, ui_->serviceComboBox->findData(slug)));
  updateVisibleGroup();

  ui_->syncEnabledCheckBox->setChecked(taiga::settings.syncEnabled());

  const auto& accounts = taiga::accounts;

  ui_->anilistUsernameLineEdit->setText(toQString(accounts.anilistUsername()));
  ui_->anilistTokenLineEdit->setText(toQString(accounts.anilistToken()));

  ui_->kitsuEmailLineEdit->setText(toQString(accounts.kitsuEmail()));
  ui_->kitsuUsernameLineEdit->setText(toQString(accounts.kitsuUsername()));
  ui_->kitsuPasswordLineEdit->setText(toQString(accounts.kitsuPassword()));
  ui_->kitsuAccessTokenLineEdit->setText(toQString(accounts.kitsuAccessToken()));
  ui_->kitsuRefreshTokenLineEdit->setText(toQString(accounts.kitsuRefreshToken()));

  ui_->myanimelistUsernameLineEdit->setText(toQString(accounts.myanimelistUsername()));
  ui_->myanimelistAccessTokenLineEdit->setText(toQString(accounts.myanimelistAccessToken()));
  ui_->myanimelistRefreshTokenLineEdit->setText(toQString(accounts.myanimelistRefreshToken()));
}

void SettingsPageAccounts::apply() const {
  taiga::settings.setService(ui_->serviceComboBox->currentData().toString().toStdString());
  taiga::settings.setSyncEnabled(ui_->syncEnabledCheckBox->isChecked());

  auto& accounts = taiga::accounts;

  accounts.setAnilistUsername(toStdString(ui_->anilistUsernameLineEdit));
  accounts.setAnilistToken(toStdString(ui_->anilistTokenLineEdit));

  accounts.setKitsuEmail(toStdString(ui_->kitsuEmailLineEdit));
  accounts.setKitsuUsername(toStdString(ui_->kitsuUsernameLineEdit));
  accounts.setKitsuPassword(ui_->kitsuPasswordLineEdit->text().toStdString());
  accounts.setKitsuAccessToken(toStdString(ui_->kitsuAccessTokenLineEdit));
  accounts.setKitsuRefreshToken(toStdString(ui_->kitsuRefreshTokenLineEdit));

  accounts.setMyanimelistUsername(toStdString(ui_->myanimelistUsernameLineEdit));
  accounts.setMyanimelistAccessToken(toStdString(ui_->myanimelistAccessTokenLineEdit));
  accounts.setMyanimelistRefreshToken(toStdString(ui_->myanimelistRefreshTokenLineEdit));
}

void SettingsPageAccounts::updateVisibleGroup() {
  using sync::ServiceId;

  const auto slug = ui_->serviceComboBox->currentData().toString();
  ui_->anilistGroupBox->setVisible(slug == sync::serviceSlug(ServiceId::AniList));
  ui_->kitsuGroupBox->setVisible(slug == sync::serviceSlug(ServiceId::Kitsu));
  ui_->myanimelistGroupBox->setVisible(slug == sync::serviceSlug(ServiceId::MyAnimeList));
}

}  // namespace gui
