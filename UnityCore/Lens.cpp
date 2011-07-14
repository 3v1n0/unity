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

#include "Lens.h"

#include <gio/gio.h>
#include <glib.h>
#include <NuxCore/Logger.h>

#include "config.h"
#include "GLibWrapper.h"


namespace unity {
namespace dash {

namespace {
nux::logging::Logger logger("unity.dash.lens");
}

using namespace std;

class Lens::Impl
{
public:
  typedef map<GFile*, glib::Object<GCancellable>> CancellableMap;

  Impl(Lens* owner,
       string const& dbus_name,
       string const& dbus_path,
       string const& name,
       string const& icon,
       string const& description,
       string const& search_hint,
       bool visible,
       string const& shortcut);

  string const& name();

  ~Impl();

  Lens* owner_;

  std::string name_;
};

Lens::Impl::Impl(Lens* owner,
                 string const& dbus_name,
                 string const& dbus_path,
                 string const& name,
                 string const& icon,
                 string const& description,
                 string const& search_hint,
                 bool visible,
                 string const& shortcut)
  : owner_(owner),
    name_(name)
{}

Lens::Impl::~Impl()
{}

string const& Lens::Impl::name()
{
  return name_;
}


Lens::Lens(string const& dbus_name_,
           string const& dbus_path_,
           string const& name_,
           string const& icon_,
           string const& description_,
           string const& search_hint_,
           bool visible_,
           string const& shortcut_)

  : pimpl(new Impl(this,
                   dbus_name_,
                   dbus_path_,
                   name_,
                   icon_,
                   description_,
                   search_hint_,
                   visible_,
                   shortcut_))
{
  name.SetGetterFunction (sigc::mem_fun(pimpl, &Lens::Impl::name));
}

Lens::~Lens()
{
  delete pimpl;
}

}
}
