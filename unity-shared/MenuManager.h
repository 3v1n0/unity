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

#ifndef __UNITY_INDICATOR_MANAGER__
#define __UNITY_INDICATOR_MANAGER__

#include <NuxCore/Property.h>
#include <UnityCore/Indicators.h>
#include "KeyGrabber.h"

namespace unity
{
namespace menu
{

struct any_true
{
  typedef bool result_type;
  template <class IT> result_type operator()(IT it, IT end)
  {
    for (; it != end; ++it) if (*it) return true;
    return false;
  }
};

class Manager : public sigc::trackable
{
public:
  typedef std::shared_ptr<Manager> Ptr;

  nux::Property<bool> show_menus;
  nux::Property<unsigned> show_menus_wait;

  nux::Property<unsigned> fadein;
  nux::Property<unsigned> fadeout;
  nux::Property<unsigned> discovery;
  nux::Property<unsigned> discovery_fadein;
  nux::Property<unsigned> discovery_fadeout;

  Manager(indicator::Indicators::Ptr const&, key::Grabber::Ptr const&);
  virtual ~Manager();

  bool HasAppMenu() const;
  indicator::Indicators::Ptr const& Indicators() const;
  indicator::Indicator::Ptr const& AppMenu() const;

  key::Grabber::Ptr const& KeyGrabber() const;

  sigc::signal<void> appmenu_added;
  sigc::signal<void> appmenu_removed;
  sigc::signal<bool>::accumulated<any_true> open_first;
  sigc::signal<bool, std::string const&>::accumulated<any_true> key_activate_entry;

private:
  Manager(Manager const&) = delete;
  Manager& operator=(Manager const&) = delete;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // menu namespace
} // unity namespace

#endif // __UNITY_INDICATOR_MANAGER__
