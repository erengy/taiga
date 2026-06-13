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

#include "theme.hpp"

#include <QApplication>
#include <QPalette>
#include <QStyleHints>
#include <QTimer>
#include <QSettings>

#include "base/file.hpp"
#include "base/string.hpp"
#include "gui/utils/svg_icon_engine.hpp"
#include "taiga/settings.hpp"

namespace gui {

Theme::Theme() : QObject() {}

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
  // Only override the system scheme if the user has explicitly chosen one
  const auto savedScheme = taiga::settings.appColorScheme();
  if (savedScheme != Qt::ColorScheme::Unknown) {
    qApp->styleHints()->setColorScheme(savedScheme);
  }

  // Re-apply style whenever the color scheme changes (system or user)
  connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [this](Qt::ColorScheme) {
    m_icons.clear();
    applyStyle();
  });

  // Poll every second as a fallback for system theme changes
  m_lastScheme = qApp->styleHints()->colorScheme();
  m_themeTimer = new QTimer(this);
  connect(m_themeTimer, &QTimer::timeout, this, [this]() {
    // Read Windows theme directly from registry
    const QSettings registry(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        QSettings::NativeFormat);
    const bool isLightTheme = registry.value("AppsUseLightTheme", 1).toInt() == 1;
    const auto currentScheme = isLightTheme ? Qt::ColorScheme::Light : Qt::ColorScheme::Dark;

    if (currentScheme != m_lastScheme) {
      m_lastScheme = currentScheme;
      qApp->styleHints()->setColorScheme(currentScheme);
      m_icons.clear();
      applyStyle();
    }
  });
  m_themeTimer->start(1000);

#ifdef Q_OS_WINDOWS
  applyStyle();
#endif
}

void Theme::applyStyle() {
  qApp->setStyle("fusion");

  if (isDark()) {
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(32, 32, 32));
    darkPalette.setColor(QPalette::WindowText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Base, QColor(28, 28, 28));
    darkPalette.setColor(QPalette::AlternateBase, QColor(40, 40, 40));
    darkPalette.setColor(QPalette::Text, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Button, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::ToolTipText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::PlaceholderText, QColor(180, 180, 180));
    qApp->setPalette(darkPalette);
  } else {
    QPalette lightPalette;
    lightPalette.setColor(QPalette::Window, QColor(249, 249, 249));
    lightPalette.setColor(QPalette::WindowText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
    lightPalette.setColor(QPalette::Text, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
    lightPalette.setColor(QPalette::ButtonText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
    lightPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
    lightPalette.setColor(QPalette::ToolTipText, QColor(0, 0, 0));
    lightPalette.setColor(QPalette::PlaceholderText, QColor(120, 120, 120));
    qApp->setPalette(lightPalette);
  }

  const QString mainStylesheet = readStylesheet("main");
  const QString themeStylesheet = readStylesheet(isDark() ? "dark" : "light");
  qApp->setStyleSheet(mainStylesheet + themeStylesheet);
}

bool Theme::isDark() const {
  return qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

QString Theme::readStylesheet(const QString& name) const {
  return base::readFile(u":/styles/%1.qss"_s.arg(name));
}

}  // namespace gui
