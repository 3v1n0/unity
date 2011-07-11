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
* Authored by: Neil Jagdish patel <neil.patel@canonical.com>
*/

#include "GLibSignal.h"

namespace unity {
namespace glib {

SignalBase::SignalBase()
  : object_(0),
    connection_id_(0)
{}

SignalBase::~SignalBase()
{
  Disconnect();
}

void SignalBase::Disconnect()
{
  if (G_IS_OBJECT(object_) && connection_id_)
    g_signal_handler_disconnect(object_, connection_id_);
  
  object_ = 0;
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
{}

// Ideally this would be SignalBase& but there is a specific requirment to allow
// only one instance of Signal to control a connection. With the templating, it
// was too messy to try and write a copy constructor/operator that would steal
// from "other" and make the new one the owner. Not only did it create
// opportunity for random bugs, it also made the API bad.
void SignalManager::Add(SignalBase* signal)
{
  SignalBase::Ptr s(signal);
  connections_.push_back(s);
}

// This uses void* to keep in line with the g_signal* functions
// (it allows you to pass in a GObject without casting up).
void SignalManager::Disconnect(void* object, std::string const& signal_name)
{
  for (ConnectionVector::iterator it = connections_.begin();
       it != connections_.end();
       ++it)
  {
    if ((*it)->object() == object
        && (*it)->name() == signal_name)
    {
      (*it)->Disconnect();
      connections_.erase(it, it);
    }
  }
}

}
}

