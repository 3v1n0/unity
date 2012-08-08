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

#ifndef UNITYSHELL_VOLUME_H
#define UNITYSHELL_VOLUME_H

#include <boost/noncopyable.hpp>
#include <string>

namespace unity
{
namespace launcher
{

class Volume : boost::noncopyable
{
public:
  virtual ~Volume() {}

  virtual std::string GetName() const = 0;
  virtual std::string GetIconName() const = 0;
  virtual std::string GetIdentifer() const = 0;
  virtual bool IsMounted() const = 0;
  virtual void MountAndOpenInFileManager() = 0;
};

} // namespace launcher
} // namespace unity

#endif // UNITYSHELL_DEVICE_H