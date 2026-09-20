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
#include "settings_page_application.hpp"

#include <QComboBox>
#include <algorithm>

#include "taiga/settings.hpp"
#include "ui_settings_dialog.h"

namespace gui {

SettingsPageApplication::SettingsPageApplication(Ui::SettingsDialog* ui, QDialog* dialog)
    : SettingsPage(ui, dialog) {}

void SettingsPageApplication::load() {
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

void SettingsPageApplication::apply() const {
  taiga::settings.setAppColorScheme(
      static_cast<Qt::ColorScheme>(ui_->colorSchemeComboBox->currentData().toInt()));
  taiga::settings.setTitleLanguage(
      static_cast<anime::TitleLanguage>(ui_->titleLanguageComboBox->currentData().toInt()));
}

}  // namespace gui
