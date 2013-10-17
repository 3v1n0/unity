// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef UNITYSHELL_UNITYSHELL_PRIVATE_H
#define UNITYSHELL_UNITYSHELL_PRIVATE_H

#include <string>

namespace unity
{
namespace impl
{

enum class ActionModifiers
{
  NONE = 0,

  USE_NUMPAD,
  USE_SHIFT,
  USE_SHIFT_NUMPAD
};

std::string CreateActionString(std::string const& modifiers,
                               char shortcut,
                               ActionModifiers flag = ActionModifiers::NONE);

} // namespace impl
} // namespace unity

#endif // UNITYSHELL_UNITYSHELL_PRIVATE_H
