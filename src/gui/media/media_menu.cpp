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
#include "track/recognition_cache.hpp"
#include "track/scanner.hpp"

namespace gui {

MediaMenu::MediaMenu(QWidget* parent, const QList<Anime>& items, const QMap<int, ListEntry> entries,
                     QItemSelectionModel* selectionModel, AnimeListContext context)
    : QMenu(parent),
      m_items(items),
      m_entries(entries),
      m_selectionModel(selectionModel),
      m_context(context) {
  setAttribute(Qt::WA_DeleteOnClose);
}

void MediaMenu::popup() {
  if (m_items.empty()) return;

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
  return m_items.size() > 1;
}

bool MediaMenu::isInList() const {
  return m_entries.size() == m_items.size();
}

bool MediaMenu::canMatchNowPlaying() const {
  const auto* detection = track::media::detection();
  return detection->getCurrentEpisode().has_value() && !detection->isMediaIdentified();
}

void MediaMenu::addToList(const anime::list::Status status) const {
  for (const auto& item : m_items) {
    if (getEntry(item.id)) continue;
    anime::list::save(ListEntry{.anime_id = item.id, .status = status});
  }
}

void MediaMenu::clearDateCompleted() const {
  for (const auto& item : m_items) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.anime_id = item.id};
    updated.date_completed = FuzzyDate{};
    anime::list::save(updated);
  }
}

void MediaMenu::clearDateStarted() const {
  for (const auto& item : m_items) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.anime_id = item.id};
    updated.date_started = FuzzyDate{};
    anime::list::save(updated);
  }
}

void MediaMenu::copyLinks() const {
  QList<QString> links;
  for (const auto& item : m_items) {
    links.push_back(sync::animePageUrl(item.id));
  }
  QGuiApplication::clipboard()->setText(links.join("\n"));
}

void MediaMenu::copyTitles() const {
  QList<QString> titles;
  for (const auto& item : m_items) {
    titles.push_back(QString::fromStdString(anime::preferredTitle(item)));
  }
  QGuiApplication::clipboard()->setText(titles.join("\n"));
}

void MediaMenu::editEpisode() const {
  QSet<int> watchedEpisodes;
  int maxValue = anime::kMaxEpisodeCount;

  for (const auto& item : m_items) {
    if (const auto entry = getEntry(item.id)) watchedEpisodes.insert(entry->watched_episodes);
    if (item.episode_count > 0) maxValue = std::min(maxValue, item.episode_count);
  }

  const int initalValue = watchedEpisodes.size() == 1 ? watchedEpisodes.values().front() : 0;

  bool ok = false;
  const auto value = QInputDialog::getInt(parentWidget(), tr("Edit Episodes Watched"),
                                          tr("Enter a number:"), initalValue, 0, maxValue, 1, &ok);
  if (!ok) return;

  for (const auto& item : m_items) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.anime_id = item.id};
    updated.watched_episodes = value;
    anime::list::save(updated);
  }
}

void MediaMenu::editNotes() const {
  bool ok = false;
  const auto notes =
      QInputDialog::getMultiLineText(parentWidget(), tr("Edit Notes"), tr("Enter notes:"), "", &ok);
  if (!ok) return;

  for (const auto& item : m_items) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.anime_id = item.id};
    updated.notes = notes.toStdString();
    anime::list::save(updated);
  }
}

void MediaMenu::editScore(const int value) const {
  for (const auto& item : m_items) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.anime_id = item.id};
    updated.score = value;
    anime::list::save(updated);
  }
}

void MediaMenu::editStatus(const anime::list::Status status) const {
  for (const auto& item : m_items) {
    const auto* entry = getEntry(item.id);
    auto updated = entry ? *entry : ListEntry{.anime_id = item.id};
    updated.status = status;
    anime::list::save(updated);
  }
}

