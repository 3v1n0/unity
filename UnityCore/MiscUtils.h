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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#ifndef UNITY_MISC_UTILS_H
#define UNITY_MISC_UTILS_H

#include <NuxCore/Property.h>

namespace unity
{
namespace utils
{

template <typename TYPE>
sigc::connection ConnectProperties(nux::ROProperty<TYPE>& local_property, nux::Property<TYPE>& remote_property)
{
  local_property.SetGetterFunction([&remote_property]() { return remote_property.Get(); });
  auto change_connection = remote_property.changed.connect([&local_property, &remote_property](TYPE const& value) { local_property.EmitChanged(value); });
  return change_connection;
}

template <typename TYPE>
sigc::connection ConnectProperties(nux::ROProperty<TYPE>& local_property, nux::ROProperty<TYPE>& remote_property)
{
  local_property.SetGetterFunction([&remote_property]() { return remote_property.Get(); });
  auto change_connection = remote_property.changed.connect([&local_property, &remote_property](TYPE const& value) { local_property.EmitChanged(value); });
  return change_connection;
}

template <typename TYPE>
sigc::connection ConnectProperties(nux::RWProperty<TYPE>& local_property, nux::Property<TYPE>& remote_property)
{  
  auto change_connection = std::make_shared<sigc::connection>();
  *change_connection = remote_property.changed.connect([&local_property, &remote_property](TYPE const& value)
  {
    local_property.EmitChanged(value);
  });

  local_property.SetGetterFunction([&remote_property]() { return remote_property.Get(); });
  local_property.SetSetterFunction([&remote_property, &local_property, change_connection](TYPE const& value)
  {
    TYPE old_value = remote_property.Get();

    // block so we doing get a loop.
    bool blocked = change_connection->block(true);
    bool ret = remote_property.Set(value) != old_value;
    change_connection->block(blocked);
    return ret;
  });
  return *change_connection;
}

template <typename TYPE>
sigc::connection ConnectProperties(nux::RWProperty<TYPE>& local_property, nux::RWProperty<TYPE>& remote_property)
{  
  auto change_connection = std::make_shared<sigc::connection>();
  *change_connection = remote_property.changed.connect([&local_property, &remote_property](TYPE const& value)
  {
    local_property.EmitChanged(value);
  });

  local_property.SetGetterFunction([&remote_property]() { return remote_property.Get(); });
  local_property.SetSetterFunction([&remote_property, &local_property, change_connection](TYPE const& value)
  {
    TYPE old_value = remote_property.Get();

    // block so we doing get a loop.
    bool blocked = change_connection->block(true);
    bool ret = remote_property.Set(value) != old_value;
    change_connection->block(blocked);
    return ret;
  });
  return *change_connection;
}

template <class TYPE>
class AutoResettingVariable
{
public:
  AutoResettingVariable(TYPE*const variable, TYPE const& new_value)
  : variable_(variable)
  , original_value_(*variable)
  {
    *variable_ = new_value;
  }

  ~AutoResettingVariable()
  {
    *variable_ = original_value_;
  }

private:
  TYPE*const variable_;
  TYPE original_value_;
};

}
}

#endif // UNITY_MISC_UTILS_H