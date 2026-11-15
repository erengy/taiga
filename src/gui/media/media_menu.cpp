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

#include "media_menu.hpp"

#include <QClipboard>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>
#include <chrono>
#include <limits>
#include <ranges>

#include "base/log.hpp"
#include "base/string.hpp"
#include "gui/main/main_window.hpp"
#include "gui/main/status_bar_controller.hpp"
#include "gui/media/media_dialog.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/rating.hpp"
#include "gui/utils/theme.hpp"
#include "gui/utils/widgets.hpp"
#include "media/anime.hpp"
#include "media/anime_db.hpp"
#include "media/anime_list.hpp"
#include "media/anime_list_utils.hpp"
#include "media/anime_utils.hpp"
#include "sync/service.hpp"
#include "taiga/settings.hpp"
#include "track/media.hpp"
#include "track/play.hpp"
#include "track/scanner.hpp"

namespace gui {

namespace {

// Parent dialogs to the window, so that they don't match stylesheet rules meant for the widget.
QWidget* dialogParent(const QWidget* widget) {
  return widget ? widget->window() : nullptr;
}

}  // namespace

MediaMenu::MediaMenu(QWidget* parent, const QList<Anime>& items,
                     const QMap<int, ListEntry>& entries, QItemSelectionModel* selectionModel,
                     AnimeListContext context)
    : QMenu(parent),
      items_(items),
      entries_(entries),
      selectionModel_(selectionModel),
      context_(context) {
  setAttribute(Qt::WA_DeleteOnClose);
}

void MediaMenu::popup() {
  if (items_.empty()) return;

  addMediaItems();
  addSeparator();
  addListItems();
  addSeparator();
  addLibraryItems();
  addSeparator();
  addTorrentsItems();
  addSeparator();
  addMetaItems();

  QMenu::popup(QCursor::pos());
}

bool MediaMenu::isBatch() const {
  return items_.size() > 1;
}

bool MediaMenu::isInList() const {
  return entries_.size() == items_.size();
}

bool MediaMenu::canMatchNowPlaying() const {
  const auto* detection = track::media::detection();
  return detection->getCurrentEpisode().has_value() && !detection->isMediaIdentified();
}

void MediaMenu::addToList(const anime::list::Status status) const {
  for (const auto& item : items_) {
    if (getEntry(item.id)) continue;
    anime::list::save(ListEntry{.animeId = item.id, .status = status});
  }
}

void MediaMenu::clearDateCompleted() const {
  for (const auto& item : items_) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.animeId = item.id};
    updated.dateCompleted = FuzzyDate{};
    anime::list::save(updated);
  }
}

void MediaMenu::clearDateStarted() const {
  for (const auto& item : items_) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.animeId = item.id};
    updated.dateStarted = FuzzyDate{};
    anime::list::save(updated);
  }
}

void MediaMenu::copyLinks() const {
  QList<QString> links;
  for (const auto& item : items_) {
    links.push_back(sync::animePageUrl(item.id));
  }
  QGuiApplication::clipboard()->setText(links.join("\n"));
}

void MediaMenu::copyTitles() const {
  QList<QString> titles;
  for (const auto& item : items_) {
    titles.push_back(QString::fromStdString(anime::preferredTitle(item)));
  }
  QGuiApplication::clipboard()->setText(titles.join("\n"));
}

void MediaMenu::editEpisode() const {
  QSet<int> watchedEpisodes;
  int maxValue = anime::kMaxEpisodeCount;

  for (const auto& item : items_) {
    if (const auto entry = getEntry(item.id)) watchedEpisodes.insert(entry->watchedEpisodes);
    if (item.episodeCount > 0) maxValue = std::min(maxValue, item.episodeCount);
  }

  const int initalValue = watchedEpisodes.size() == 1 ? *watchedEpisodes.cbegin() : 0;

  bool ok = false;
  const auto value = QInputDialog::getInt(dialogParent(parentWidget()), tr("Edit Episodes Watched"),
                                          tr("Enter a number:"), initalValue, 0, maxValue, 1, &ok);
  if (!ok) return;

  for (const auto& item : items_) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.animeId = item.id};
    updated.watchedEpisodes = value;
    anime::list::save(updated);
  }
}

void MediaMenu::editNotes() const {
  bool ok = false;
  const auto notes = QInputDialog::getMultiLineText(dialogParent(parentWidget()), tr("Edit Notes"),
                                                    tr("Enter notes:"), "", &ok);
  if (!ok) return;

  for (const auto& item : items_) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.animeId = item.id};
    updated.notes = notes.toStdString();
    anime::list::save(updated);
  }
}

