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

#include "media_dialog.hpp"

#include <QDesktopServices>
#include <QFileDialog>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QUrl>
#include <algorithm>

#include "base/string.hpp"
#include "gui/common/poster_widget.hpp"
#include "gui/utils/format.hpp"
#include "gui/utils/image_provider.hpp"
#include "gui/utils/rating.hpp"
#include "gui/utils/theme.hpp"
#include "gui/utils/widgets.hpp"
#include "media/anime_db.hpp"
#include "media/anime_list_utils.hpp"
#include "media/anime_season.hpp"
#include "media/anime_utils.hpp"
#include "sync/service.hpp"
#include "taiga/session.hpp"
#include "ui_media_dialog.h"

#ifdef Q_OS_WINDOWS
#include "gui/platforms/windows.hpp"
#endif

namespace gui {

MediaDialog::MediaDialog(QWidget* parent) : QDialog(parent), ui_(new Ui::MediaDialog) {
  ui_->setupUi(this);

#ifdef Q_OS_WINDOWS
  enableMicaBackground(this);
#endif

  if (const auto geometry = taiga::session.mediaDialogGeometry(); !geometry.isEmpty()) {
    restoreGeometry(geometry);
    centerWidgetToScreen(this);
  }

  ui_->splitter->setSizes(
      {ui_->posterWidget->minimumWidth(), ui_->posterWidget->minimumWidth() * 4});
  if (const auto state = taiga::session.mediaDialogSplitterState(); !state.isEmpty()) {
    ui_->splitter->restoreState(state);
  }

  ui_->tabWidget->setTabIcon(static_cast<int>(MediaDialogPage::Details), theme()->getIcon("info"));
  ui_->tabWidget->setTabIcon(static_cast<int>(MediaDialogPage::List), theme()->getIcon("list_alt"));
  ui_->tabWidget->setTabIcon(static_cast<int>(MediaDialogPage::Settings),
                             theme()->getIcon("settings"));

  ui_->verticalLayoutRewatching->setAlignment(Qt::AlignBottom);

  connect(imageProvider(), &ImageProvider::posterChanged, this, [this](int id) {
    if (id == anime_.id) loadPosterImage();
  });

  connect(anime::db(), &anime::Database::itemUpdated, this, [this](const int id) {
    if (id != anime_.id) return;
    anime_ = *anime::db()->item(id);
    initTitles();
    initDetails();
    initSettings();
  });

  connect(anime::db(), &anime::Database::entryUpdated, this, [this](const int id) {
    if (id != anime_.id) return;
    initList();
  });

  connect(anime::db(), &anime::Database::entryDeleted, this, [this](const int id) {
    if (id != anime_.id) return;
    initList();
  });

  connect(ui_->posterWidget, &PosterWidget::clicked, this, [this](Qt::MouseButton button) {
    if (button == Qt::MouseButton::LeftButton) {
      const QUrl url{sync::animePageUrl(anime_.id)};
      QDesktopServices::openUrl(url);
    }
  });

  connect(ui_->splitter, &QSplitter::splitterMoved, this, [this]() { resizePosterImage(); });

  connect(ui_->checkRewatching, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
    const bool isChecked = state == Qt::CheckState::Checked;
    const int progress = ui_->spinProgress->value();

    // Set status
    switch (sync::currentServiceId()) {
      case sync::ServiceId::MyAnimeList:
        // MyAnimeList tracks rewatching independently of status.
        break;
      case sync::ServiceId::Kitsu:
      case sync::ServiceId::AniList: {
        const int status =
            static_cast<int>(isChecked ? anime::list::Status::Watching : entry_->status);
        if (const int index = ui_->comboStatus->findData(status); index > -1) {
          ui_->comboStatus->setCurrentIndex(index);
        }
        break;
      }
    }

    // Reset progress
    if (isChecked) {
      const bool isCompleted = entry_->status == anime::list::Status::Completed;
      if (isCompleted && progress == entry_->watchedEpisodes) {
        ui_->spinProgress->setValue(0);
      }
    } else {
      if (progress == 0) {
        ui_->spinProgress->setValue(entry_->watchedEpisodes);
      }
    }
  });

  connect(ui_->comboStatus, &QComboBox::currentIndexChanged, this, [this](int index) {
    const int status = ui_->comboStatus->itemData(index).toInt();
    if (status != static_cast<int>(anime::list::Status::Completed)) return;
    if (entry_->status == anime::list::Status::Completed) return;
    if (anime_.episodeCount < 1) return;
    ui_->spinProgress->setValue(anime_.episodeCount);
  });

