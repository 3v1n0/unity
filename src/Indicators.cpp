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
 *              Tim Penhey <tim.penhey@canonical.com>
 */
#include "Indicators.h"

#include <vector>

namespace unity {
namespace indicator {

class IndicatorsImpl
{
public:
  typedef std::vector<Indicator::Ptr> Collection;

  IndicatorsImpl(Indicators* owner)
    : owner_(owner)
    {}

private:
  Indicators* owner_;
  Collection indicators_;
};


Indicators::Indicators()
  : pimpl_(new IndicatorsImpl(this))
{

}

  // For adding factory-specific properties
  virtual void AddProperties(GVariantBuilder *builder) = 0;

  // Signals
  sigc::signal<void, Indicator::Ptr const&> on_object_added;
  sigc::signal<void, Indicator::Ptr const&> on_object_removed;
  sigc::signal<void, int, int> on_menu_pointer_moved;
  sigc::signal<void, std::string const&> on_entry_activate_request;
  sigc::signal<void, std::string const&> on_entry_activated;
  sigc::signal<void> on_synced;

private:
  boost::scoped_ptr<IndicatorsImpl> pimpl_;
};



} // namespace indicator
} // namespace unity
