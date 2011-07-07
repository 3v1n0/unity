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

}
}