  connect(ui_->checkDateStarted, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState state) {
    ui_->dateStarted->setEnabled(state == Qt::CheckState::Checked);
  });
  connect(ui_->checkDateCompleted, &QCheckBox::checkStateChanged, this,
          [this](Qt::CheckState state) {
            ui_->dateCompleted->setEnabled(state == Qt::CheckState::Checked);
          });

  connect(ui_->browseLibraryFolderButton, &QToolButton::clicked, this, [this]() {
    constexpr auto options =
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks | QFileDialog::ReadOnly;
    const auto directory = QFileDialog::getExistingDirectory(this, tr("Browse Folder"),
                                                             ui_->libraryFolder->text(), options);
    if (!directory.isEmpty()) {
      ui_->libraryFolder->setText(directory);
    }
  });
}

void MediaDialog::closeEvent(QCloseEvent* event) {
  taiga::session.setMediaDialogGeometry(saveGeometry());
  taiga::session.setMediaDialogSplitterState(ui_->splitter->saveState());
  event->accept();
}

void MediaDialog::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_F5) {
    imageProvider()->fetchPoster(anime_.id);
    sync::fetchAnime(anime_.id);
    return;
  }

  QDialog::keyPressEvent(event);
}

void MediaDialog::resizeEvent(QResizeEvent* event) {
  QDialog::resizeEvent(event);
  resizePosterImage();
}

void MediaDialog::showEvent(QShowEvent* event) {
  QDialog::showEvent(event);
  resizePosterImage();
}

void MediaDialog::show(QWidget* parent, MediaDialogPage page, const Anime& anime) {
  // Parent to the window, so that the dialog doesn't match stylesheet rules meant for the widget.
  auto* dlg = new MediaDialog(parent ? parent->window() : nullptr);
  dlg->setAttribute(Qt::WA_DeleteOnClose);
  dlg->setAnime(anime);
  dlg->ui_->tabWidget->setCurrentIndex(static_cast<int>(page));
  dlg->QDialog::show();
}

void MediaDialog::setAnime(const Anime& anime) {
  anime_ = anime;

  loadPosterImage();
  initTitles();
  initDetails();
  initList();
  initSettings();

  if (anime::isStale(anime)) {
    sync::fetchAnime(anime.id);
  }
}

void MediaDialog::initTitles() {
  const auto mainTitle = QString::fromStdString(anime::preferredTitle(anime_));
  setWindowTitle(mainTitle);
  ui_->titleLabel->setText(mainTitle);

  QList<QString> altTitles;
  const auto addTitle = [&mainTitle, &altTitles](const QString& title) {
    if (title.isEmpty() || title == mainTitle) return;
    altTitles.push_back(title);
  };
  addTitle(QString::fromStdString(anime_.titles.romaji));
  addTitle(QString::fromStdString(anime_.titles.english));
  addTitle(QString::fromStdString(anime_.titles.japanese));
  if (!altTitles.isEmpty()) ui_->altTitlesLabel->setText(altTitles.join(", "));
  ui_->altTitlesLabel->setHidden(altTitles.isEmpty());
}

