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
#include "settings_page_advanced.hpp"

#include <QComboBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QNetworkProxy>
#include <QSpinBox>
#include <QTableWidget>
#include <algorithm>

#include "taiga/settings.hpp"
#include "ui_settings_dialog.h"

namespace gui {

namespace {

enum Row {
  ProxyType,
  ProxyHost,
  ProxyPort,
  ProxyUsername,
  ProxyPassword,
  RowCount,
};

constexpr int kNameColumn = 0;
constexpr int kValueColumn = 1;

}  // namespace

SettingsPageAdvanced::SettingsPageAdvanced(Ui::SettingsDialog* ui, QDialog* dialog)
    : SettingsPage(ui, dialog),
      proxyTypeComboBox_(new QComboBox(ui->advancedTable)),
      proxyPortSpinBox_(new QSpinBox(ui->advancedTable)),
      proxyPasswordLineEdit_(new QLineEdit(ui->advancedTable)) {
  auto table = ui_->advancedTable;
  table->setRowCount(RowCount);
  table->horizontalHeader()->setSectionResizeMode(kNameColumn, QHeaderView::ResizeToContents);

  const auto setName = [table](const Row row, const QString& name) {
    auto item = new QTableWidgetItem(name);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    table->setItem(row, kNameColumn, item);
  };
  setName(ProxyType, tr("Proxy type"));
  setName(ProxyHost, tr("Proxy host"));
  setName(ProxyPort, tr("Proxy port"));
  setName(ProxyUsername, tr("Proxy username"));
  setName(ProxyPassword, tr("Proxy password"));

  proxyTypeComboBox_->addItem(tr("HTTP"), static_cast<int>(QNetworkProxy::HttpProxy));
  proxyTypeComboBox_->addItem(tr("SOCKS5"), static_cast<int>(QNetworkProxy::Socks5Proxy));
  table->setCellWidget(ProxyType, kValueColumn, proxyTypeComboBox_);

  table->setItem(ProxyHost, kValueColumn, new QTableWidgetItem());

  proxyPortSpinBox_->setRange(0, 65535);
  proxyPortSpinBox_->setSpecialValueText(tr("Not set"));
  proxyPortSpinBox_->setFrame(false);
  table->setCellWidget(ProxyPort, kValueColumn, proxyPortSpinBox_);

  table->setItem(ProxyUsername, kValueColumn, new QTableWidgetItem());

  proxyPasswordLineEdit_->setEchoMode(QLineEdit::Password);
  proxyPasswordLineEdit_->setFrame(false);
  table->setCellWidget(ProxyPassword, kValueColumn, proxyPasswordLineEdit_);
}

void SettingsPageAdvanced::load() {
  auto table = ui_->advancedTable;

  proxyTypeComboBox_->setCurrentIndex(
      std::max(0, proxyTypeComboBox_->findData(static_cast<int>(taiga::settings.proxyType()))));
  table->item(ProxyHost, kValueColumn)
      ->setText(QString::fromStdString(taiga::settings.proxyHost()));
  proxyPortSpinBox_->setValue(std::max(0, taiga::settings.proxyPort()));
  table->item(ProxyUsername, kValueColumn)
      ->setText(QString::fromStdString(taiga::settings.proxyUsername()));
  proxyPasswordLineEdit_->setText(QString::fromStdString(taiga::settings.proxyPassword()));
}

void SettingsPageAdvanced::apply() const {
  const auto table = ui_->advancedTable;

  taiga::settings.setProxyType(
      static_cast<QNetworkProxy::ProxyType>(proxyTypeComboBox_->currentData().toInt()));
  taiga::settings.setProxyHost(
      table->item(ProxyHost, kValueColumn)->text().trimmed().toStdString());
  taiga::settings.setProxyPort(proxyPortSpinBox_->value() > 0 ? proxyPortSpinBox_->value() : -1);
  taiga::settings.setProxyUsername(table->item(ProxyUsername, kValueColumn)->text().toStdString());
  taiga::settings.setProxyPassword(proxyPasswordLineEdit_->text().toStdString());
}

}  // namespace gui
