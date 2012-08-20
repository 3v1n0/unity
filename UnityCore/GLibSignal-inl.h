// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2011-2012 Canonical Ltd
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
* Authored by: Neil Jagdish patel <neil.patel@canonical.com>
*              Marco Trevisan <marco.trevisan@canonical.com>
*/

#ifndef UNITY_GLIB_SIGNAL_INL_H
#define UNITY_GLIB_SIGNAL_INL_H

namespace unity
{
namespace glib
{

template <typename R, typename G, typename... Ts>
Signal<R, G, Ts...>::Signal(G object, std::string const& signal_name,
                            SignalCallback const& callback)
{
  Connect(object, signal_name, callback);
}

template <typename R, typename G, typename... Ts>
void Signal<R, G, Ts...>::Connect(G object, std::string const& signal_name,
                                  SignalCallback const& callback)
{
  if (!callback || !G_IS_OBJECT(object) || signal_name.empty())
    return;

  Disconnect();

  object_ = reinterpret_cast<GObject*>(object);
  name_ = signal_name;
  callback_ = callback;
  connection_id_ = g_signal_connect(object_, signal_name.c_str(), G_CALLBACK(Callback), this);
  g_object_add_weak_pointer(object_, reinterpret_cast<gpointer*>(&object_));
}

template <typename R, typename G, typename... Ts>
R Signal<R, G, Ts...>::Callback(G object, Ts... vs, Signal* self)
{
  return self->callback_(object, vs...);
}

}
}

#endif
