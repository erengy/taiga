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
#include <QStyle>
#include <QStyleHints>

#include "base/file.hpp"
#include "base/string.hpp"
#include "gui/utils/svg_icon_engine.hpp"
#include "taiga/settings.hpp"

namespace gui {

Theme::Theme(QObject* parent) : QObject{parent} {}

const QIcon& Theme::getIcon(const QString& key, const QString& extension, bool useSvgIconEngine) {
  if (!m_icons.contains(key)) {
    if (extension == "svg" && useSvgIconEngine) {
      m_icons[key] = QIcon(new SvgIconEngine(key));
    } else {
      m_icons[key] = QIcon(u":/icons/%1.%2"_s.arg(key, extension));
    }
  }

  return m_icons[key];
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
  m_isFusion = qApp->style()->name().compare("fusion", Qt::CaseInsensitive) == 0;

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
  return m_isFusion;
}

bool Theme::isDark() const {
  return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

void Theme::applyStylesheet() const {
  const QString mainStylesheet = readStylesheet("main");
  const QString themeStylesheet = readStylesheet(isDark() ? "dark" : "light");
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

QColor Theme::warningColor() {
  return QColor(0xfb, 0x8c, 0x00);  // Orange 600
}

QColor Theme::itemAlternateColor() const {
  return isDark() ? QColor::fromRgbF(1, 1, 1, 0.03f) : QColor::fromRgbF(0, 0, 0, 0.04f);
}

QColor Theme::itemHoverColor() const {
  return isDark() ? QColor::fromRgbF(1, 1, 1, 0.05f) : QColor::fromRgbF(0, 0, 0, 0.04f);
}

QColor Theme::itemSelectionColor(bool active) const {
  if (isDark()) return QColor::fromRgbF(1, 1, 1, active ? 0.20f : 0.10f);
  return QColor::fromRgbF(0, 0, 0, active ? 0.08f : 0.06f);
}

Theme* theme() {
  static auto instance = new Theme{qApp};
  return instance;
}

}  // namespace gui
