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

#include <memory>
#include <Nux/Nux.h>

#include "Introspectable.h"
namespace unity
{
namespace panel
{

class Controller : public sigc::trackable, public unity::debug::Introspectable
{
public:
  typedef std::shared_ptr<Controller> Ptr;

  Controller();
  ~Controller();

  void FirstMenuShow();
  void QueueRedraw();

  std::vector<Window> GetTrayXids() const;
  std::vector<nux::Geometry> GetGeometries() const;

  // NOTE: nux::Property maybe?
  void SetOpacity(float opacity);
  void SetOpacityMaximizedToggle(bool enabled);
  void SetMenuShowTimings(int fadein, int fadeout, int discovery, int discovery_fadein, int discovery_fadeout);

  float opacity() const;

  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);
private:
  void OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors);
  class Impl;
  Impl* pimpl;
};

}
}

#endif // _PANEL_CONTROLLER_H_