void MediaMenu::matchNowPlaying() const {
  if (m_items.empty()) return;

  const auto& item = m_items.front();

  if (const auto episode = track::media::detection()->getCurrentEpisode()) {
    const auto title = episode->element(anitomy::ElementKind::Title);
    if (!title.empty()) {
      const auto* settings = anime::db.settings(item.id);
      auto updated = settings ? *settings : anime::Settings{.id = item.id};
      if (!std::ranges::contains(updated.synonyms, title)) {
        updated.synonyms.push_back(title);
        anime::db.updateSettings(updated);
        if (const auto* anime = anime::db.item(item.id)) {
          track::recognition::cache()->update(*anime);
        }
      }
    }
  }

  track::media::detection()->setCurrentEpisodeAnimeId(item.id);
}

void MediaMenu::openFolder() const {
  const auto& item = m_items.front();

  const auto libraryFolders = taiga::settings.libraryFolders();

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
  const auto& item = m_items.front();

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
  const auto& item = m_items.front();
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
  for (const auto& item : m_items) {
    sync::fetchAnime(item.id);
  }
}

void MediaMenu::removeFromList() const {
  QList<QString> titles;
  for (const auto& item : m_items) {
    titles.push_back(u"<li>%1</li>"_s.arg(QString::fromStdString(anime::preferredTitle(item))));
  }

  if (confirm(parentWidget(), tr("Do you want to remove selected items from your list?"),
              u"<ul>%1</ul>"_s.arg(titles.join("")), tr("Remove"))) {
    for (const auto& item : m_items) {
      anime::list::remove(item.id);
    }
  }
}

void MediaMenu::search() const {
  const auto& item = m_items.front();
  mainWindow()->navigateTo(MainWindowPage::Search);
  mainWindow()->searchBox()->setText(QString::fromStdString(anime::preferredTitle(item)));
}

