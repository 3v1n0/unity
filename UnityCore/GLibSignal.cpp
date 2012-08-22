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

#include "GLibSignal.h"

namespace unity
{
namespace glib
{

SignalBase::SignalBase()
  : object_(nullptr),
    connection_id_(0)
{}

SignalBase::~SignalBase()
{
  Disconnect();
}

void SignalBase::Disconnect()
{
  if (connection_id_ && G_IS_OBJECT(object_))
  {
    g_signal_handler_disconnect(object_, connection_id_);
    g_object_remove_weak_pointer(object_, reinterpret_cast<gpointer*>(&object_));
  }

  object_ = nullptr;
  connection_id_ = 0;
}

GObject* SignalBase::object() const
{
  return object_;
}

std::string const& SignalBase::name() const
{
  return name_;
}


SignalManager::SignalManager()
{}

SignalManager::~SignalManager()
{
  for (auto const& signal : connections_)
  {
    if (G_IS_OBJECT(signal->object()))
      g_object_weak_unref(signal->object(), (GWeakNotify)&OnObjectDestroyed, this);
  }
}

// Ideally this would be SignalBase& but there is a specific requirment to allow
// only one instance of Signal to control a connection. With the templating, it
// was too messy to try and write a copy constructor/operator that would steal
// from "other" and make the new one the owner. Not only did it create
// opportunity for random bugs, it also made the API bad.
void SignalManager::Add(SignalBase* signal)
{
  Add(SignalBase::Ptr(signal));
}

void SignalManager::Add(SignalBase::Ptr const& signal)
{
  connections_.push_back(signal);
  g_object_weak_ref(signal->object(), (GWeakNotify)&OnObjectDestroyed, this);
}

void SignalManager::OnObjectDestroyed(SignalManager* self, GObject* old_obj)
{
  for (auto it = self->connections_.begin(); it != self->connections_.end();)
  {
    auto const& signal = *it;

    // When an object has been destroyed, the signal member is nullified,
    // so at this point we can be sure that removing signal with a null object,
    // means removing invalid signals.
    if (!signal->object())
    {
      it = self->connections_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

// This uses void* to keep in line with the g_signal* functions
// (it allows you to pass in a GObject without casting up).
void SignalManager::Disconnect(void* object, std::string const& signal_name)
{
  bool all_signals = signal_name.empty();

  for (auto it = connections_.begin(); it != connections_.end();)
  {
    auto const& signal = *it;

    if (signal->object() == object && (all_signals || signal->name() == signal_name))
    {
      g_object_weak_unref(signal->object(), (GWeakNotify)&OnObjectDestroyed, this);
      it = connections_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

}
}

