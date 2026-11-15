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

#pragma once

#include <QMessageBox>
#include <QString>
#include <QWidget>

namespace taiga {
class Orange;
}

namespace gui {

class AboutDialogHandler : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(AboutDialogHandler)

public:
  AboutDialogHandler(QObject* parent);

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
  void resetWindowTitle();

private:
  QMessageBox* messageBox() const;

  taiga::Orange* orange_;
  QString previousWindowTitle_;
};

void displayAboutDialog(QWidget* parent);

}  // namespace gui