void MediaMenu::searchAniDB() const {
  for (const auto& item : m_items) {
    QUrl url{"https://anidb.net/anime/"};
    url.setQuery({{"adb.search", QString::fromStdString(anime::preferredTitle(item))}});
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchAniList() const {
  for (const auto& item : m_items) {
    if (sync::currentServiceId() == sync::ServiceId::AniList) {
      QUrl url{sync::animePageUrl(item.id)};
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
  for (const auto& item : m_items) {
    QUrl url{"https://www.animenewsnetwork.com/search"};
    url.setQuery({{"q", QString::fromStdString(anime::preferredTitle(item))}});
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchKitsu() const {
  for (const auto& item : m_items) {
    if (sync::currentServiceId() == sync::ServiceId::Kitsu) {
      QUrl url{sync::animePageUrl(item.id)};
      QDesktopServices::openUrl(url);
    } else {
      QUrl url{"https://kitsu.app/anime"};
      url.setQuery({{"text", QString::fromStdString(anime::preferredTitle(item))}});
      QDesktopServices::openUrl(url);
    }
  }
}

void MediaMenu::searchMyAnimeList() const {
  for (const auto& item : m_items) {
    if (sync::currentServiceId() == sync::ServiceId::MyAnimeList) {
      QUrl url{sync::animePageUrl(item.id)};
      QDesktopServices::openUrl(url);
    } else {
      QUrl url{"https://myanimelist.net/anime.php"};
      url.setQuery({{"q", QString::fromStdString(anime::preferredTitle(item))}});
      QDesktopServices::openUrl(url);
    }
  }
}

void MediaMenu::searchReddit() const {
  for (const auto& item : m_items) {
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
  for (const auto& item : m_items) {
    QUrl url{"https://en.wikipedia.org/wiki/Special:Search"};
    url.setQuery({{"search", QString::fromStdString(anime::preferredTitle(item))}});
    QDesktopServices::openUrl(url);
  }
}

void MediaMenu::searchYouTube() const {
  for (const auto& item : m_items) {
    if (!item.trailer_id.empty()) {
      QUrl url{u"https://youtu.be/%1"_s.arg(QString::fromStdString(item.trailer_id))};
      QDesktopServices::openUrl(url);
    } else {
      QUrl url{"https://www.youtube.com/results"};
      url.setQuery({{"search_query", QString::fromStdString(anime::preferredTitle(item))}});
      QDesktopServices::openUrl(url);
    }
  }
}

void MediaMenu::setDateCompletedToAiringEnd() const {
  for (const auto& item : m_items) {
    if (!item.date_finished) continue;
    const auto* entry = getEntry(item.id);
    if (entry && entry->date_completed) continue;
    auto updated = entry ? *entry : ListEntry{.anime_id = item.id};
    updated.date_completed = item.date_finished;
    anime::list::save(updated);
  }
}

void MediaMenu::setDateCompletedToLastUpdated() const {
  for (const auto& item : m_items) {
    const auto* entry = getEntry(item.id);
    if (!entry || entry->date_completed || !entry->last_updated) continue;
    const auto date = std::chrono::floor<std::chrono::days>(
        std::chrono::system_clock::from_time_t(entry->last_updated));
    auto updated = *entry;
    updated.date_completed = FuzzyDate{date};
    anime::list::save(updated);
  }
}

void MediaMenu::setDateStartedToAiringStart() const {
  for (const auto& item : m_items) {
    if (!item.date_started) continue;
    const auto* entry = getEntry(item.id);
    if (entry && entry->date_started) continue;
    auto updated = entry ? *entry : ListEntry{.anime_id = item.id};
    updated.date_started = item.date_started;
    anime::list::save(updated);
  }
}

void MediaMenu::startNewRewatch() const {
  const auto& item = m_items.front();

  const auto* entry = getEntry(item.id);
  auto updated = entry ? *entry : ListEntry{.anime_id = item.id};
  updated.status = anime::list::Status::Watching;
  updated.rewatching = true;
  updated.watched_episodes = 0;
  anime::list::save(updated);

  playEpisode(1);
}

void MediaMenu::torrents() const {
  const auto& item = m_items.front();
  mainWindow()->navigateTo(MainWindowPage::Torrents);
  mainWindow()->searchBox()->setText(QString::fromStdString(anime::preferredTitle(item)));
}

void MediaMenu::viewDetails() const {
  if (m_items.empty()) return;

  const auto& anime = m_items.front();
  MediaDialog::show(parentWidget(), MediaDialogPage::Details, anime);
}

void MediaMenu::edit() const {
  if (m_items.empty()) return;

  const auto& anime = m_items.front();
  MediaDialog::show(parentWidget(), MediaDialogPage::List, anime);
}

void MediaMenu::addMediaItems() {
  if (!isBatch()) {
    // View details
    addAction(theme.getIcon("info"), tr("Details"), tr("Enter"), this, &MediaMenu::viewDetails);

    if (m_context != AnimeListContext::Search) {
      // Search
      addAction(theme.getIcon("search"), tr("Search"), this, &MediaMenu::search);
    }
  }

  if (m_context == AnimeListContext::Search) {
    // Refresh
    addAction(theme.getIcon("sync"), tr("Refresh"), this, &MediaMenu::refresh);
  }

  // External
  addMenu([this]() {
    auto menu = new QMenu(tr("External"), this);
    menu->setIcon(theme.getIcon("open_in_new"));

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
    for (const auto [text, slot] : items) {
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
      menu->setIcon(theme.getIcon("list_alt"));
      for (const auto& status : anime::list::kStatuses) {
        menu->addAction(formatListStatus(status), this, [this, status]() { addToList(status); });
      }
      return menu;
    }());

    return;
  }

  // Edit
  if (!isBatch()) {
    addAction(theme.getIcon("edit"), tr("Edit..."), this, &MediaMenu::edit);

  } else {
    addMenu([this]() {
      auto menu = new QMenu(tr("Edit"), this);
      menu->setIcon(theme.getIcon("edit"));

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
        for (const auto& item : m_items) {
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
        for (const auto& item : m_items) {
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
  addAction(theme.getIcon("delete"), tr("Remove..."), QKeySequence::Delete, this,
            &MediaMenu::removeFromList);
}

void MediaMenu::addLibraryItems() {
  // Open folder
  addAction(theme.getIcon("folder"), tr("Open folder"), this, &MediaMenu::openFolder);

  if (isBatch()) return;

  const auto& item = m_items.front();
  const auto entry = getEntry(item.id);

  // Play
  addMenu([this, &item, entry]() {
    const int total_episodes = std::max(item.episode_count, 0);
    const int watched_episodes = entry ? entry->watched_episodes : 0;
    const int max_episodes = total_episodes ? total_episodes : std::numeric_limits<int>::max();
    const int last_episode = std::min(watched_episodes, max_episodes);
    const int next_episode = last_episode + 1;

    auto menu = new QMenu(tr("Play"), this);
    menu->setIcon(theme.getIcon("play_arrow"));

    // Play next episode
    if (next_episode <= max_episodes) {
      menu->addAction(theme.getIcon("skip_next"), tr("Next episode (#%1)").arg(next_episode), this,
                      [this, next_episode]() { playEpisode(next_episode); });
    }

    // Play last episode
    if (last_episode > 0) {
      menu->addAction(tr("Last episode (#%1)").arg(last_episode), this,
                      [this, last_episode]() { playEpisode(last_episode); });
    }

    // Play random episode
    if (item.episode_count != 1) {
      menu->addAction(theme.getIcon("shuffle"), tr("Random episode"), this,
                      &MediaMenu::playRandomEpisode);
    }

    // Play episode
    if (total_episodes > 1) {
      menu->addSeparator();
      menu->addMenu([this, total_episodes, last_episode]() {
        auto menu = new QMenu(tr("Episode"), this);
        for (int i = 1; i <= total_episodes; ++i) {
          auto action = new QAction(u"#%1"_s.arg(i), this);
          action->setCheckable(true);
          action->setChecked(i <= last_episode);
          menu->addAction(action);
          connect(action, &QAction::triggered, this, [this, i]() { playEpisode(i); });
        }
        return menu;
      }());
    }

    // Start new rewatch
    if (entry && !entry->rewatching && total_episodes > 0 &&
        entry->watched_episodes == total_episodes && item.status == anime::Status::FinishedAiring) {
      menu->addSeparator();
      menu->addAction(tr("Start new rewatch"), this, &MediaMenu::startNewRewatch);
    }

    return menu;
  }());
}

void MediaMenu::addTorrentsItems() {
  if (isBatch()) return;

  // Torrents
  addAction(theme.getIcon("rss_feed"), tr("Torrents"), this, &MediaMenu::torrents);
}

void MediaMenu::addMetaItems() {
  addMenu([this]() {
    auto menu = new QMenu(tr("Copy"), this);
    menu->setIcon(theme.getIcon("content_copy"));
    menu->addAction(isBatch() ? tr("Titles") : tr("Title"), this, &MediaMenu::copyTitles);
    menu->addAction(isBatch() ? tr("Links") : tr("Link"), this, &MediaMenu::copyLinks);
    return menu;
  }());

  if (isBatch() && m_selectionModel) {
    addAction(tr("Invert selection"), this, [this]() {
      const auto* model = m_selectionModel->model();
      if (model->rowCount() == 0) return;
      const QItemSelection all(model->index(0, 0),
                               model->index(model->rowCount() - 1, model->columnCount() - 1));
      m_selectionModel->select(all, QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
    });
  }

  if (canMatchNowPlaying() && !isBatch()) {
    addAction(tr("Set as now playing..."), this, &MediaMenu::matchNowPlaying);
  }
}

const ListEntry* MediaMenu::getEntry(int id) const {
  const auto it = m_entries.find(id);
  return it != m_entries.end() ? &*it : nullptr;
}

}  // namespace gui
