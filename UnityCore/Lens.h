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

  nux::ROProperty<std::string> dbus_name;
  nux::ROProperty<std::string> dbus_path;
};

}
}

#endif
