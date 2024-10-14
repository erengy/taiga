/**
 * Taiga
 * Copyright (C) 2010-2024, Eren Okka
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

#include "page_widget.hpp"

namespace gui {

PageWidget::PageWidget(QWidget* parent) : QWidget(parent) {
  // Main layout
  auto layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  setLayout(layout);

  // Toolbar layout
  m_toolbarLayout = new QHBoxLayout(this);
  m_toolbarLayout->setContentsMargins(16, 8, 16, 8);
  m_toolbarLayout->addStretch();
  layout->addLayout(m_toolbarLayout);

  // Toolbar
  m_toolbar = new QToolBar(this);
  m_toolbar->setIconSize({18, 18});
  m_toolbarLayout->addWidget(m_toolbar);
}

}  // namespace gui