void MediaDialog::initDetails() {
  while (ui_->infoLayout->rowCount() > 0) {
    ui_->infoLayout->removeRow(0);
  }

  const auto getRowTitle = [this](const QString& text) {
    auto* label = new QLabel(text, this);
    label->setAlignment(Qt::AlignRight | Qt::AlignTop);
    label->setFont([label]() {
      auto font = label->font();
      font.setWeight(QFont::Weight::DemiBold);
      return font;
    }());
    return label;
  };

  const auto getRowLabel = [this](const QString& text) {
    auto* label = new QLabel(text, this);
    label->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
    label->setTextInteractionFlags(Qt::TextInteractionFlag::TextSelectableByMouse);
    label->setOpenExternalLinks(true);
    label->setWordWrap(true);
    return label;
  };

  auto seasonLabel = new QLabel(formatSeason(anime::Season(anime_.dateStarted)), this);
  seasonLabel->setCursor(QCursor(Qt::CursorShape::WhatsThisCursor));
  seasonLabel->setToolTip(formatFuzzyDateRange(anime_.dateStarted, anime_.dateFinished));

  if (!anime_.titles.synonyms.empty()) {
    ui_->infoLayout->addRow(getRowTitle(tr("Titles:")),
                            getRowLabel(joinStrings(anime_.titles.synonyms)));
  }
  ui_->infoLayout->addRow(getRowTitle(tr("Type:")), getRowLabel(formatType(anime_.type)));
  ui_->infoLayout->addRow(getRowTitle(tr("Episodes:")),
                          getRowLabel(formatNumber(anime_.episodeCount, "?")));
  if (anime_.episodeLength > 0 && anime_.type != anime::Type::Tv) {
    const auto duration = formatEpisodeLength(anime_.episodeLength);
    const auto label = anime_.episodeCount == 1 ? duration : u"%1 per episode"_s.arg(duration);
    ui_->infoLayout->addRow(getRowTitle(tr("Duration:")), getRowLabel(label));
  }
  ui_->infoLayout->addRow(getRowTitle(tr("Status:")), getRowLabel(formatStatus(anime_.status)));
  ui_->infoLayout->addRow(getRowTitle(tr("Season:")), seasonLabel);
  ui_->infoLayout->addRow(getRowTitle(tr("Score:")), getRowLabel(formatScore(anime_.score)));
  if (!anime_.genres.empty()) {
    ui_->infoLayout->addRow(getRowTitle(tr("Genres:")), getRowLabel(joinStrings(anime_.genres)));
  }
  if (!anime_.tags.empty()) {
    ui_->infoLayout->addRow(getRowTitle(tr("Tags:")), getRowLabel(joinStrings(anime_.tags)));
  }
  if (!anime_.studios.empty()) {
    ui_->infoLayout->addRow(getRowTitle(tr("Studios:")), getRowLabel(joinStrings(anime_.studios)));
  }
  if (!anime_.producers.empty()) {
    ui_->infoLayout->addRow(getRowTitle(tr("Producers:")),
                            getRowLabel(joinStrings(anime_.producers)));
  }

  const auto synopsis = QString::fromStdString(anime_.synopsis);

  ui_->synopsisHeader->setHidden(synopsis.isEmpty());

  ui_->synopsis->document()->setDocumentMargin(0);
  ui_->synopsis->viewport()->setAutoFillBackground(false);
  ui_->synopsis->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
  ui_->synopsis->setHtml(synopsis);
  ui_->synopsis->setHidden(synopsis.isEmpty());
}

void MediaDialog::initList() {
  const auto entry = anime::db()->entry(anime_.id);
  entry_ = entry ? std::optional<ListEntry>{*entry} : std::nullopt;

  ui_->tabWidget->setTabVisible(1, entry_.has_value());

  if (!entry_.has_value()) return;

  // Episodes watched
  if (anime_.episodeCount > 0) {
    ui_->spinProgress->setMaximum(anime_.episodeCount);
  }
  ui_->spinProgress->setValue(entry_->watchedEpisodes);

  // Times rewatched
  ui_->spinRewatches->setValue(entry_->rewatchedTimes);

  // Rewatching
  {
    const QSignalBlocker blocker(ui_->checkRewatching);
    ui_->checkRewatching->setChecked(entry_->rewatching);
  }

  // Status
  ui_->comboStatus->clear();
  for (const auto status : anime::list::kStatuses) {
    ui_->comboStatus->addItem(formatListStatus(status), static_cast<int>(status));
    if (status == entry_->status) {
      ui_->comboStatus->setCurrentIndex(ui_->comboStatus->count() - 1);
    }
  }

  // Score
  const bool useScoreSpinBox = usesRatingSpinBox();
  ui_->comboScore->setVisible(!useScoreSpinBox);
  ui_->spinScore->setVisible(useScoreSpinBox);
  if (useScoreSpinBox) {
    populateRatingSpinBox(ui_->spinScore);
    setRatingSpinBoxValue(ui_->spinScore, entry_->score);
  } else {
    populateRatingComboBox(ui_->comboScore);
    setRatingComboBoxValue(ui_->comboScore, entry_->score);
  }

  const auto fuzzyToDate = [](const FuzzyDate& date) {
    return QDate{date.year(), date.month(), date.day()};
  };

  // Date started & completed
  ui_->checkDateStarted->setChecked(static_cast<bool>(entry_->dateStarted));
  ui_->checkDateCompleted->setChecked(static_cast<bool>(entry_->dateCompleted));
  if (anime_.dateStarted) {
    ui_->dateStarted->setMinimumDate(fuzzyToDate(anime_.dateStarted));
    ui_->dateCompleted->setMinimumDate(fuzzyToDate(anime_.dateStarted));
  }
  ui_->dateStarted->setDate(entry_->dateStarted ? fuzzyToDate(entry_->dateStarted)
                                                : QDate::currentDate());
  ui_->dateCompleted->setDate(entry_->dateCompleted ? fuzzyToDate(entry_->dateCompleted)
                                                    : QDate::currentDate());

  // Notes
  ui_->plainTextEditNotes->setPlainText(QString::fromStdString(entry_->notes));
}

