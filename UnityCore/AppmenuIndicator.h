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

#ifndef UNITY_APPMENU_INDICATOR_H
#define UNITY_APPMENU_INDICATOR_H

#include "Indicator.h"

namespace unity
{
namespace indicator
{

class AppmenuIndicator : public Indicator
{
public:
  typedef std::shared_ptr<AppmenuIndicator> Ptr;

  AppmenuIndicator(std::string const& name);

  virtual bool IsAppmenu() const { return true; }

  void ShowAppmenu(unsigned xid, int x, int y) const;

  sigc::signal<void, unsigned, int, int> on_show_appmenu;
};

}
}

#endif // UNITY_APPMENU_INDICATOR_H
