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

#ifndef UNITYSHELL_XDND_COLLECTION_WINDOW_H
#define UNITYSHELL_XDND_COLLECTION_WINDOW_H

#include <boost/noncopyable.hpp>
#include <memory>
#include <sigc++/signal.h>
#include <string>
#include <vector>

namespace unity {

/**
 * XdndCollectionWindow makes it possible to collect drag and drop data as
 * soon as dnd starts and not when the mouse pointer enter the x window.
 **/

class XdndCollectionWindow : boost::noncopyable
{
public:
  typedef std::shared_ptr<XdndCollectionWindow> Ptr;

  virtual ~XdndCollectionWindow() {}

  virtual void Collect() = 0;
  virtual void Deactivate() = 0;

  sigc::signal<void, std::vector<std::string>> collected;
};

}

#endif
