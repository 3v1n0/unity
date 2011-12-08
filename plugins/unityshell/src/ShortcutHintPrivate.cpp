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

#include "ShortcutHintPrivate.h"

#include <algorithm>
#include <boost/regex.hpp> 

namespace unity
{
namespace shortcut
{
namespace impl
{

std::string FixShortcutFormat(std::string const& scut)
{
  std::string ret = scut;
  
  // Removes all the '<'
  ret.erase(std::remove(ret.begin(), ret.end(), '<'), ret.end());

  // FIXME
    
  return ret;
}

} // namespace impl
} // namespace shortcut
} // namespace unity

