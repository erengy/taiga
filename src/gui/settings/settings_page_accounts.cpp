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
#include <algorithm>

#include "sync/service.hpp"
#include "taiga/settings.hpp"
#include "ui_settings_dialog.h"

namespace gui {

SettingsPageAccounts::SettingsPageAccounts(Ui::SettingsDialog* ui, QDialog* dialog)
    : SettingsPage(ui, dialog) {
  using sync::ServiceId;

  for (const auto id : {ServiceId::AniList, ServiceId::Kitsu, ServiceId::MyAnimeList}) {
    ui_->serviceComboBox->addItem(sync::serviceName(id), sync::serviceSlug(id));
  }
}

void SettingsPageAccounts::load() {
  const auto slug = QString::fromStdString(taiga::settings.service());
  ui_->serviceComboBox->setCurrentIndex(std::max(0, ui_->serviceComboBox->findData(slug)));

  ui_->syncEnabledCheckBox->setChecked(taiga::settings.syncEnabled());
}

void SettingsPageAccounts::apply() const {
  taiga::settings.setService(ui_->serviceComboBox->currentData().toString().toStdString());
  taiga::settings.setSyncEnabled(ui_->syncEnabledCheckBox->isChecked());
}

}  // namespace gui