void MediaMenu::editScore(const int value) const {
  for (const auto& item : items_) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.animeId = item.id};
    updated.score = value;
    anime::list::save(updated);
  }
}

void MediaMenu::editStatus(const anime::list::Status status) const {
  for (const auto& item : items_) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.animeId = item.id};
    updated.status = status;
    anime::list::save(updated);
  }
}

void MediaMenu::matchNowPlaying() const {
  if (items_.empty()) return;

  const auto& item = items_.front();

  if (const auto episode = track::media::detection()->getCurrentEpisode()) {
    const auto title = episode->element(anitomy::ElementKind::Title);
    if (!title.empty()) {
      const auto* settings = anime::db()->settings(item.id);
      auto updated = settings ? *settings : anime::Settings{.id = item.id};
      if (!std::ranges::contains(updated.synonyms, title)) {
        updated.synonyms.push_back(title);
        anime::db()->updateSettings(updated);
      }
    }
  }

  track::media::detection()->setCurrentEpisodeAnimeId(item.id);
}

void MediaMenu::openFolder() const {
  const auto& item = items_.front();

  const auto libraryFolders = taiga::settings()->libraryFolders();

  for (const auto& path : libraryFolders) {
    const auto folder = track::findFolder(QString::fromStdString(path), item.id);
    if (folder) {
      qDebug() << "Found folder:" << *folder;
      QDesktopServices::openUrl(QUrl::fromLocalFile(*folder));
      return;
    }
  }

  QMessageBox::information(nullptr, tr("Open Folder"),
                           tr("Could not find folder for %1.").arg(anime::preferredTitle(item)));
}

void MediaMenu::playEpisode(int number) const {
  const auto& item = items_.front();

  if (track::playEpisode(item.id, number)) {
    mainWindow()->statusBarController()->clearMessage(StatusBarController::Source::Playback);
    return;
  }

  mainWindow()->statusBarController()->showMessage({
      .source = StatusBarController::Source::Playback,
      .text = tr("Could not find episode #%1 (%2).").arg(number).arg(anime::preferredTitle(item)),
      .spin = false,
  });
}

void MediaMenu::playRandomEpisode() const {
  const auto& item = items_.front();
  const auto number = track::randomEpisodeNumber(item.id);

  if (!number) {
    mainWindow()->statusBarController()->clearMessage(StatusBarController::Source::Playback);
    return;
  }

  if (track::playEpisode(item.id, *number)) {
    mainWindow()->statusBarController()->clearMessage(StatusBarController::Source::Playback);
    return;
  }

  mainWindow()->statusBarController()->showMessage({
      .source = StatusBarController::Source::Playback,
      .text = tr("Could not find episode #%1 (%2).").arg(*number).arg(anime::preferredTitle(item)),
      .spin = false,
  });
}

void MediaMenu::refresh() const {
  for (const auto& item : items_) {
    sync::fetchAnime(item.id);
  }
}

void MediaMenu::removeFromList() const {
  QList<QString> titles;
  for (const auto& item : items_) {
    titles.push_back(u"<li>%1</li>"_s.arg(anime::preferredTitle(item)));
  }

  if (confirm(dialogParent(parentWidget()),
              tr("Do you want to remove selected items from your list?"),
              u"<ul>%1</ul>"_s.arg(titles.join("")), tr("Remove"))) {
    for (const auto& item : items_) {
      anime::list::remove(item.id);
    }
  }
}

void MediaMenu::search() const {
  const auto& item = items_.front();
  mainWindow()->navigateTo(MainWindowPage::Search);
  mainWindow()->searchBox()->setText(QString::fromStdString(anime::preferredTitle(item)));
}

