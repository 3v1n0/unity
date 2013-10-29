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

#include "UnityshellPrivate.h"

namespace unity
{
namespace impl
{

std::string CreateActionString(std::string const& modifiers,
                               char shortcut,
                               ActionModifiers flag)
{
  std::string ret(modifiers);

  if (flag == ActionModifiers::USE_SHIFT || flag == ActionModifiers::USE_SHIFT_NUMPAD)
    ret += "<Shift>";
  if (flag == ActionModifiers::USE_NUMPAD || flag == ActionModifiers::USE_SHIFT_NUMPAD)
    ret += "KP_";

  ret += shortcut;

  return ret;
}

} // namespace impl
} // namespace unity
