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
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 */

#ifndef UNITYSHARED_APPLICATION_MANAGER_H
#define UNITYSHARED_APPLICATION_MANAGER_H

#include <memory>
#include <vector>

#include <sigc++/signal.h>
#include <NuxCore/Property.h>
#include <unity-shared/WindowManager.h>


namespace unity
{

class Application;
class ApplicationManager;
class ApplicationWindow;
typedef std::shared_ptr<Application> ApplicationPtr;
typedef std::shared_ptr<ApplicationWindow> ApplicationWindowPtr;

typedef std::vector<ApplicationPtr> ApplicationList;
typedef std::vector<ApplicationWindowPtr> WindowList;


class ApplicationWindow
{
public:
  virtual ~ApplicationWindow() {}

  virtual std::string title() const = 0;
  virtual std::string icon() const = 0;

  virtual Window window_id() const = 0;
  virtual int monitor() const = 0;

  virtual ApplicationPtr application() const = 0;
};


class Application
{
public:
  virtual ~Application() {}

  virtual std::string icon() const = 0;
  virtual std::string title() const = 0;

  virtual WindowList get_windows() const = 0;

  // Considering using a property for the "unity-seen" quark
  nux::RWProperty<bool> seen;
  nux::RWProperty<bool> sticky;

  nux::ROProperty<bool> visible;
  nux::ROProperty<bool> active;
  nux::ROProperty<bool> running;
  nux::ROProperty<bool> urgent;

  sigc::signal<void> closed;

  sigc::signal<void, ApplicationWindow const&> window_opened;
  sigc::signal<void, ApplicationWindow const&> window_moved;
  sigc::signal<void> window_closed;
};


class ApplicationManager
{
public:
  virtual ~ApplicationManager() {}

  static ApplicationManager& Default();

  virtual ApplicationWindowPtr GetActiveWindow() const = 0;

  virtual ApplicationPtr active_application() const = 0;

  virtual ApplicationPtr GetApplicationForDesktopFile(std::string const& desktop_file) const = 0;

  virtual ApplicationList running_applications() const = 0;

  sigc::signal<void, ApplicationPtr const&> application_started;
};

}

#endif // UNITYSHARED_APPLICATION_MANAGER_H
