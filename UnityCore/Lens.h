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

#ifndef UNITY_LENS_H
#define UNITY_LENS_H

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <NuxCore/Property.h>
#include <sigc++/trackable.h>

namespace unity {
namespace dash {

class Lens : public sigc::trackable, boost::noncopyable
{
public:
  typedef boost::shared_ptr<Lens> Ptr;

  Lens(std::string const& id,
       std::string const& dbus_name,
       std::string const& dbus_path,
       std::string const& name,
       std::string const& icon,
       std::string const& description="",
       std::string const& search_hint="",
       bool visible=true,
       std::string const& shortcut="");

  ~Lens();

  nux::ROProperty<std::string> id;
  nux::ROProperty<std::string> dbus_name;
  nux::ROProperty<std::string> dbus_path;
  nux::ROProperty<std::string> name;
  nux::ROProperty<std::string> icon;
  nux::ROProperty<std::string> description;
  nux::ROProperty<std::string> search_hint;
  nux::ROProperty<bool> visible;
  nux::ROProperty<std::string> shortcut;

  class Impl;
private:
  Impl *pimpl;
};

}
}

#endif
