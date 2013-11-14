// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2012 Canonical Ltd
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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#ifndef UNITYSHELL_XDND_MANAGER_H
#define UNITYSHELL_XDND_MANAGER_H

#include <boost/noncopyable.hpp>
#include <memory>
#include <sigc++/signal.h>
#include <string>

namespace unity {

class XdndManager : boost::noncopyable {
public:
  typedef std::shared_ptr<XdndManager> Ptr;

  virtual ~XdndManager() {}

  virtual int Monitor() const = 0;

  sigc::signal<void, std::string const&, int> dnd_started;
  sigc::signal<void, std::string const&, int, int> monitor_changed;
  sigc::signal<void> dnd_finished;
};

}

#endif
