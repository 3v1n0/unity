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

#include "LensViewPrivate.h"
#include "AbstractPlacesGroup.h"

namespace unity
{
namespace dash
{
namespace impl
{

void UpdateDrawSeparators(std::list<AbstractPlacesGroup*> groups)
{
  std::list<AbstractPlacesGroup*>::reverse_iterator rit;
  bool found_one = false;

  for (rit = groups.rbegin(); rit != groups.rend(); ++rit)
  {
    if ((*rit)->IsVisible())
    {
      (*rit)->draw_separator = found_one;
      found_one = true;
    }
  }
}
  
}
}
}


