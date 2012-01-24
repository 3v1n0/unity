// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Tim Penhey <tim.penhey@canonical.com>
 */

#ifndef LAUNCHERCONTROLLER_H
#define LAUNCHERCONTROLLER_H

#include <memory>
#include <vector>
#include <sigc++/sigc++.h>
#include <core/core.h>

#include "SoftwareCenterLauncherIcon.h"

namespace unity
{
namespace launcher
{
class AbstractLauncherIcon;
class Launcher;
class LauncherModel;

class Controller : public sigc::trackable
{
public:
  typedef std::shared_ptr<Controller> Ptr;
  typedef std::vector<nux::ObjectPtr<Launcher> > LauncherList;

  nux::Property<Options::Ptr> options;

  Controller(Display* display);
  ~Controller();

  Launcher& launcher();
  LauncherList& launchers();
  Window launcher_input_window_id();

  void UpdateNumWorkspaces(int workspaces);
  std::vector<char> GetAllShortcuts();
  std::vector<AbstractLauncherIcon*> GetAltTabIcons();

  void PushToFront();

  void SetShowDesktopIcon(bool show_desktop_icon);

private:
  class Impl;
  Impl* pimpl;
};

}
}

#endif // LAUNCHERCONTROLLER_H