void MediaDialog::initSettings() {
  const auto settings = anime::db()->settings(anime_.id);
  settings_ = settings ? *settings : anime::Settings{.id = anime_.id};

  ui_->comboDisplayTitle->clear();

  QList<QString> candidates;
  const auto addCandidate = [&candidates](const QString& title) {
    if (!title.isEmpty() && !candidates.contains(title)) {
      candidates.push_back(title);
    }
  };
  addCandidate(QString::fromStdString(anime_.titles.romaji));
  addCandidate(QString::fromStdString(anime_.titles.english));
  addCandidate(QString::fromStdString(anime_.titles.japanese));
  for (const auto& synonym : anime_.titles.synonyms) {
    addCandidate(QString::fromStdString(synonym));
  }
  addCandidate(QString::fromStdString(settings_.displayTitle));
  ui_->comboDisplayTitle->addItems(candidates);

  ui_->comboDisplayTitle->setCurrentText(QString::fromStdString(settings_.displayTitle));
  ui_->comboDisplayTitle->lineEdit()->setPlaceholderText(
      QString::fromStdString(anime::preferredTitle(anime_)));

  QStringList synonyms;
  for (const auto& synonym : settings_.synonyms) {
    synonyms.push_back(QString::fromStdString(synonym));
  }
  ui_->recognitionTitles->setPlainText(synonyms.join("\n"));

  ui_->libraryFolder->setText(QString::fromStdString(settings_.folder));
}

void MediaDialog::loadPosterImage() {
  const auto posterPixmap = imageProvider()->loadPoster(anime_.id);
  ui_->posterWidget->setPixmap(posterPixmap);
  ui_->posterWidget->setLoading(posterPixmap.isNull() && !anime_.imageUrl.empty());
  resizePosterImage();
}

void MediaDialog::resizePosterImage() {
  const auto& posterPixmap = ui_->posterWidget->pixmap();
  const int width = ui_->posterWidget->width();

  if (posterPixmap.isNull()) {
    ui_->posterWidget->setFixedHeight(width * 3 / 2);
  } else {
    const int w = posterPixmap.width();
    const int h = posterPixmap.height();
    const int height = h * (width / static_cast<float>(w));
    ui_->posterWidget->setFixedHeight(height);
  }
}

void MediaDialog::accept() {
  if (entry_) {
    entry_->watchedEpisodes = ui_->spinProgress->value();
    entry_->rewatchedTimes = ui_->spinRewatches->value();
    entry_->rewatching = ui_->checkRewatching->isChecked();
    entry_->status = ui_->comboStatus->currentData().value<anime::list::Status>();
    entry_->score = usesRatingSpinBox() ? ratingSpinBoxValue(ui_->spinScore)
                                        : ui_->comboScore->currentData().toInt();
    entry_->dateStarted = ui_->checkDateStarted->isChecked()
                              ? FuzzyDate{ui_->dateStarted->date().toStdSysDays()}
                              : FuzzyDate{};
    entry_->dateCompleted = ui_->checkDateCompleted->isChecked()
                                ? FuzzyDate{ui_->dateCompleted->date().toStdSysDays()}
                                : FuzzyDate{};
    entry_->notes = ui_->plainTextEditNotes->toPlainText().toStdString();

    anime::list::save(*entry_);
  }

  settings_.displayTitle = ui_->comboDisplayTitle->currentText().trimmed().toStdString();
  settings_.folder = ui_->libraryFolder->text().trimmed().toStdString();

  settings_.synonyms.clear();
  for (const auto& line : ui_->recognitionTitles->toPlainText().split("\n")) {
    const auto synonym = line.trimmed().toStdString();
    if (synonym.empty()) continue;
    if (!std::ranges::contains(settings_.synonyms, synonym)) {
      settings_.synonyms.push_back(synonym);
    }
  }

  anime::db()->updateSettings(settings_);

  QDialog::accept();
}

}  // namespace gui
