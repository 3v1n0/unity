/*
 * Copyright (C) 2012 Canonical Ltd
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

#ifndef UNITYSHELL_LENS_VIEW_PRIVATE_H
#define UNITYSHELL_LENS_VIEW_PRIVATE_H

#include <list>

namespace unity
{
namespace dash
{

class AbstractPlacesGroup;

namespace impl
{

void UpdateDrawSeparators(std::list<AbstractPlacesGroup*> groups);

} // namespace impl
} // namespace dash
} // namespace unity

#endif
