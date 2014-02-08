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
#include <UnityCore/Indicators.h>
#include <UnityCore/GLibSource.h>
#include "DecorationsWidgets.h"

namespace unity
{
namespace decoration
{
class MenuDropdown;

class MenuLayout : public Layout
{
public:
  MenuLayout(indicator::Indicators::Ptr const&, CompWindow*);

  nux::Property<bool> active;
  nux::Property<bool> show_now;

  void SetAppMenu(indicator::AppmenuIndicator::Ptr const&);
  void ChildrenGeometries(indicator::EntryLocationMap&) const;

protected:
  void DoRelayout() override;
  std::string GetName() const override { return "MenuLayout"; }

private:
  void OnEntryMouseOwnershipChanged(bool);
  void OnEntryActiveChanged(bool);
  void OnEntryShowNowChanged(bool);

  CompWindow* win_;
  CompPoint last_pointer_;
  glib::Source::UniquePtr pointer_tracker_;
  glib::Source::UniquePtr show_now_timeout_;
  std::shared_ptr<MenuDropdown> dropdown_;
};

} // decoration namespace
} // unity namespace

#endif // UNITY_DECORATIONS_MENU_LAYOUT
