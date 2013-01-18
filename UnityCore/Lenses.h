// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef UNITY_LENSES_H
#define UNITY_LENSES_H

#include <sigc++/trackable.h>
#include <sigc++/signal.h>

#include "Lens.h"

namespace unity
{
namespace dash
{

class Lenses : public sigc::trackable, boost::noncopyable
{
public:
  typedef std::vector<Lens::Ptr> LensList;

  /**
   * Get the currently loaded Lenses. This is necessary as some of the consumers
   * of this object employ a lazy-loading technique to reduce the overhead of
   * starting Unity. Therefore, the Lenses may already have been loaded by the time
   * the objects have been initiated (and so just connecting to the signals is not
   * enough)
   */
  virtual LensList GetScopes() const = 0;
  virtual Lens::Ptr GetScope(std::string const& lens_id) const = 0;
  virtual Lens::Ptr GetScopeAtIndex(std::size_t index) const = 0;

  nux::ROProperty<std::size_t> count;

  sigc::signal<void, Lens::Ptr&> scope_added;
};

typedef Lenses Scopes;

}
}

#endif
