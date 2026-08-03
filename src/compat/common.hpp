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

#pragma once

class QString;

namespace compat::v1 {

// Removes the extra root element from v1's XML documents so that they can be read by
// `QXmlStreamReader` without an "Extra content at end of document" error.
// See issue #842 for more information.
void removeMetaElement(QString& str);

// Removes numeric references to control characters that XML 1.0 disallows (e.g. `&#03;` for mIRC's
// color-code prefix), which make `QXmlStreamReader` fail with "Invalid character reference".
void removeInvalidCharacterReferences(QString& str);

}  // namespace compat::v1
