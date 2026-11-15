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

#include "about_dialog.hpp"

#include <utf8proc.h>

#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>

#include "base/string.hpp"
#include "taiga/orange.hpp"
#include "taiga/version.hpp"

namespace gui {

namespace {

class AboutDialog {
  Q_DECLARE_TR_FUNCTIONS(gui::AboutDialog)
};

}  // namespace

QString getAboutDialogText() {
  const auto version = QString::fromStdString(taiga::version().to_string());

  const QStringList links{
      u"<a href='https://taiga.moe/'>Website</a>"_s,
      u"<a href='https://github.com/erengy/taiga'>GitHub</a>"_s,
      u"<a href='https://discord.gg/yeGNktZ'>Discord</a>"_s,
  };

  // clang-format off
  const QStringList contributors{
      u"saka"_s,
      u"Diablofan"_s,
      u"slevir"_s,
      u"LordGravewish"_s,
      u"rr-"_s,
      u"sunjayc"_s,
      u"ConnorKrammer"_s,
      u"Soinou"_s,
      u"Jiyuu"_s,
      u"ryban"_s,
      u"tollyx"_s,
      u"pavelxdd"_s,
      u"gunt3001"_s,
      u"synthtech"_s,
      u"cnguy"_s,
      u"CeruleanSky"_s,
      u"Xabis"_s,
      u"rzumer"_s,
      u"Juplay"_s,
      u"SacredZenpie"_s,
  };
  // clang-format on

  // clang-format off
  const QStringList donators{
      u"Farfie"_s,
      u"snickler"_s,
      u"Nydaleclya"_s,
      u"WizardTim"_s,
      u"Kinzer"_s,
      u"MeGaNeKo"_s,
      u"WhatsCPS"_s,
      u"Jerico64"_s,
      u"Xen the Greedy"_s,
      AboutDialog::tr("and other anonymous supporters"),
  };
  // clang-format on

  const QStringList components{
      u"Material Symbols"_s,
      u"Qt %1"_s.arg(QT_VERSION_STR),
      u"utf8proc %1"_s.arg(utf8proc_version()),
  };

  QStringList sections;

  const auto addSection = [&sections](const QString& title, const QString& text) {
    sections.append(u"<b>%1:</b><br>%2"_s.arg(title).arg(text));
  };

  sections.append(u"<big><b>Taiga</b> %1</big>"_s.arg(version));
  sections.append(links.join(" · "));
  addSection(AboutDialog::tr("Author"), "erengy (Eren Okka)");
  addSection(AboutDialog::tr("Contributors"), contributors.join(", "));
  addSection(AboutDialog::tr("Donators"), donators.join(", "));
  addSection(AboutDialog::tr("Third-party components"), components.join(", "));

  return sections.join("<br><br>");
}

void displayAboutDialog(QWidget* parent) {
  const auto msgBox = new QMessageBox(parent);

  // See `QMessageBox::about()`
  msgBox->setAttribute(Qt::WA_DeleteOnClose);
  msgBox->setIconPixmap(parent->windowIcon().pixmap(QSize(64, 64), msgBox->devicePixelRatio()));
  msgBox->setText(getAboutDialogText());
  msgBox->setWindowTitle(AboutDialog::tr("About Taiga"));

  if (const auto iconLabel = msgBox->findChild<QLabel*>()) {
    const auto handler = new AboutDialogHandler(msgBox);
    iconLabel->installEventFilter(handler);
  }

  msgBox->exec();
}

AboutDialogHandler::AboutDialogHandler(QObject* parent)
    : QObject(parent), orange_(new taiga::Orange(this)) {
  connect(orange_, &QThread::finished, this, &AboutDialogHandler::resetWindowTitle);
}

void AboutDialogHandler::resetWindowTitle() {
  messageBox()->setWindowTitle(this->previousWindowTitle_);
}

QMessageBox* AboutDialogHandler::messageBox() const {
  return static_cast<QMessageBox*>(this->parent());
}

bool AboutDialogHandler::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::MouseButtonDblClick) {
    if (!orange_->isRunning()) {
      previousWindowTitle_ = messageBox()->windowTitle();
      messageBox()->setWindowTitle("Orange");
      orange_->start();
    }
    return true;
  }

  return QObject::eventFilter(watched, event);
}

}  // namespace gui
