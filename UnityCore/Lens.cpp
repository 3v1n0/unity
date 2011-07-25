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


namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.lens");
}

using std::string;

class Lens::Impl
{
public:
  Impl(Lens* owner,
       string const& id,
       string const& dbus_name,
       string const& dbus_path,
       string const& name,
       string const& icon,
       string const& description,
       string const& search_hint,
       bool visible,
       string const& shortcut);

  string const& id();
  string const& dbus_name();
  string const& dbus_path();
  string const& name();
  string const& icon();
  string const& description();
  string const& search_hint();
  bool visible();
  string const& shortcut();


  ~Impl();

  Lens* owner_;

  string id_;
  string dbus_name_;
  string dbus_path_;
  string name_;
  string icon_;
  string description_;
  string search_hint_;
  bool visible_;
  string shortcut_;
};

Lens::Impl::Impl(Lens* owner,
                 string const& id,
                 string const& dbus_name,
                 string const& dbus_path,
                 string const& name,
                 string const& icon,
                 string const& description,
                 string const& search_hint,
                 bool visible,
                 string const& shortcut)
  : owner_(owner)
  , id_(id)
  , dbus_name_(dbus_name)
  , dbus_path_(dbus_path)
  , name_(name)
  , icon_(icon)
  , description_(description)
  , search_hint_(search_hint)
  , visible_(visible)
  , shortcut_(shortcut)
{}

Lens::Impl::~Impl()
{}

string const& Lens::Impl::id()
{
  return id_;
}

string const& Lens::Impl::dbus_name()
{
  return dbus_name_;
}

string const& Lens::Impl::dbus_path()
{
  return dbus_path_;
}

string const& Lens::Impl::name()
{
  return name_;
}

string const& Lens::Impl::icon()
{
  return icon_;
}

string const& Lens::Impl::description()
{
  return description_;
}

string const& Lens::Impl::search_hint()
{
  return search_hint_;
}

bool Lens::Impl::visible()
{
  return visible_;
}

string const& Lens::Impl::shortcut()
{
  return shortcut_;
}

Lens::Lens(string const& id_,
           string const& dbus_name_,
           string const& dbus_path_,
           string const& name_,
           string const& icon_,
           string const& description_,
           string const& search_hint_,
           bool visible_,
           string const& shortcut_)

  : pimpl(new Impl(this,
                   id_,
                   dbus_name_,
                   dbus_path_,
                   name_,
                   icon_,
                   description_,
                   search_hint_,
                   visible_,
                   shortcut_))
{
  id.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::id));
  dbus_name.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::dbus_name));
  dbus_path.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::dbus_path));
  name.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::name));
  icon.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::icon));
  description.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::description));
  search_hint.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::search_hint));
  visible.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::visible));
  shortcut.SetGetterFunction(sigc::mem_fun(pimpl, &Lens::Impl::shortcut));
}

Lens::~Lens()
{
  delete pimpl;
}

}
}
