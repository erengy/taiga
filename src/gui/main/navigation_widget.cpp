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

#include "navigation_widget.hpp"

#include <QMouseEvent>

#include "gui/main/main_window.hpp"
#include "gui/main/navigation_item_delegate.hpp"
#include "gui/utils/theme.hpp"

namespace gui {

NavigationWidget::NavigationWidget(MainWindow* mainWindow)
    : QTreeWidget(mainWindow), m_mainWindow(mainWindow) {
  setFixedWidth(200);
  setFrameShape(QFrame::Shape::NoFrame);
  setHeaderHidden(true);
  setIndentation(0);
  setItemDelegate(new NavigationItemDelegate(this));
  setObjectName("navigation");

  setPalette([this]() {
    QPalette palette = this->palette();
    palette.setColor(QPalette::Base, theme.isDark() ? QColor{0, 0, 0, 255 / 100 * 50}
                                                    : QColor{255, 255, 255, 255 / 100 * 50});
    return palette;
  }());

  viewport()->setMouseTracking(true);

  refresh();

  connect(this, &QTreeWidget::currentItemChanged,
          [this](QTreeWidgetItem* current, QTreeWidgetItem* previous) {
            if (current) {
              const int role = static_cast<int>(NavigationItemDataRole::PageIndex);
              const int index = current->data(0, role).toInt();
              const auto page = static_cast<MainWindowPage>(index);
              m_mainWindow->setPage(page);
            }
          });
}

void NavigationWidget::refresh() {
  setUpdatesEnabled(false);
  clear();

  addItem("Home", "home", MainWindowPage::Home);
  addItem("Search", "search", MainWindowPage::Search);
  addSeparator();

  auto listItem = addItem("Anime List", "list_alt", MainWindowPage::List);
  listItem->setExpanded(true);
  setItemData(listItem, NavigationItemDataRole::HasChildren, true);

  const QList<QPair<QString, int>> statusList{
      {"Watching", 29}, {"Completed", 710}, {"Paused", 44}, {"Dropped", 34}, {"Planning", 144},
  };
  for (const auto [text, counter] : statusList) {
    auto item = addChildItem(listItem, text);
    setItemData(item, NavigationItemDataRole::PageIndex, static_cast<int>(MainWindowPage::List));
    setItemData(item, NavigationItemDataRole::IsLastChild, text == "Planning");
    setItemData(item, NavigationItemDataRole::Counter, counter);
  }

  addItem("History", "history", MainWindowPage::History);
  addSeparator();
  addItem("Library", "folder", MainWindowPage::Library);
  addItem("Torrents", "rss_feed", MainWindowPage::Torrents);

  setUpdatesEnabled(true);
}

void NavigationWidget::mouseMoveEvent(QMouseEvent* event) {
  auto cursor = Qt::CursorShape::ArrowCursor;

  if (const auto item = itemAt(event->pos())) {
    const int role = static_cast<int>(NavigationItemDataRole::IsSeparator);
    const bool isSeparator = item->data(0, role).toBool();
    if (!isSeparator) cursor = Qt::CursorShape::PointingHandCursor;
  }

  setCursor(cursor);

  QTreeView::mouseMoveEvent(event);
}

QTreeWidgetItem* NavigationWidget::addItem(const QString& text, const QString& icon,
                                           MainWindowPage page) {
  auto item = new QTreeWidgetItem(this);

  item->setFont(0, [item]() {
    auto font = item->font(0);
    font.setPointSize(10);
    font.setWeight(QFont::Weight::DemiBold);
    return font;
  }());

  item->setIcon(0, theme.getIcon(icon));
  item->setSizeHint(0, QSize{0, 32});
  item->setText(0, text);

  setItemData(item, NavigationItemDataRole::PageIndex, static_cast<int>(page));

  return item;
}

QTreeWidgetItem* NavigationWidget::addChildItem(QTreeWidgetItem* parent, const QString& text) {
  auto item = new QTreeWidgetItem(parent);

  item->setIcon(0, theme.getIcon("empty"));
  item->setSizeHint(0, QSize{0, 32});
  item->setText(0, text);

  setItemData(item, NavigationItemDataRole::IsChild, true);

  return item;
}

void NavigationWidget::addSeparator() {
  auto item = new QTreeWidgetItem(this);

  item->setDisabled(true);
  item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
  item->setSizeHint(0, QSize{0, 16});

  setItemData(item, NavigationItemDataRole::IsSeparator, true);
}

void NavigationWidget::setItemData(QTreeWidgetItem* item, NavigationItemDataRole role,
                                   const QVariant& value) {
  item->setData(0, static_cast<int>(role), value);
}

}  // namespace gui
