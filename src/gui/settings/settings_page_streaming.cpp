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
#include "settings_page_streaming.hpp"

#include <QCheckBox>
#include <QGroupBox>
#include <anisthesia.hpp>

#include "gui/settings/settings_player_list.hpp"
#include "taiga/settings.hpp"
#include "ui_settings_dialog.h"

namespace gui {

SettingsPageStreaming::SettingsPageStreaming(Ui::SettingsDialog* ui, QDialog* dialog)
    : SettingsPage(ui, dialog) {
  connect(ui_->streamingEnabledCheckBox, &QCheckBox::toggled, ui_->streamingBrowsersGroupBox,
          &QWidget::setEnabled);
}

void SettingsPageStreaming::load() {
  ui_->streamingEnabledCheckBox->setChecked(taiga::settings.streamingMediaEnabled());
  loadPlayerList(ui_->streamingBrowsersList, anisthesia::PlayerType::WebBrowser);
}

void SettingsPageStreaming::apply() const {
  taiga::settings.setStreamingMediaEnabled(ui_->streamingEnabledCheckBox->isChecked());
  savePlayerList(ui_->streamingBrowsersList, anisthesia::PlayerType::WebBrowser);
}

}  // namespace gui
