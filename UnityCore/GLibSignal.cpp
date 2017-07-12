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

bool SignalBase::Disconnect()
{
  bool disconnected = false;

  if (connection_id_ && G_IS_OBJECT(object_))
  {
    g_signal_handler_disconnect(object_, connection_id_);
    g_object_remove_weak_pointer(object_, reinterpret_cast<gpointer*>(&object_));
    disconnected = true;
  }

  object_ = nullptr;
  connection_id_ = 0;
  return disconnected;
}

bool SignalBase::Block() const
{
  bool blocked = false;

  if (connection_id_ && G_IS_OBJECT(object_))
  {
    g_signal_handler_block(object_, connection_id_);
    blocked = true;
  }

  return blocked;
}

bool SignalBase::Unblock() const
{
  bool unblocked = false;

  if (connection_id_ && G_IS_OBJECT(object_))
  {
    g_signal_handler_unblock(object_, connection_id_);
    unblocked = true;
  }

  return unblocked;
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
SignalBase::Ptr SignalManager::Add(SignalBase* signal)
{
  return Add(SignalBase::Ptr(signal));
}

SignalBase::Ptr SignalManager::Add(SignalBase::Ptr const& signal)
{
  connections_.push_back(signal);
  g_object_weak_ref(signal->object(), (GWeakNotify)&OnObjectDestroyed, this);
  return signal;
}

// This uses void* to keep in line with the g_signal* functions
// (it allows you to pass in a GObject without casting up).
bool SignalManager::ForeachMatchedSignal(void* object, std::string const& signal_name, std::function<void(SignalBase::Ptr const&)> action, bool erase_after)
{
  bool action_performed = false;
  bool all_signals = signal_name.empty();

  for (auto it = connections_.begin(); it != connections_.end();)
  {
    auto const& signal = *it;

    if (signal->object() == object && (all_signals || signal->name() == signal_name))
    {
      if (action)
      {
        action_performed = true;
        action(signal);
      }

      it = erase_after ? connections_.erase(it) : ++it;
    }
    else
    {
      ++it;
    }
  }

  return action_performed;
}

void SignalManager::OnObjectDestroyed(SignalManager* self, GObject* old_obj)
{
  self->ForeachMatchedSignal(nullptr, "", nullptr, /*erase_after*/ true);
}

bool SignalManager::Block(void* object, std::string const& signal_name)
{
  return ForeachMatchedSignal(object, signal_name, [this] (SignalBase::Ptr const& signal) {
    signal->Block();
  });
}

bool SignalManager::Unblock(void* object, std::string const& signal_name)
{
  return ForeachMatchedSignal(object, signal_name, [this] (SignalBase::Ptr const& signal) {
    signal->Unblock();
  });
}

bool SignalManager::Disconnect(void* object, std::string const& signal_name)
{
  return ForeachMatchedSignal(object, signal_name, [this] (SignalBase::Ptr const& signal) {
    g_object_weak_unref(signal->object(), (GWeakNotify)&OnObjectDestroyed, this);
  }, true);
}

}
}

