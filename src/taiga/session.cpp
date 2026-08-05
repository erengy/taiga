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

#include "session.hpp"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "base/string.hpp"
#include "gui/common/anime_list_view_base.hpp"
#include "gui/models/anime_list_model.hpp"
#include "gui/models/anime_list_proxy_model.hpp"
#include "media/anime_season.hpp"
#include "taiga/path.hpp"

namespace taiga {

QString Session::fileName() const {
  return u"%1/session.json"_s.arg(QString::fromStdString(get_data_path()));
}

////////////////////////////////////////////////////////////////////////////////

int Session::animeListSortColumn() const {
  return value("animeList.sortColumn", gui::AnimeListModel::COLUMN_TITLE).toInt();
}

Qt::SortOrder Session::animeListSortOrder() const {
  return value("animeList.sortOrder", Qt::SortOrder::AscendingOrder).value<Qt::SortOrder>();
}

gui::ListViewMode Session::animeListViewMode() const {
  return value("animeList.viewMode", static_cast<int>(gui::ListViewMode::List))
      .value<gui::ListViewMode>();
}

QByteArray Session::mainWindowGeometry() const {
  return QByteArray::fromBase64(value("mainWindow.geometry", QByteArray{}).toByteArray());
}

QByteArray Session::mediaDialogGeometry() const {
  return QByteArray::fromBase64(value("mediaDialog.geometry", QByteArray{}).toByteArray());
}

QByteArray Session::mediaDialogSplitterState() const {
  return QByteArray::fromBase64(value("mediaDialog.splitterState", QByteArray{}).toByteArray());
}

gui::AnimeListProxyModelFilter Session::searchListFilters() const {
  const auto json = QJsonDocument::fromJson(
      QByteArray::fromBase64(value("searchList.filters", QByteArray{}).toByteArray()));

  // clang-format off
  if (json.isEmpty()) {
    return gui::AnimeListProxyModelFilter{
        .year   = QDate::currentDate().year(),
        .season = static_cast<int>(anime::Season{QDate::currentDate().toStdSysDays()}.name),
        .type   = static_cast<int>(anime::Type::Tv),
    };
  } else {
    static const auto optionalInt = [](const QJsonValue& value) {
      return value.isDouble() ? value.toInt() : std::optional<int>{};
    };
    return gui::AnimeListProxyModelFilter{
        .year   = optionalInt(json["year"]),
        .season = optionalInt(json["season"]),
        .type   = optionalInt(json["type"]),
        .status = optionalInt(json["status"]),
        .listStatus = {
            .status = optionalInt(json["listStatus"]),
        },
    };
  }
  // clang-format on
}

int Session::searchListSortColumn() const {
  return value("searchList.sortColumn", gui::AnimeListModel::COLUMN_AVERAGE).toInt();
}

Qt::SortOrder Session::searchListSortOrder() const {
  return value("searchList.sortOrder", Qt::SortOrder::DescendingOrder).value<Qt::SortOrder>();
}

gui::ListViewMode Session::searchListViewMode() const {
  return value("searchList.viewMode", static_cast<int>(gui::ListViewMode::Cards))
      .value<gui::ListViewMode>();
}

////////////////////////////////////////////////////////////////////////////////

void Session::setAnimeListSortColumn(const int column) const {
  setValue("animeList.sortColumn", column);
}

void Session::setAnimeListSortOrder(const Qt::SortOrder order) const {
  setValue("animeList.sortOrder", order);
}

void Session::setAnimeListViewMode(const gui::ListViewMode mode) const {
  setValue("animeList.viewMode", static_cast<int>(mode));
}

void Session::setMainWindowGeometry(const QByteArray& geometry) const {
  setValue("mainWindow.geometry", geometry.toBase64().toStdString());
}

void Session::setMediaDialogGeometry(const QByteArray& geometry) const {
  setValue("mediaDialog.geometry", geometry.toBase64().toStdString());
}

void Session::setMediaDialogSplitterState(const QByteArray& state) const {
  setValue("mediaDialog.splitterState", state.toBase64().toStdString());
}

void Session::setSearchListFilters(const gui::AnimeListProxyModelFilter& filters) const {
  // clang-format off
  QJsonObject object{
      {"year",   filters.year   ? *filters.year   : QJsonValue{}},
      {"season", filters.season ? *filters.season : QJsonValue{}},
      {"type",   filters.type   ? *filters.type   : QJsonValue{}},
      {"status", filters.status ? *filters.status : QJsonValue{}},
  };
  // clang-format on
  if (filters.listStatus.status) {
    object["listStatus"] = *filters.listStatus.status;
  }
  setValue("searchList.filters",
           QJsonDocument{object}.toJson(QJsonDocument::Compact).toBase64().toStdString());
}

void Session::setSearchListSortColumn(const int column) const {
  setValue("searchList.sortColumn", column);
}

void Session::setSearchListSortOrder(const Qt::SortOrder order) const {
  setValue("searchList.sortOrder", order);
}

void Session::setSearchListViewMode(const gui::ListViewMode mode) const {
  setValue("searchList.viewMode", static_cast<int>(mode));
}

}  // namespace taiga
