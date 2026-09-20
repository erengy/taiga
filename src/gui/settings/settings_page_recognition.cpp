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
#include "settings_page_recognition.hpp"

#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <chrono>

#include "taiga/settings.hpp"
#include "track/media.hpp"
#include "track/update_trigger.hpp"
#include "ui_settings_dialog.h"

namespace gui {

SettingsPageRecognition::SettingsPageRecognition(Ui::SettingsDialog* ui, QDialog* dialog)
    : SettingsPage(ui, dialog) {
  using taiga::Settings;

  ui_->detectionIntervalSpinBox->setRange(1, 60);
  ui_->updateDelaySpinBox->setRange(static_cast<int>(Settings::kUpdateDelayMin.count()),
                                    static_cast<int>(Settings::kUpdateDelayMax.count()));
}

void SettingsPageRecognition::load() {
  using std::chrono::duration_cast;
  using std::chrono::seconds;

  ui_->detectionEnabledCheckBox->setChecked(taiga::settings.detectionEnabled());
  ui_->detectionIntervalSpinBox->setValue(
      static_cast<int>(duration_cast<seconds>(taiga::settings.mediaDetectionInterval()).count()));

  ui_->updateDelaySpinBox->setValue(static_cast<int>(taiga::settings.updateDelay().count()));
  const auto onPlayerClose = taiga::settings.updateTrigger() == track::UpdateTrigger::OnPlayerClose;
  (onPlayerClose ? ui_->updateOnPlayerCloseRadioButton : ui_->updateAfterDelayRadioButton)
      ->setChecked(true);
  ui_->updatePauseWhenUnfocusedCheckBox->setChecked(taiga::settings.updatePauseWhenUnfocused());
  ui_->updateLibraryOnlyCheckBox->setChecked(taiga::settings.updateLibraryOnly());
}

void SettingsPageRecognition::apply() const {
  using std::chrono::seconds;

  taiga::settings.setMediaDetectionInterval(seconds{ui_->detectionIntervalSpinBox->value()});
  taiga::settings.setUpdateDelay(seconds{ui_->updateDelaySpinBox->value()});
  taiga::settings.setUpdateTrigger(ui_->updateOnPlayerCloseRadioButton->isChecked()
                                       ? track::UpdateTrigger::OnPlayerClose
                                       : track::UpdateTrigger::AfterDelay);
  taiga::settings.setUpdatePauseWhenUnfocused(ui_->updatePauseWhenUnfocusedCheckBox->isChecked());
  taiga::settings.setUpdateLibraryOnly(ui_->updateLibraryOnlyCheckBox->isChecked());

  // Must come after the interval is saved, since it restarts polling with the new interval.
  track::media::detection()->setEnabled(ui_->detectionEnabledCheckBox->isChecked());
}

}  // namespace gui
