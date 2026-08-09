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

#include "accounts.hpp"

#include "base/string.hpp"
#include "sync/anilist/anilist_ratings.hpp"
#include "sync/kitsu/kitsu_ratings.hpp"
#include "taiga/path.hpp"

namespace taiga {

Accounts::Accounts() : QObject{} {}

QString Accounts::fileName() const {
  return u"%1/accounts.json"_s.arg(QString::fromStdString(get_data_path()));
}

////////////////////////////////////////////////////////////////////////////////

bool Accounts::anilistAuthenticated() const {
  return value("anilist.authenticated").toBool();
}

sync::anilist::RatingSystem Accounts::anilistRatingSystem() const {
  const auto ratingSystem = value("anilist.ratingSystem").toString();
  return sync::anilist::parseRatingSystem(ratingSystem);
}

std::string Accounts::anilistUsername() const {
  return value("anilist.username").toString().toStdString();
}

std::string Accounts::anilistToken() const {
  return value("anilist.token").toString().toStdString();
}

bool Accounts::kitsuAuthenticated() const {
  return value("kitsu.authenticated").toBool();
}

std::string Accounts::kitsuAccessToken() const {
  return value("kitsu.accessToken").toString().toStdString();
}

std::string Accounts::kitsuDisplayName() const {
  return value("kitsu.displayName").toString().toStdString();
}

std::string Accounts::kitsuEmail() const {
  return value("kitsu.email").toString().toStdString();
}

sync::kitsu::RatingSystem Accounts::kitsuRatingSystem() const {
  const auto ratingSystem = value("kitsu.ratingSystem").toString();
  return sync::kitsu::parseRatingSystem(ratingSystem);
}

std::string Accounts::kitsuRefreshToken() const {
  return value("kitsu.refreshToken").toString().toStdString();
}

std::string Accounts::kitsuUserId() const {
  return value("kitsu.userId").toString().toStdString();
}

std::string Accounts::kitsuUsername() const {
  return value("kitsu.username").toString().toStdString();
}

std::string Accounts::kitsuPassword() const {
  return value("kitsu.password").toString().toStdString();
}

bool Accounts::myanimelistAuthenticated() const {
  return value("myanimelist.authenticated").toBool();
}

std::string Accounts::myanimelistUsername() const {
  return value("myanimelist.username").toString().toStdString();
}

std::string Accounts::myanimelistAccessToken() const {
  return value("myanimelist.accessToken").toString().toStdString();
}

std::string Accounts::myanimelistRefreshToken() const {
  return value("myanimelist.refreshToken").toString().toStdString();
}

////////////////////////////////////////////////////////////////////////////////

void Accounts::setAnilistAuthenticated(bool authenticated) {
  setValue("anilist.authenticated", authenticated);
  emit authenticationChanged(authenticated);
}

void Accounts::setAnilistRatingSystem(const std::string& ratingSystem) const {
  setValue("anilist.ratingSystem", ratingSystem);
}

void Accounts::setAnilistUsername(const std::string& username) const {
  setValue("anilist.username", username);
}

void Accounts::setAnilistToken(const std::string& token) const {
  setValue("anilist.token", token);
}

void Accounts::setKitsuAuthenticated(bool authenticated) {
  setValue("kitsu.authenticated", authenticated);
  emit authenticationChanged(authenticated);
}

void Accounts::setKitsuAccessToken(const std::string& accessToken) const {
  setValue("kitsu.accessToken", accessToken);
}

void Accounts::setKitsuDisplayName(const std::string& displayName) const {
  setValue("kitsu.displayName", displayName);
}

void Accounts::setKitsuEmail(const std::string& email) const {
  setValue("kitsu.email", email);
}

void Accounts::setKitsuRatingSystem(const std::string& ratingSystem) const {
  setValue("kitsu.ratingSystem", ratingSystem);
}

void Accounts::setKitsuRefreshToken(const std::string& refreshToken) const {
  setValue("kitsu.refreshToken", refreshToken);
}

void Accounts::setKitsuUserId(const std::string& userId) const {
  setValue("kitsu.userId", userId);
}

void Accounts::setKitsuUsername(const std::string& username) const {
  setValue("kitsu.username", username);
}

void Accounts::setKitsuPassword(const std::string& password) const {
  setValue("kitsu.password", password);
}

void Accounts::setMyanimelistAuthenticated(bool authenticated) {
  setValue("myanimelist.authenticated", authenticated);
  emit authenticationChanged(authenticated);
}

void Accounts::setMyanimelistUsername(const std::string& username) const {
  setValue("myanimelist.username", username);
}

void Accounts::setMyanimelistAccessToken(const std::string& accessToken) const {
  setValue("myanimelist.accessToken", accessToken);
}

void Accounts::setMyanimelistRefreshToken(const std::string& refreshToken) const {
  setValue("myanimelist.refreshToken", refreshToken);
}

////////////////////////////////////////////////////////////////////////////////

std::string Accounts::serviceUsername(const std::string& service) const {
  if (service == "anilist") return anilistUsername();
  if (service == "kitsu") return kitsuUsername();
  if (service == "myanimelist") return myanimelistUsername();
  return {};
}

}  // namespace taiga