void MediaMenu::searchAniDB() const {
  for (const auto& item : items_) {
    QUrl url{"https://anidb.net/anime/"};
    url.setQuery({{"adb.search", QString::fromStdString(anime::preferredTitle(item))}});
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchAniList() const {
  for (const auto& item : items_) {
    if (sync::currentServiceId() == sync::ServiceId::AniList) {
      const QUrl url{sync::animePageUrl(item.id)};
      QDesktopServices::openUrl(url);
    } else {
      QUrl url{"https://anilist.co/search/anime"};
      QUrlQuery query{{"search", QString::fromStdString(anime::preferredTitle(item))}};
      if (anime::isNsfw(item)) query.addQueryItem("adult", "true");
      url.setQuery(query);
      QDesktopServices::openUrl(url);
    }
  }
}

void MediaMenu::searchANN() const {
  for (const auto& item : items_) {
    QUrl url{"https://www.animenewsnetwork.com/search"};
    url.setQuery({{"q", QString::fromStdString(anime::preferredTitle(item))}});
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchKitsu() const {
  for (const auto& item : items_) {
    if (sync::currentServiceId() == sync::ServiceId::Kitsu) {
      const QUrl url{sync::animePageUrl(item.id)};
      QDesktopServices::openUrl(url);
    } else {
      QUrl url{"https://kitsu.app/anime"};
      url.setQuery({{"text", QString::fromStdString(anime::preferredTitle(item))}});
      QDesktopServices::openUrl(url);
    }
  }
}

void MediaMenu::searchMyAnimeList() const {
  for (const auto& item : items_) {
    if (sync::currentServiceId() == sync::ServiceId::MyAnimeList) {
      const QUrl url{sync::animePageUrl(item.id)};
      QDesktopServices::openUrl(url);
    } else {
      QUrl url{"https://myanimelist.net/anime.php"};
      url.setQuery({{"q", QString::fromStdString(anime::preferredTitle(item))}});
      QDesktopServices::openUrl(url);
    }
  }
}

void MediaMenu::searchReddit() const {
  for (const auto& item : items_) {
    QUrl url{"https://www.reddit.com/search"};
    const auto title = QString::fromStdString(anime::preferredTitle(item));
    url.setQuery({
        {"q", u"subreddit:anime title:%1 episode discussion"_s.arg(title)},
        {"sort", "new"},
    });
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchWikipedia() const {
  for (const auto& item : items_) {
    QUrl url{"https://en.wikipedia.org/wiki/Special:Search"};
    url.setQuery({{"search", QString::fromStdString(anime::preferredTitle(item))}});
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchYouTube() const {
  for (const auto& item : items_) {
    if (!item.trailerId.empty()) {
      const QUrl url{u"https://youtu.be/%1"_s.arg(item.trailerId)};
      QDesktopServices::openUrl(url);
    } else {
      QUrl url{"https://www.youtube.com/results"};
      url.setQuery({{"search_query", QString::fromStdString(anime::preferredTitle(item))}});
      QDesktopServices::openUrl(url);
    }
  }
}

void MediaMenu::setDateCompletedToAiringEnd() const {
  for (const auto& item : items_) {
    if (!item.dateFinished) continue;
    const auto* entry = getEntry(item.id);
    if (entry && entry->dateCompleted) continue;
    auto updated = entry ? *entry : ListEntry{.animeId = item.id};
    updated.dateCompleted = item.dateFinished;
    anime::list::save(updated);
  }
}

void MediaMenu::setDateCompletedToLastUpdated() const {
  for (const auto& item : items_) {
    const auto* entry = getEntry(item.id);
    if (!entry || entry->dateCompleted || !entry->lastUpdated) continue;
    const auto date = std::chrono::floor<std::chrono::days>(
        std::chrono::system_clock::from_time_t(entry->lastUpdated));
    auto updated = *entry;
    updated.dateCompleted = FuzzyDate{date};
    anime::list::save(updated);
  }
}

void MediaMenu::setDateStartedToAiringStart() const {
  for (const auto& item : items_) {
    if (!item.dateStarted) continue;
    const auto* entry = getEntry(item.id);
    if (entry && entry->dateStarted) continue;
    auto updated = entry ? *entry : ListEntry{.animeId = item.id};
    updated.dateStarted = item.dateStarted;
    anime::list::save(updated);
  }
}

void MediaMenu::startNewRewatch() const {
  const auto& item = items_.front();

  const auto* entry = getEntry(item.id);
  auto updated = entry ? *entry : ListEntry{.animeId = item.id};
  updated.status = anime::list::Status::Watching;
  updated.rewatching = true;
  updated.watchedEpisodes = 0;
  anime::list::save(updated);

  playEpisode(1);
}

void MediaMenu::torrents() const {
  const auto& item = items_.front();
  mainWindow()->navigateTo(MainWindowPage::Torrents);
  mainWindow()->searchBox()->setText(QString::fromStdString(anime::preferredTitle(item)));
}

void MediaMenu::viewDetails() const {
  if (items_.empty()) return;

  const auto& anime = items_.front();
  MediaDialog::show(parentWidget(), MediaDialogPage::Details, anime);
}

void MediaMenu::edit() const {
  if (items_.empty()) return;

  const auto& anime = items_.front();
  MediaDialog::show(parentWidget(), MediaDialogPage::List, anime);
}

void MediaMenu::addMediaItems() {
  if (!isBatch()) {
    // View details
    addAction(theme()->getIcon("info"), tr("Details"), tr("Enter"), this, &MediaMenu::viewDetails);

    if (context_ != AnimeListContext::Search) {
      // Search
      addAction(theme()->getIcon("search"), tr("Search"), this, &MediaMenu::search);
    }
  }

  if (context_ == AnimeListContext::Search) {
    // Refresh
    addAction(theme()->getIcon("sync"), tr("Refresh"), this, &MediaMenu::refresh);
  }

  // External
  addMenu([this]() {
    auto menu = new QMenu(tr("External"), this);
    menu->setIcon(theme()->getIcon("open_in_new"));

    using slot_t = void (MediaMenu::*)() const;
    const QList<QPair<QString, slot_t>> items = {
        {"AniDB", &MediaMenu::searchAniDB},
        {"AniList", &MediaMenu::searchAniList},
        {"Anime News Network", &MediaMenu::searchANN},
        {"Kitsu", &MediaMenu::searchKitsu},
        {"MyAnimeList", &MediaMenu::searchMyAnimeList},
        {"Reddit", &MediaMenu::searchReddit},
        {"Wikipedia", &MediaMenu::searchWikipedia},
        {"YouTube", &MediaMenu::searchYouTube},
    };
    for (const auto& [text, slot] : items) {
      menu->addAction(text, this, slot);
    }

    return menu;
  }());
}

void MediaMenu::addListItems() {
  if (!isInList()) {
    // Add to list
    addMenu([this]() {
      auto menu = new QMenu(tr("Add to list"), this);
      menu->setIcon(theme()->getIcon("list_alt"));
      for (const auto& status : anime::list::kStatuses) {
        menu->addAction(formatListStatus(status), this, [this, status]() { addToList(status); });
      }
      return menu;
    }());

    return;
  }

  // Edit
  if (!isBatch()) {
    addAction(theme()->getIcon("edit"), tr("Edit..."), this, &MediaMenu::edit);

  } else {
    addMenu([this]() {
      auto menu = new QMenu(tr("Edit"), this);
      menu->setIcon(theme()->getIcon("edit"));

      menu->addMenu([this]() {
        auto menu = new QMenu(tr("Date started"), this);
        menu->addAction(tr("Clear"), this, &MediaMenu::clearDateStarted);
        menu->addAction(tr("Set to date started airing"), this,
                        &MediaMenu::setDateStartedToAiringStart);
        return menu;
      }());

      menu->addMenu([this]() {
        auto menu = new QMenu(tr("Date completed"), this);
        menu->addAction(tr("Clear"), this, &MediaMenu::clearDateCompleted);
        menu->addAction(tr("Set to date finished airing"), this,
                        &MediaMenu::setDateCompletedToAiringEnd);
        menu->addAction(tr("Set to last updated"), this, &MediaMenu::setDateCompletedToLastUpdated);
        return menu;
      }());

      menu->addAction(tr("Episode..."), this, &MediaMenu::editEpisode);
      menu->addAction(tr("Notes..."), this, &MediaMenu::editNotes);

      menu->addMenu([this]() {
        auto menu = new QMenu(tr("Score"), this);

        QSet<int> scores;
        for (const auto& item : items_) {
          if (const auto* entry = getEntry(item.id)) scores.insert(entry->score);
        }
        const bool hasCommonScore = scores.size() == 1;

        for (const auto& rating : currentRatingList()) {
          auto action = new QAction(rating.text, this);
          action->setCheckable(true);
          action->setChecked(hasCommonScore && *scores.begin() == rating.value);
          menu->addAction(action);
          connect(action, &QAction::triggered, this, [this, rating]() { editScore(rating.value); });
        }
        return menu;
      }());

      menu->addMenu([this]() {
        auto menu = new QMenu(tr("Status"), this);

        QSet<int> statuses;
        for (const auto& item : items_) {
          if (const auto* entry = getEntry(item.id)) {
            statuses.insert(static_cast<int>(entry->status));
          }
        }
        const bool hasCommonStatus = statuses.size() == 1;

        for (const auto status : anime::list::kStatuses) {
          auto action = new QAction(formatListStatus(status), this);
          action->setCheckable(true);
          action->setChecked(hasCommonStatus && *statuses.begin() == static_cast<int>(status));
          menu->addAction(action);
          connect(action, &QAction::triggered, this, [this, status]() { editStatus(status); });
        }
        return menu;
      }());

      return menu;
    }());
  }

  // Remove from list
  addAction(theme()->getIcon("delete"), tr("Remove..."), QKeySequence::Delete, this,
            &MediaMenu::removeFromList);
}

void MediaMenu::addLibraryItems() {
  // Open folder
  addAction(theme()->getIcon("folder"), tr("Open folder"), this, &MediaMenu::openFolder);

  if (isBatch()) return;

  const auto& item = items_.front();
  const auto entry = getEntry(item.id);

  // Play
  addMenu([this, &item, entry]() {
    const int totalEpisodes = std::max(item.episodeCount, 0);
    const int watchedEpisodes = entry ? entry->watchedEpisodes : 0;
    const int maxEpisodes = totalEpisodes ? totalEpisodes : std::numeric_limits<int>::max();
    const int lastEpisode = std::min(watchedEpisodes, maxEpisodes);
    const int nextEpisode = lastEpisode + 1;

    auto menu = new QMenu(tr("Play"), this);
    menu->setIcon(theme()->getIcon("play_arrow"));

    // Play next episode
    if (nextEpisode <= maxEpisodes) {
      menu->addAction(theme()->getIcon("skip_next"), tr("Next episode (#%1)").arg(nextEpisode),
                      this, [this, nextEpisode]() { playEpisode(nextEpisode); });
    }

    // Play last episode
    if (lastEpisode > 0) {
      menu->addAction(tr("Last episode (#%1)").arg(lastEpisode), this,
                      [this, lastEpisode]() { playEpisode(lastEpisode); });
    }

    // Play random episode
    if (item.episodeCount != 1) {
      menu->addAction(theme()->getIcon("shuffle"), tr("Random episode"), this,
                      &MediaMenu::playRandomEpisode);
    }

    // Play episode
    if (totalEpisodes > 1) {
      menu->addSeparator();
      menu->addMenu([this, totalEpisodes, lastEpisode]() {
        auto menu = new QMenu(tr("Episode"), this);
        for (int i = 1; i <= totalEpisodes; ++i) {
          auto action = new QAction(u"#%1"_s.arg(i), this);
          action->setCheckable(true);
          action->setChecked(i <= lastEpisode);
          menu->addAction(action);
          connect(action, &QAction::triggered, this, [this, i]() { playEpisode(i); });
        }
        return menu;
      }());
    }

    // Start new rewatch
    if (entry && !entry->rewatching && totalEpisodes > 0 &&
        entry->watchedEpisodes == totalEpisodes && item.status == anime::Status::FinishedAiring) {
      menu->addSeparator();
      menu->addAction(tr("Start new rewatch"), this, &MediaMenu::startNewRewatch);
    }

    return menu;
  }());
}

void MediaMenu::addTorrentsItems() {
  if (isBatch()) return;

  // Torrents
  addAction(theme()->getIcon("rss_feed"), tr("Torrents"), this, &MediaMenu::torrents)
      ->setDisabled(true);  // placeholder
}

void MediaMenu::addMetaItems() {
  addMenu([this]() {
    auto menu = new QMenu(tr("Copy"), this);
    menu->setIcon(theme()->getIcon("content_copy"));
    menu->addAction(isBatch() ? tr("Titles") : tr("Title"), this, &MediaMenu::copyTitles);
    menu->addAction(isBatch() ? tr("Links") : tr("Link"), this, &MediaMenu::copyLinks);
    return menu;
  }());

  if (isBatch() && selectionModel_) {
    addAction(tr("Invert selection"), this, [this]() {
      const auto* model = selectionModel_->model();
      if (model->rowCount() == 0) return;
      const QItemSelection all(model->index(0, 0),
                               model->index(model->rowCount() - 1, model->columnCount() - 1));
      selectionModel_->select(all, QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
    });
  }

  if (canMatchNowPlaying() && !isBatch()) {
    addAction(tr("Set as now playing..."), this, &MediaMenu::matchNowPlaying);
  }
}

const ListEntry* MediaMenu::getEntry(int id) const {
  const auto it = entries_.find(id);
  return it != entries_.end() ? &*it : nullptr;
}

}  // namespace gui
