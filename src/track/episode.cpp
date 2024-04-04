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

#include "track/episode.h"

#include "base/string.h"
#include "media/anime.h"
#include "media/anime_db.h"
#include "ui/menu.h"

namespace anime {

Episode::Episode()
    : anime_id(ID_UNKNOWN), processed(false) {
}

void Episode::Clear() {
  anime_id = ID_UNKNOWN;
  elements_.clear();
  folder.clear();
  processed = false;
}

void Episode::Set(int anime_id) {
  this->anime_id = anime_id;
  this->processed = false;
  ui::Menus.UpdateAll(anime::db.Find(anime_id));
}

////////////////////////////////////////////////////////////////////////////////

anitomy::Elements& Episode::elements() {
  return elements_;
}

const anitomy::Elements& Episode::elements() const {
  return elements_;
}

////////////////////////////////////////////////////////////////////////////////

int Episode::anime_season() const {
  return GetElementAsInt(anitomy::kElementAnimeSeason);
}

const std::wstring& Episode::anime_title() const {
  return GetElementAsString(anitomy::kElementAnimeTitle);
}

const std::wstring& Episode::anime_type() const {
  return GetElementAsString(anitomy::kElementAnimeType);
}

int Episode::anime_year() const {
  return GetElementAsInt(anitomy::kElementAnimeYear);
}

std::wstring Episode::audio_terms() const {
  return GetElementsAsString(anitomy::kElementAudioTerm);
}

int Episode::episode_number() const {
  return GetElementAsInt(anitomy::kElementEpisodeNumber);
}

number_range_t Episode::episode_number_range() const {
  return GetElementAsRange(anitomy::kElementEpisodeNumber);
}

const std::wstring& Episode::episode_title() const {
  return GetElementAsString(anitomy::kElementEpisodeTitle);
}

const std::wstring& Episode::file_checksum() const {
  return GetElementAsString(anitomy::kElementFileChecksum);
}

const std::wstring& Episode::file_extension() const {
  return GetElementAsString(anitomy::kElementFileExtension);
}

const std::wstring& Episode::file_name() const {
  return GetElementAsString(anitomy::kElementFileName);
}

std::wstring Episode::file_name_with_extension() const {
  auto filename = GetElementAsString(anitomy::kElementFileName);
  auto extension = file_extension();
  if (!extension.empty())
    filename += L"." + extension;
  return filename;
}

const std::wstring& Episode::release_group() const {
  return GetElementAsString(anitomy::kElementReleaseGroup);
}

int Episode::release_version() const {
  return GetElementAsInt(anitomy::kElementReleaseVersion, 1);
}

const std::wstring& Episode::video_resolution() const {
  return GetElementAsString(anitomy::kElementVideoResolution);
}

std::wstring Episode::video_terms() const {
  return GetElementsAsString(anitomy::kElementVideoTerm);
}

int Episode::volume_number() const {
  return GetElementAsInt(anitomy::kElementVolumeNumber);
}

number_range_t Episode::volume_number_range() const {
  return GetElementAsRange(anitomy::kElementVolumeNumber);
}

////////////////////////////////////////////////////////////////////////////////

void Episode::set_anime_season(int value) {
  SetElementValue(anitomy::kElementAnimeSeason, value);
}

void Episode::set_anime_title(const std::wstring& str) {
  SetElementValue(anitomy::kElementAnimeTitle, str);
}

void Episode::set_anime_type(const std::wstring& str) {
  SetElementValue(anitomy::kElementAnimeType, str);
}

void Episode::set_anime_year(int value) {
  SetElementValue(anitomy::kElementAnimeYear, value);
}

void Episode::set_audio_terms(const std::wstring& str) {
  SetElementValue(anitomy::kElementAudioTerm, str);
}

void Episode::set_elements(const anitomy::Elements& elements) {
  elements_ = elements;
}

void Episode::set_episode_number(int value) {
  SetElementValue(anitomy::kElementEpisodeNumber, value);
}

void Episode::set_episode_number_range(std::pair<int, int> range) {
  elements_.erase(anitomy::kElementEpisodeNumber);
  elements_.insert(anitomy::kElementEpisodeNumber, ToWstr(range.first));
  if (range.second > range.first)
    elements_.insert(anitomy::kElementEpisodeNumber, ToWstr(range.second));
}

void Episode::set_episode_title(const std::wstring& str) {
  SetElementValue(anitomy::kElementEpisodeTitle, str);
}

void Episode::set_file_checksum(const std::wstring& str) {
  SetElementValue(anitomy::kElementFileChecksum, str);
}

void Episode::set_file_extension(const std::wstring& str) {
  SetElementValue(anitomy::kElementFileExtension, str);
}

void Episode::set_file_name(const std::wstring& str) {
  SetElementValue(anitomy::kElementFileName, str);
}

void Episode::set_release_group(const std::wstring& str) {
  SetElementValue(anitomy::kElementReleaseGroup, str);
}

void Episode::set_release_version(int value) {
  SetElementValue(anitomy::kElementReleaseVersion, value);
}

void Episode::set_video_resolution(const std::wstring& str) {
  SetElementValue(anitomy::kElementVideoResolution, str);
}

void Episode::set_video_terms(const std::wstring& str) {
  SetElementValue(anitomy::kElementVideoTerm, str);
}

void Episode::set_volume_number(int value) {
  SetElementValue(anitomy::kElementVolumeNumber, value);
}

////////////////////////////////////////////////////////////////////////////////

int Episode::GetElementAsInt(anitomy::ElementCategory category,
                             int default_value) const {
  auto it = elements_.find(category);
  return it != elements_.end() ? ToInt(it->second) : default_value;
}

const std::wstring& Episode::GetElementAsString(
    anitomy::ElementCategory category) const {
  auto it = elements_.find(category);
  return it != elements_.end() ? it->second : EmptyString();
}

number_range_t Episode::GetElementAsRange(
    anitomy::ElementCategory category) const {
  const auto numbers = elements_.get_all(category);
  number_range_t range{
    numbers.empty() ? 0 : ToInt(numbers.front()),
    numbers.empty() ? 0 : ToInt(numbers.back())
  };
  return range;
}

std::wstring Episode::GetElementsAsString(
    anitomy::ElementCategory category) const {
  return Join(elements_.get_all(category), L" ");
}

////////////////////////////////////////////////////////////////////////////////

void Episode::SetElementValue(anitomy::ElementCategory category, int value) {
  elements_.set(category, ToWstr(value));
}

void Episode::SetElementValue(anitomy::ElementCategory category,
                              const std::wstring& str) {
  elements_.set(category, str);
}

}  // namespace anime
