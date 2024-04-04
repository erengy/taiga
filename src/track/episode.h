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

#include <string>

#include <anitomy/anitomy/element.h>

namespace anime {

using number_range_t = std::pair<int, int>;

class Episode {
 public:
  Episode();
  virtual ~Episode() {}

  void Clear();
  void Set(int anime_id);

  anitomy::Elements& elements();
  const anitomy::Elements& elements() const;

  int anime_season() const;
  const std::wstring& anime_title() const;
  const std::wstring& anime_type() const;
  int anime_year() const;
  std::wstring audio_terms() const;
  int episode_number() const;
  number_range_t episode_number_range() const;
  const std::wstring& episode_title() const;
  const std::wstring& file_checksum() const;
  const std::wstring& file_extension() const;
  const std::wstring& file_name() const;
  std::wstring file_name_with_extension() const;
  const std::wstring& release_group() const;
  int release_version() const;
  const std::wstring& video_resolution() const;
  std::wstring video_terms() const;
  int volume_number() const;
  number_range_t volume_number_range() const;

  void set_anime_season(int value);
  void set_anime_title(const std::wstring& str);
  void set_anime_type(const std::wstring& str);
  void set_anime_year(int value);
  void set_audio_terms(const std::wstring& str);
  void set_elements(const anitomy::Elements& elements);
  void set_episode_number(int value);
  void set_episode_number_range(std::pair<int, int> range);
  void set_episode_title(const std::wstring& str);
  void set_file_checksum(const std::wstring& str);
  void set_file_extension(const std::wstring& str);
  void set_file_name(const std::wstring& str);
  void set_release_group(const std::wstring& str);
  void set_release_version(int value);
  void set_video_resolution(const std::wstring& str);
  void set_video_terms(const std::wstring& str);
  void set_volume_number(int value);

  int GetElementAsInt(anitomy::ElementCategory category, int default_value = 0) const;
  const std::wstring& GetElementAsString(anitomy::ElementCategory category) const;
  number_range_t GetElementAsRange(anitomy::ElementCategory category) const;
  std::wstring GetElementsAsString(anitomy::ElementCategory category) const;

  void SetElementValue(anitomy::ElementCategory category, int value);
  void SetElementValue(anitomy::ElementCategory category, const std::wstring& str);

  int anime_id;
  std::wstring folder;
  bool processed;

private:
  anitomy::Elements elements_;
};

}  // namespace anime

inline anime::Episode CurrentEpisode;
