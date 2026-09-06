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

#include <QTreeView>

#include "gui/common/combobox.hpp"
#include "gui/common/page_widget.hpp"

namespace gui {

class LibraryModel;

class LibraryWidget final : public PageWidget {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(LibraryWidget)

public:
  LibraryWidget(QWidget* parent);
  ~LibraryWidget() = default;

private:
  bool eventFilter(QObject* watched, QEvent* event) override;
  void showContextMenu() const;

  LibraryModel* m_model = nullptr;
  ComboBox* m_comboRoot = nullptr;
  QTreeView* m_view = nullptr;
};

}  // namespace gui
