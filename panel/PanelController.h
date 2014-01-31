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

#include "launcher/EdgeBarrierController.h"
#include "unity-shared/GnomeKeyGrabber.h"
#include "unity-shared/Introspectable.h"

namespace unity
{

class PanelView;

namespace panel
{

class Controller : public sigc::trackable, public unity::debug::Introspectable
{
public:
  typedef std::shared_ptr<Controller> Ptr;
  typedef std::vector<nux::ObjectPtr<PanelView>> PanelVector;

  Controller(ui::EdgeBarrierController::Ptr const& barrier_controller, GnomeKeyGrabber::Ptr const& grabber);
  Controller(ui::EdgeBarrierController::Ptr const& barrier_controller);
  ~Controller();

  void ShowMenuBar();
  void HideMenuBar();
  void FirstMenuShow();
  void QueueRedraw();

  std::vector<Window> const& GetTrayXids() const;
  PanelVector& panels() const;
  std::vector<nux::Geometry> const& GetGeometries() const;

  nux::Property<int> launcher_width;

  // NOTE: nux::Property maybe?
  void SetOpacity(float opacity);
  void SetOpacityMaximizedToggle(bool enabled);
  void SetMenuShowTimings(int fadein, int fadeout, int discovery, int discovery_fadein, int discovery_fadeout);

  float opacity() const;

  bool IsMouseInsideIndicator(nux::Point const& mouse_position) const;

protected:
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

private:
  void OnScreenChanged(int primary_monitor, std::vector<nux::Geometry>& monitors);

  class Impl;
  Impl* pimpl;
};

}
}

#endif // _PANEL_CONTROLLER_H_
