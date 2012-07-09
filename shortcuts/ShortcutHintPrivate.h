/*
 * Copyright (C) 2011 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 */

#ifndef UNITYSHELL_SHORTCUT_HINT_PRIVATE_H
#define UNITYSHELL_SHORTCUT_HINT_PRIVATE_H

#include <string>

namespace unity
{
namespace shortcut
{
namespace impl
{

std::string GetMetaKey(std::string const& scut);
std::string FixShortcutFormat(std::string const& scut);
std::string GetTranslatableLabel(std::string const& scut);
std::string FixMouseShortcut(std::string const& scut);
std::string ProperCase(std::string const& str);

} // namespace impl
} // namespace shortcut
} // namespace unity

#endif // UNITYSHELL_SHORTCUT_HINT_PRIVATE_H
