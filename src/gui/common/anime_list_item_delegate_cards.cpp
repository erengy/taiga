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

#include "anime_list_item_delegate_cards.hpp"

#include <QListView>
#include <QPainter>
#include <QPainterPath>

#include "gui/models/anime_list_model.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/image_provider.hpp"
#include "gui/utils/painter_state_saver.hpp"
#include "gui/utils/theme.hpp"

namespace gui {

ListItemDelegateCards::ListItemDelegateCards(QObject* parent) : QStyledItemDelegate(parent) {}

void ListItemDelegateCards::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const {
  const PainterStateSaver painterStateSaver(painter);

  const auto item =
      index.data(static_cast<int>(AnimeListItemDataRole::Anime)).value<const Anime*>();

  QStyleOptionViewItem opt = option;
  QRect rect = opt.rect;

  QPainterPath path;
  path.addRoundedRect(rect, 4, 4);
  painter->setClipPath(path);

  // Background
  if (option.state & QStyle::State_Selected) {
    painter->fillRect(rect, opt.palette.highlight());
  } else if (theme.isDark()) {
    painter->fillRect(rect, opt.palette.mid());
  } else {
    painter->fillRect(rect, opt.palette.alternateBase());
  }

  // Poster
  {
    QRect posterRect = rect;
    posterRect.setWidth(140);

    if (theme.isDark()) {
      painter->fillRect(posterRect, opt.palette.dark());
    } else {
      painter->fillRect(posterRect, opt.palette.mid());
    }
    if (const auto& posterPixmap = imageProvider.loadPoster(item->id); !posterPixmap.isNull()) {
      painter->drawPixmap(posterRect, posterPixmap);
    }

    rect.adjust(140, 0, 0, 0);
  }

  auto font = painter->font();

  // Title
  {
    QRect titleRect = rect;
    titleRect.setHeight(24);

    painter->fillRect(titleRect, opt.palette.dark());
    titleRect.adjust(8, 0, -8, 0);

    auto titleFont = painter->font();
    titleFont.setWeight(QFont::Weight::DemiBold);
    painter->setFont(titleFont);

    const QString title = index.data(Qt::DisplayRole).toString();
    const QFontMetrics metrics(painter->font());
    const QString elidedTitle = metrics.elidedText(title, Qt::ElideRight, titleRect.width());

    painter->drawText(titleRect, Qt::AlignVCenter | Qt::TextSingleLine, elidedTitle);

    rect.adjust(8, 24 + 8, -8, -8);
  }

  // Summary
  {
    const QString summary =
        u"%1 · %2 episodes · %3"_qs.arg(formatType(item->type))
            .arg(item->episode_count > 0 ? u"%1"_qs.arg(item->episode_count) : u"?"_qs)
            .arg(formatScore(item->score));
    const QFontMetrics metrics(painter->font());
    QRect summaryRect = rect;
    summaryRect.setHeight(metrics.height());
    painter->setFont(font);
    painter->drawText(summaryRect, Qt::AlignVCenter | Qt::TextSingleLine, summary);
    rect.adjust(0, summaryRect.height() + 8, 0, 0);
  }

  auto detailsFont = painter->font();

  // Details
  {
    static const auto from_vector = [](const std::vector<std::string>& vector) {
      if (vector.empty()) return u"?"_qs;
      QStringList list;
      for (const auto& str : vector) {
        list.append(QString::fromStdString(str));
      }
      return list.join(", ");
    };

    const QString titles =
        "Aired:\n"
        "Genres:\n"
        "Studios:";
    const QString values = u"%1 to %2 (%3)\n%4\n%5"_qs.arg(formatFuzzyDate(item->start_date))
                               .arg(formatFuzzyDate(item->end_date))
                               .arg(formatStatus(item->status))
                               .arg(from_vector(item->genres))
                               .arg(from_vector(item->studios));

    detailsFont.setWeight(QFont::Weight::DemiBold);
    painter->setFont(detailsFont);

    const QFontMetrics metrics(painter->font());

    QRect titlesRect = rect;
    titlesRect.setHeight(metrics.height() * 3);
    titlesRect.setWidth(metrics.boundingRect("Studios:").width());

    painter->drawText(titlesRect, 0, titles);

    detailsFont.setWeight(QFont::Weight::Normal);
    painter->setFont(detailsFont);

    QRect valuesRect = rect;
    valuesRect.setHeight(metrics.height() * 3);
    valuesRect.adjust(titlesRect.width() + 8, 0, 0, 0);

    painter->drawText(valuesRect, 0, values);

    rect.adjust(0, titlesRect.height() + 8, 0, 0);
  }

  // Synopsis
  {
    const QString synopsis = QString::fromStdString(item->synopsis);

    painter->setPen(opt.palette.placeholderText().color());

    detailsFont.setPointSize(8);
    painter->setFont(detailsFont);
    const QFontMetrics metrics(painter->font());

    QRect synopsisRect = rect;
    synopsisRect.setHeight(qMin(synopsisRect.height(), metrics.height() * 5));

    painter->drawText(synopsisRect, Qt::TextWordWrap, synopsis);
  }
}

QSize ListItemDelegateCards::sizeHint(const QStyleOptionViewItem& option,
                                      const QModelIndex& index) const {
  if (index.isValid()) return itemSize();
  return QStyledItemDelegate::sizeHint(option, index);
}

void ListItemDelegateCards::initStyleOption(QStyleOptionViewItem* option,
                                            const QModelIndex& index) const {
  QStyledItemDelegate::initStyleOption(option, index);

  option->features &= ~QStyleOptionViewItem::ViewItemFeature::HasDisplay;
  option->features &= ~QStyleOptionViewItem::ViewItemFeature::HasDecoration;
}

QSize ListItemDelegateCards::itemSize() const {
  const auto parent = reinterpret_cast<QListView*>(this->parent());
  const auto rect = parent->geometry();

  constexpr int maxWidth = 360;
  int columns = 1;
  if (rect.width() > maxWidth * 2) columns = 2;
  if (rect.width() > maxWidth * 3) columns = 3;
  if (rect.width() > maxWidth * 4) columns = 4;

  constexpr int spacing = 18;
  const int width = (rect.width() - (spacing * (columns + 2))) / columns;
  constexpr int height = 210;

  return QSize(width, height);
}

}  // namespace gui
