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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include "GLibSource.h"
#include "AppmenuIndicator.h"

namespace unity
{
namespace indicator
{

struct AppmenuIndicator::Impl
{
  Impl(AppmenuIndicator* parent)
  {
    // When the active window has changed we might need to emit an updated signal
    parent->active_window.changed.connect([this, parent] (unsigned long) {
      update_wait_.reset(new glib::Timeout(250, [parent] {
        parent->updated.emit();
        return false;
      }));
    });

    parent->updated.connect([this] { update_wait_.reset(); });
  }

  glib::Source::UniquePtr update_wait_;
};

AppmenuIndicator::AppmenuIndicator(std::string const& name)
  : Indicator(name)
  , active_window(0)
  , impl_(new AppmenuIndicator::Impl(this))
{}

AppmenuIndicator::~AppmenuIndicator()
{}

void AppmenuIndicator::ShowAppmenu(unsigned int xid, int x, int y) const
{
  on_show_appmenu.emit(xid, x, y);
}

} // namespace indicator
} // namespace unity
