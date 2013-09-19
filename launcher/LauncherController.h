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

#include "EdgeBarrierController.h"
#include "LauncherOptions.h"
#include "SoftwareCenterLauncherIcon.h"
#include "XdndManager.h"

namespace unity
{
namespace launcher
{

class AbstractLauncherIcon;
class Launcher;
class LauncherModel;
class TestLauncherController;

class Controller : public unity::debug::Introspectable, public sigc::trackable
{
public:
  typedef std::shared_ptr<Controller> Ptr;
  typedef std::vector<nux::ObjectPtr<Launcher> > LauncherList;

  nux::Property<Options::Ptr> options;
  nux::Property<bool> multiple_launchers;

  Controller(XdndManager::Ptr const& xdnd_manager, ui::EdgeBarrierController::Ptr const& edge_barriers);
  ~Controller();

  Launcher& launcher() const;
  LauncherList& launchers() const;
  Window LauncherWindowId(int launcher) const;
  Window KeyNavLauncherInputWindowId() const;

  void UpdateNumWorkspaces(int workspaces);
  std::vector<char> GetAllShortcuts() const;
  std::vector<AbstractLauncherIcon::Ptr> GetAltTabIcons(bool current, bool show_deskop_disabled) const;

  void PushToFront();

  bool AboutToShowDash(int was_tap, int when) const;

  void HandleLauncherKeyPress(int when);
  void HandleLauncherKeyRelease(bool was_tap, int when);
  bool HandleLauncherKeyEvent(Display *display,
                              unsigned int key_sym,
                              unsigned long key_code,
                              unsigned long key_state,
                              char* key_string,
                              Time timestamp);

  void KeyNavActivate();
  void KeyNavGrab();
  void KeyNavTerminate(bool activate = true);
  void KeyNavNext();
  void KeyNavPrevious();
  bool KeyNavIsActive() const;

  bool IsOverlayOpen() const;

protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

private:
  friend class TestLauncherController;
  class Impl;
  std::unique_ptr<Impl> pimpl;
};

}
}

#endif // LAUNCHERCONTROLLER_H
