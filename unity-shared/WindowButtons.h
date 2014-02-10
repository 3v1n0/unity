// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#ifndef WINDOW_BUTTONS_H
#define WINDOW_BUTTONS_H

#include <Nux/HLayout.h>
#include <Nux/Button.h>

#include "unity-shared/Introspectable.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UnitySettings.h"

namespace unity
{
class WindowButtons : public nux::HLayout, public debug::Introspectable
{
  // These are the [close][minimize][restore] buttons on the panel when there
  // is a maximized window

public:
  WindowButtons();

  nux::Property<int> monitor;
  nux::Property<Window> controlled_window;
  nux::Property<double> opacity;
  nux::Property<bool> focused;

  bool IsMouseOwner();
  virtual nux::Area* FindAreaUnderMouse(const nux::Point& mouse_pos, nux::NuxEventType event_type);

  sigc::signal<void> close_clicked;
  sigc::signal<void> minimize_clicked;
  sigc::signal<void> restore_clicked;
  sigc::signal<void> maximize_clicked;
  sigc::signal<void, int, int, int, int, unsigned long, unsigned long> mouse_move;
  sigc::signal<void, int, int, unsigned long, unsigned long> mouse_enter;
  sigc::signal<void, int, int, unsigned long, unsigned long> mouse_leave;

protected:
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

private:
  void OnCloseClicked(nux::Button *button);
  void OnMinimizeClicked(nux::Button *button);
  void OnRestoreClicked(nux::Button *button);
  void OnMaximizeClicked(nux::Button *button);
  void OnOverlayShown(GVariant* data);
  void OnOverlayHidden(GVariant* data);
  void OnDashSettingsUpdated(FormFactor form_factor);
  void OnControlledWindowChanged(Window xid);
  bool OpacitySetter(double& target, double new_value);
  void OnMonitorChanged(int monitor);


  std::string active_overlay_;
  UBusManager ubus_manager_;
};
}

#endif
