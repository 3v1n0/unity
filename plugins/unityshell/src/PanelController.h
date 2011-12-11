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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef _PANEL_CONTROLLER_H_
#define _PANEL_CONTROLLER_H_

#include <list>
#include <memory>

#include <Nux/Nux.h>

namespace unity
{
namespace panel
{

class Controller
{
public:
  typedef std::shared_ptr<Controller> Ptr;

  Controller();
  ~Controller();

  void StartFirstMenuShow();
  void EndFirstMenuShow();
  void QueueRedraw();

  unsigned int GetTrayXid ();
  std::list<nux::Geometry> GetGeometries ();

  // NOTE: nux::Property maybe?
  void SetOpacity(float opacity);
  void SetOpacityMaximizedToggle(bool enabled);
  void SetMenuShowTimings(int fadein, int fadeout, int discovery, int discovery_fadein, int discovery_fadeout);

  float opacity() const;

private:
  class Impl;
  Impl* pimpl;
};

}
}

#endif // _PANEL_CONTROLLER_H_
