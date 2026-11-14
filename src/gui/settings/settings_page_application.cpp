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

#include <QCheckBox>
#include <QComboBox>
#include <QSystemTrayIcon>
#include <algorithm>

#include "base/string.hpp"
#include "taiga/settings.hpp"
#include "ui_settings_dialog.h"

namespace gui {

namespace {

QString titleExamples(const anime::TitleLanguage language) {
  switch (language) {
    default:
    case anime::TitleLanguage::Romaji:
      return u"Shingeki no Kyojin"_s;
    case anime::TitleLanguage::English:
      return u"Attack on Titan"_s;
    case anime::TitleLanguage::Native:
      return u"進撃の巨人"_s;
  }
}

}  // namespace

SettingsPageApplication::SettingsPageApplication(Ui::SettingsDialog* ui, QDialog* dialog)
    : SettingsPage(ui, dialog) {
  connect(ui_->titleLanguageComboBox, &QComboBox::currentIndexChanged, this, [this]() {
    const auto language =
        static_cast<anime::TitleLanguage>(ui_->titleLanguageComboBox->currentData().toInt());
    ui_->titleLanguageHintLabel->setText(tr("e.g. %1").arg(titleExamples(language)));
  });
}

void SettingsPageApplication::load() {
  {
    auto combo = ui_->colorSchemeComboBox;
    combo->addItem(tr("System"), static_cast<int>(Qt::ColorScheme::Unknown));
    combo->addItem(tr("Light"), static_cast<int>(Qt::ColorScheme::Light));
    combo->addItem(tr("Dark"), static_cast<int>(Qt::ColorScheme::Dark));
    combo->setCurrentIndex(
        std::max(0, combo->findData(static_cast<int>(taiga::settings()->appColorScheme()))));
  }
  {
    using anime::TitleLanguage;
    auto combo = ui_->titleLanguageComboBox;
    combo->addItem(tr("Romaji"), static_cast<int>(TitleLanguage::Romaji));
    combo->addItem(tr("English"), static_cast<int>(TitleLanguage::English));
    combo->addItem(tr("Native"), static_cast<int>(TitleLanguage::Native));
    combo->setCurrentIndex(
        std::max(0, combo->findData(static_cast<int>(taiga::settings()->appTitleLanguage()))));
  }
  ui_->hideNsfwCheckBox->setChecked(taiga::settings()->appHideNsfw());
  {
    ui_->closeToTrayCheckBox->setChecked(taiga::settings()->appCloseToTray());
    ui_->startInTrayCheckBox->setChecked(taiga::settings()->appStartInTray());
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
      ui_->behaviorGroupBox->setDisabled(true);
      ui_->behaviorGroupBox->setToolTip(tr("The system tray is not available."));
    }
  }
}

void SettingsPageApplication::apply() const {
  taiga::settings()->setAppColorScheme(
      static_cast<Qt::ColorScheme>(ui_->colorSchemeComboBox->currentData().toInt()));

  taiga::settings()->setAppTitleLanguage(
      static_cast<anime::TitleLanguage>(ui_->titleLanguageComboBox->currentData().toInt()));

  taiga::settings()->setAppHideNsfw(ui_->hideNsfwCheckBox->isChecked());

  taiga::settings()->setAppCloseToTray(ui_->closeToTrayCheckBox->isChecked());
  taiga::settings()->setAppStartInTray(ui_->startInTrayCheckBox->isChecked());
}

}  // namespace gui
