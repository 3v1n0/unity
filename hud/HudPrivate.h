/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 *
 */

#ifndef UNITYSHELL_HUD_PRIVATE_H
#define UNITYSHELL_HUD_PRIVATE_H

#include <string>
#include <utility>
#include <vector>

namespace unity
{
namespace hud
{
namespace impl
{

std::vector<std::pair<std::string, bool>> RefactorText(std::string const& text);

} // namespace impl
} // namespace hud
} // namespace unity

#endif // UNITYSHELL_HUD_PRIVATE_H
