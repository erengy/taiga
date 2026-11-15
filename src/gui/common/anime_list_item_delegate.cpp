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

#include "anime_list_item_delegate.hpp"

#include <QComboBox>
#include <QHeaderView>
#include <QPainter>
#include <QTreeView>

#include "gui/models/anime_list_model.hpp"
#include "gui/utils/painter_state_saver.hpp"
#include "gui/utils/painters.hpp"
#include "gui/utils/rating.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime.hpp"
#include "media/anime_list.hpp"

namespace gui {

namespace {

bool isLastVisibleColumn(const QHeaderView* header, const int logicalIndex) {
  for (int visualIndex = header->count() - 1; visualIndex >= 0; --visualIndex) {
    const int logical = header->logicalIndex(visualIndex);
    if (!header->isSectionHidden(logical)) return logical == logicalIndex;
  }
  return true;
}

}  // namespace

ListItemDelegate::ListItemDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

void ListItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
  const auto entry =
      index.data(static_cast<int>(AnimeListItemDataRole::ListEntry)).value<const ListEntry*>();
  if (!entry) return;
  auto* combobox = static_cast<QComboBox*>(editor);
  setRatingComboBoxValue(combobox, entry->score);
}

QWidget* ListItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&,
                                        const QModelIndex& index) const {
  if (index.column() != AnimeListModel::ColumnScore) return nullptr;

  auto* editor = new QComboBox(parent);
  populateRatingComboBox(editor);
  for (int i = 0; i < editor->count(); ++i) {
    editor->setItemData(i, Qt::AlignCenter, Qt::TextAlignmentRole);
  }

  return editor;
}

void ListItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                    const QModelIndex& index) const {
  const auto* combobox = static_cast<QComboBox*>(editor);
  model->setData(index, combobox->currentData(), Qt::EditRole);
}

void ListItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                                            const QModelIndex&) const {
  editor->setGeometry(option.rect);
}

void ListItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                             const QModelIndex& index) const {
  // Grid lines
  const auto* view = qobject_cast<const QTreeView*>(option.widget);
  if (view && !isLastVisibleColumn(view->header(), index.column())) {
    const PainterStateSaver painterStateSaver(painter);
    painter->setPen(theme()->colors().gridLine);
    const int x = option.rect.right() - (theme()->isFusion() ? 1 : 0);
    painter->drawLine(x, option.rect.top(), x, option.rect.bottom());
  }

  switch (index.column()) {
    case AnimeListModel::ColumnProgress: {
      const PainterStateSaver painterStateSaver(painter);
      QStyledItemDelegate::paint(painter, option, index);
      const auto anime =
          index.data(static_cast<int>(AnimeListItemDataRole::Anime)).value<const Anime*>();
      const auto entry =
          index.data(static_cast<int>(AnimeListItemDataRole::ListEntry)).value<const ListEntry*>();
      QStyleOptionViewItem opt = option;
      opt.rect.adjust(2, 2, theme()->isFusion() ? -6 : -2, -2);
      paintProgressBar(painter, opt, anime, entry);
      return;
    }
  }

  QStyledItemDelegate::paint(painter, option, index);
}

QSize ListItemDelegate::sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const {
  if (index.isValid()) {
    return QSize(0, 24);
  }

  return QStyledItemDelegate::sizeHint(option, index);
}

}  // namespace gui
