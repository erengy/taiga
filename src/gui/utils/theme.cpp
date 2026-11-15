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

#include "theme.hpp"

#include <QApplication>
#include <QRegularExpression>
#include <QStyle>
#include <QStyleHints>

#include "base/file.hpp"
#include "base/log.hpp"
#include "base/string.hpp"
#include "gui/utils/svg_icon_engine.hpp"
#include "taiga/settings.hpp"

namespace gui {

namespace {

QColor black(const float alpha) {
  return QColor::fromRgbF(0, 0, 0, alpha);
}

QColor white(const float alpha) {
  return QColor::fromRgbF(1, 1, 1, alpha);
}

const ThemeColors kDarkColors{
    .background = QColor(0x1c, 0x1c, 0x1c),
    .border = white(0.16f),
    .splitter = black(0.30f),
    .itemAlternate = white(0.03f),
    .itemHover = white(0.05f),
    .itemSelected = white(0.20f),
    .itemSelectedInactive = white(0.10f),
    .gridLine = QColor(255, 255, 255, 8),
    .secondaryText = QColor(0xaa, 0xaa, 0xaa),
    .progress = QColor(12, 164, 12, 128),
    .success = QColor(0x66, 0xbb, 0x6a),  // Green 400
    .warning = QColor(0xff, 0xa7, 0x26),  // Orange 400
};

const ThemeColors kLightColors{
    .background = QColor(0xff, 0xff, 0xff),
    .border = black(0.16f),
    .splitter = white(0.30f),
    .itemAlternate = black(0.04f),
    .itemHover = black(0.04f),
    .itemSelected = black(0.08f),
    .itemSelectedInactive = black(0.06f),
    .gridLine = QColor(0, 0, 0, 8),
    .secondaryText = QColor(0x66, 0x66, 0x66),
    .progress = QColor(12, 164, 12),
    .success = QColor(0x43, 0xa0, 0x47),  // Green 600
    .warning = QColor(0xfb, 0x8c, 0x00),  // Orange 600
};

QString toStylesheetColor(const QColor& color) {
  return u"rgba(%1, %2, %3, %4)"_s.arg(color.red())
      .arg(color.green())
      .arg(color.blue())
      .arg(color.alpha());
}

// Replaces `@name` placeholders with the matching theme colors.
QString replaceColorPlaceholders(const QString& stylesheet, const ThemeColors& colors) {
  const QHash<QString, QColor> values{
      {u"background"_s, colors.background},
      {u"border"_s, colors.border},
      {u"splitter"_s, colors.splitter},
      {u"itemAlternate"_s, colors.itemAlternate},
      {u"itemHover"_s, colors.itemHover},
      {u"itemSelected"_s, colors.itemSelected},
      {u"itemSelectedInactive"_s, colors.itemSelectedInactive},
  };

  static const QRegularExpression placeholder{uR"(@(\w+))"_s};

  QString result;
  qsizetype last = 0;

  auto it = placeholder.globalMatch(stylesheet);
  while (it.hasNext()) {
    const auto match = it.next();
    result += stylesheet.sliced(last, match.capturedStart() - last);
    if (const auto value = values.find(match.captured(1)); value != values.end()) {
      result += toStylesheetColor(*value);
    } else {
      qWarning() << "Unknown stylesheet color:" << match.captured(0);
      result += match.captured(0);
    }
    last = match.capturedEnd();
  }
  result += stylesheet.sliced(last);

  return result;
}

}  // namespace

Theme::Theme(QObject* parent) : QObject{parent} {}

const QIcon& Theme::getIcon(const QString& key, const QString& extension, bool useSvgIconEngine) {
  if (!icons_.contains(key)) {
    if (extension == "svg" && useSvgIconEngine) {
      icons_[key] = QIcon(new SvgIconEngine(key));
    } else {
      icons_[key] = QIcon(u":/icons/%1.%2"_s.arg(key, extension));
    }
  }

  return icons_[key];
}

void Theme::initStyle() {
  qApp->styleHints()->setColorScheme(taiga::settings()->appColorScheme());
  connect(taiga::settings(), &taiga::Settings::changed, this, [](const QSet<QString>& keys) {
    if (keys.contains(taiga::Settings::Key::kAppColorScheme)) {
      qApp->styleHints()->setColorScheme(taiga::settings()->appColorScheme());
    }
  });

  const auto style = QString::fromStdString(taiga::settings()->appStyle());
  if (style.compare(taiga::Settings::kAppStyleSystem, Qt::CaseInsensitive) != 0) {
    qApp->setStyle(style);
  }

  // Must be checked before applying the stylesheet: Qt then wraps the app style in a
  // QStyleSheetStyle, whose name() is empty.
  isFusion_ = qApp->style()->name().compare("fusion", Qt::CaseInsensitive) == 0;

#ifdef Q_OS_WINDOWS
  if (style.compare("fusion", Qt::CaseInsensitive) == 0) {
    applyStylesheet();
    connect(
        qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this]() { applyStylesheet(); },
        Qt::QueuedConnection);
  }
#endif
}

bool Theme::isFusion() const {
  return isFusion_;
}

bool Theme::isDark() const {
  return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

void Theme::applyStylesheet() const {
  const QString mainStylesheet = readStylesheet("main");
  const QString themeStylesheet = replaceColorPlaceholders(readStylesheet("theme"), colors());
  qApp->setStyleSheet(mainStylesheet + themeStylesheet);
}

QString Theme::readStylesheet(const QString& name) const {
  return base::readFile(u":/styles/%1.qss"_s.arg(name));
}

QColor Theme::errorColor() {
  return QColor(0xe5, 0x39, 0x35);  // Red 600
}

QColor Theme::successColor() {
  return QColor(0x43, 0xa0, 0x47);  // Green 600
}

const ThemeColors& Theme::colors() const {
  return isDark() ? kDarkColors : kLightColors;
}

Theme* theme() {
  static auto instance = new Theme{qApp};
  return instance;
}

}  // namespace gui
