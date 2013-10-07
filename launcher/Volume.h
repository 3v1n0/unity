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

#include <memory>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>
#include <string>

namespace unity
{
namespace launcher
{

class Volume : public sigc::trackable
{
public:
  typedef std::shared_ptr<Volume> Ptr;

  Volume() = default;
  virtual ~Volume() = default;

  virtual bool CanBeEjected() const = 0;
  virtual bool CanBeRemoved() const = 0;
  virtual bool CanBeStopped() const = 0;
  virtual std::string GetName() const = 0;
  virtual std::string GetIconName() const = 0;
  virtual std::string GetIdentifier() const = 0;
  virtual std::string GetUri() const = 0;
  virtual bool HasSiblings() const = 0;
  virtual bool IsMounted() const = 0;
  virtual bool IsOpened() const = 0;

  virtual void Eject() = 0;
  virtual void Mount() = 0;
  virtual void StopDrive() = 0;
  virtual void Unmount() = 0;

  sigc::signal<void> changed;
  sigc::signal<void> removed;
  sigc::signal<void> mounted;
  sigc::signal<void> unmounted;
  sigc::signal<void> ejected;
  sigc::signal<void> stopped;
  sigc::signal<void, bool> opened;

private:
  Volume(Volume const&) = delete;
  Volume& operator=(Volume const&) = delete;
};

}
}

#endif
