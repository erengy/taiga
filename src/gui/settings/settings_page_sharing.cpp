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
#include "settings_page_sharing.hpp"

#include <QCheckBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <ranges>

#include "base/string.hpp"
#include "taiga/settings.hpp"
#include "ui_settings_dialog.h"

namespace gui {

SettingsPageSharing::SettingsPageSharing(Ui::SettingsDialog* ui, QDialog* dialog)
    : SettingsPage(ui, dialog) {
  connect(ui_->sharingEnabledCheckBox, &QCheckBox::toggled, ui_->discordGroupBox,
          &QWidget::setEnabled);
  connect(ui_->sharingEnabledCheckBox, &QCheckBox::toggled, ui_->httpGroupBox,
          &QWidget::setEnabled);

  const std::initializer_list<QWidget*> widgets{
      ui_->httpUrlLabel,
      ui_->httpUrlLineEdit,
      ui_->httpHeadersLabel,
      ui_->httpHeadersPlainTextEdit,
  };
  for (auto* widget : widgets) {
    connect(ui_->httpEnabledCheckBox, &QCheckBox::toggled, widget, &QWidget::setEnabled);
  }
}

void SettingsPageSharing::load() {
  ui_->sharingEnabledCheckBox->setChecked(taiga::settings()->sharingEnabled());
  ui_->discordGroupBox->setEnabled(ui_->sharingEnabledCheckBox->isChecked());
  ui_->httpGroupBox->setEnabled(ui_->sharingEnabledCheckBox->isChecked());

  ui_->discordEnabledCheckBox->setChecked(taiga::settings()->discordEnabled());

  ui_->httpEnabledCheckBox->setChecked(taiga::settings()->webhookEnabled());
  emit ui_->httpEnabledCheckBox->toggled(ui_->httpEnabledCheckBox->isChecked());
  ui_->httpUrlLineEdit->setText(QString::fromStdString(taiga::settings()->webhookUrl()));
  ui_->httpHeadersPlainTextEdit->setPlainText(
      toStringList(taiga::settings()->webhookHeaders()).join(u'\n'));
}

void SettingsPageSharing::apply() const {
  taiga::settings()->setSharingEnabled(ui_->sharingEnabledCheckBox->isChecked());
  taiga::settings()->setDiscordEnabled(ui_->discordEnabledCheckBox->isChecked());

  taiga::settings()->setWebhookEnabled(ui_->httpEnabledCheckBox->isChecked());
  taiga::settings()->setWebhookUrl(ui_->httpUrlLineEdit->text().trimmed().toStdString());
  const auto lines = ui_->httpHeadersPlainTextEdit->toPlainText().split(u'\n');
  taiga::settings()->setWebhookHeaders(
      toVector(lines | std::views::transform([](const QString& s) { return s.trimmed(); }) |
               std::views::filter([](const QString& s) { return !s.isEmpty(); }) |
               std::ranges::to<QStringList>()));
}

}  // namespace gui
