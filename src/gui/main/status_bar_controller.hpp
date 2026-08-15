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

#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>

class QStatusBar;

namespace gui {

class SpinnerWidget;

class StatusBarController final : public QObject {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(StatusBarController)

public:
  // Declaration order sets display priority.
  enum class Source {
    Sync,
    Playback,
    Export,
    Selection,
  };

  struct Message {
    Source source;
    QString text;
    bool spin = true;
  };

  StatusBarController(QObject* parent, QStatusBar* statusBar, SpinnerWidget* spinner);
  ~StatusBarController() = default;

  void showMessage(const Message& message);
  void clearMessage(Source source);
  void clearAll();

private:
  void refresh();

  QStatusBar* statusBar_ = nullptr;
  SpinnerWidget* spinner_ = nullptr;
  QMap<Source, QString> messages_;
  QSet<Source> spinningSources_;
};

}  // namespace gui
