// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2013 Canonical Ltd
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */


#ifndef UNITY_SCOPE_DATA_H
#define UNITY_SCOPE_DATA_H

#include <NuxCore/Property.h>
#include "GLibWrapper.h"

namespace unity
{
namespace dash
{

class ScopeData
{
public:
  typedef std::shared_ptr<ScopeData> Ptr;
  ScopeData();

  nux::Property<std::string> id;
  nux::Property<std::string> full_path;
  nux::Property<std::string> dbus_name;
  nux::Property<std::string> dbus_path;
  nux::Property<bool> is_master;
  nux::Property<std::string> icon_hint;
  nux::Property<std::string> category_icon_hint;
  nux::Property<std::vector<std::string>> keywords;
  nux::Property<std::string> type;
  nux::Property<std::string> query_pattern;
  nux::Property<std::string> name;
  nux::Property<std::string> description;
  nux::Property<std::string> shortcut;
  nux::Property<std::string> search_hint;
  
  nux::Property<bool> visible; // FIXME!

  static ScopeData::Ptr ReadProtocolDataForId(std::string const& scope_id, glib::Error& error);
};

}
}

#endif 