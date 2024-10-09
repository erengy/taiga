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

#pragma once

#include <QList>
#include <QMap>
#include <QMenu>

#include "media/anime.hpp"

namespace gui {

class MediaMenu final : public QMenu {
  Q_OBJECT
  Q_DISABLE_COPY_MOVE(MediaMenu)

public:
  MediaMenu(QWidget* parent, const QList<Anime>& items, const QMap<int, ListEntry> entries);
  ~MediaMenu() = default;

  void popup();

private slots:
  void addToList(const anime::list::Status status) const;
  void editStatus(const anime::list::Status status) const;
  void playEpisode(int number) const;
  void removeFromList() const;
  void searchAniDB() const;
  void searchAniList() const;
  void searchANN() const;
  void searchKitsu() const;
  void searchMyAnimeList() const;
  void searchReddit() const;
  void searchWikipedia() const;
  void searchYouTube() const;
  void test() const;  // @TEMP
  void viewDetails() const;

private:
  void addMediaItems();
  void addListItems();
  void addLibraryItems();
  void addNowPlayingItems();

  bool isBatch() const;
  bool isInList() const;
  bool isNowPlaying() const;

  const ListEntry* getEntry(int id) const;

  const QList<Anime> m_items;
  const QMap<int, ListEntry> m_entries;
};

}  // namespace gui
