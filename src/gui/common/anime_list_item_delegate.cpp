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

#include "anime_list_item_delegate.hpp"

#include <QComboBox>
#include <QPainter>
#include <QProxyStyle>
#include <limits>

#include "gui/models/anime_list_model.hpp"
#include "gui/utils/painter_state_saver.hpp"
#include "gui/utils/theme.hpp"

namespace gui {

ListItemDelegate::ListItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent), m_proxyStyle(new QProxyStyle("fusion")) {
  m_proxyStyle->setParent(parent);
}

void ListItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
  const auto entry =
      index.data(static_cast<int>(AnimeListItemDataRole::ListEntry)).value<const ListEntry*>();
  if (!entry) return;
  auto* combobox = static_cast<QComboBox*>(editor);
  combobox->setCurrentIndex(std::clamp(entry->score / 10, 0, 10));
}

QWidget* ListItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem&,
                                        const QModelIndex& index) const {
  if (index.column() != AnimeListModel::COLUMN_SCORE) return nullptr;

  auto* editor = new QComboBox(parent);
  editor->addItem("-");
  editor->setItemData(0, Qt::AlignCenter, Qt::TextAlignmentRole);
  for (int i = 1; i <= 10; ++i) {
    editor->addItem(tr("%1").arg(i));
    editor->setItemData(i, Qt::AlignCenter, Qt::TextAlignmentRole);
  }

  return editor;
}

void ListItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
                                    const QModelIndex& index) const {
  const auto* combobox = static_cast<QComboBox*>(editor);
  model->setData(index, combobox->currentText(), Qt::EditRole);
}

void ListItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                                            const QModelIndex&) const {
  editor->setGeometry(option.rect);
}

void ListItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                             const QModelIndex& index) const {
  // Grid lines
  if (index.column() > 0) {
    const PainterStateSaver painterStateSaver(painter);
    painter->setPen(theme.isDark() ? QColor{255, 255, 255, 8} : QColor{0, 0, 0, 8});
    painter->drawLine(option.rect.topLeft(), option.rect.bottomLeft());
  }

  switch (index.column()) {
    case AnimeListModel::COLUMN_PROGRESS: {
      const PainterStateSaver painterStateSaver(painter);

      QStyledItemDelegate::paint(painter, option, index);

      const auto anime =
          index.data(static_cast<int>(AnimeListItemDataRole::Anime)).value<const Anime*>();
      const auto entry =
          index.data(static_cast<int>(AnimeListItemDataRole::ListEntry)).value<const ListEntry*>();

      if (!anime || !entry) return;

      const int episodes = anime->episode_count;
      const int progress = std::clamp(entry->watched_episodes, 0,
                                      episodes > 0 ? episodes : std::numeric_limits<int>::max());
      const int percent = episodes > 0 ? static_cast<float>(progress) / episodes * 100.0f : 50;
      const auto text =
          tr("%1/%2").arg(progress).arg(episodes > 0 ? QString::number(episodes) : "?");

      QStyleOptionProgressBar styleOption{};
      styleOption.state = option.state | QStyle::State_Horizontal;
      styleOption.direction = option.direction;
      styleOption.rect = option.rect.adjusted(2, 2, -2, -2);
      styleOption.palette = option.palette;
      styleOption.palette.setCurrentColorGroup(QPalette::ColorGroup::Active);
      styleOption.palette.setColor(QPalette::ColorRole::Highlight, theme.isDark()
                                                                       ? QColor{12, 164, 12, 128}
                                                                       : QColor{12, 164, 12, 255});
      styleOption.fontMetrics = option.fontMetrics;
      styleOption.maximum = 100;
      styleOption.minimum = 0;
      styleOption.progress = percent;
      styleOption.text = text;
      styleOption.textAlignment = Qt::AlignCenter;
      styleOption.textVisible = true;

      m_proxyStyle->drawControl(QStyle::CE_ProgressBar, &styleOption, painter);
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
