// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#ifndef UNITY_DECORATIONS_MENU_LAYOUT
#define UNITY_DECORATIONS_MENU_LAYOUT

#include <UnityCore/AppmenuIndicator.h>
#include "DecorationsWidgets.h"

namespace unity
{
namespace decoration
{

class MenuLayout : public Layout
{
public:
  typedef std::shared_ptr<MenuLayout> Ptr;
  typedef unity::uweak_ptr<MenuLayout> WeakPtr;

  MenuLayout();

  nux::Property<bool> active;

  void Setup(indicator::AppmenuIndicator::Ptr const&, CompWindow*);
  void ChildrenGeometries(indicator::EntryLocationMap&) const;

protected:
  std::string GetName() const override { return "MenuLayout"; }

private:
  void OnEntryMouseOwnershipChanged(bool);
  void OnEntryActiveChanged(bool);
};

} // decoration namespace
} // unity namespace

#endif // UNITY_DECORATIONS_MENU_LAYOUT
