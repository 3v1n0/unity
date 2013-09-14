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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#ifndef UNITY_SHARED_APPLICATION_STARTER_H
#define UNITY_SHARED_APPLICATION_STARTER_H

#include <boost/noncopyable.hpp>
#include <memory>
#include <string>

#include <X11/X.h>

namespace unity {

class ApplicationStarter : boost::noncopyable
{
public:
  typedef std::shared_ptr<ApplicationStarter> Ptr;

  virtual ~ApplicationStarter() {}

  virtual bool Launch(std::string const& application_name, Time timestamp) = 0;
};

}

#endif
