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

#include "painters.hpp"

#include <QGuiApplication>
#include <QPainter>
#include <QProxyStyle>

#include "base/string.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/theme.hpp"
#include "media/anime.hpp"
#include "media/anime_list.hpp"
#include "media/anime_list_utils.hpp"

namespace gui {

void paintEmptyListText(QAbstractScrollArea* area, const QString& text) {
  QPainter painter(area->viewport());

  painter.setFont([&painter]() {
    auto font = painter.font();
    font.setItalic(true);
    return font;
  }());

  painter.drawText(area->viewport()->rect(), Qt::AlignCenter, text);
}

void paintProgressBar(QPainter* painter, const QStyleOption& option, const Anime* anime,
                      const ListEntry* entry) {
  if (!anime || !entry) return;

  const int episodes = anime->episode_count;
  const int watched = std::clamp(entry->watched_episodes, 0,
                                 episodes > 0 ? episodes : std::numeric_limits<int>::max());
  const auto text = u"%1/%2"_s.arg(watched).arg(formatNumber(episodes, "?"));

  QStyleOptionProgressBar styleOption{};
  styleOption.state = option.state | QStyle::State_Horizontal;
  styleOption.direction = option.direction;
  styleOption.rect = option.rect;
  styleOption.palette = option.palette;
  styleOption.palette.setCurrentColorGroup(QPalette::ColorGroup::Active);
  styleOption.palette.setColor(QPalette::ColorRole::Highlight, theme.isDark()
                                                                   ? QColor{12, 164, 12, 128}
                                                                   : QColor{12, 164, 12, 255});
  styleOption.fontMetrics = option.fontMetrics;
  styleOption.maximum = 100;
  styleOption.minimum = 0;
  styleOption.progress = static_cast<int>(anime::list::getProgressRatio(anime, entry) * 100);
  styleOption.text = text;
  styleOption.textAlignment = Qt::AlignCenter;
  styleOption.textVisible = true;

  static const auto proxyStyle{new QProxyStyle{"fusion"}};
  proxyStyle->drawControl(QStyle::CE_ProgressBar, &styleOption, painter);
}

void paintSpinner(QPainter* painter, const QPixmap& pixmap, const QPointF& center, qreal angle) {
  const qreal dpr = pixmap.devicePixelRatio();
  const QPointF halfSize(pixmap.width() / dpr / 2.0, pixmap.height() / dpr / 2.0);

  painter->save();
  painter->setRenderHint(QPainter::SmoothPixmapTransform);
  painter->translate(center);
  painter->rotate(angle);
  painter->drawPixmap(-halfSize, pixmap);
  painter->restore();
}

}  // namespace gui
