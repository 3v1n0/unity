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
#include <UnityCore/GLibWrapper.h>
#include <unity-shared/WindowManager.h>

struct _GdkPixbuf;

namespace unity
{

class Application;
class ApplicationManager;
class ApplicationWindow;
class ApplicationSubject;
typedef std::shared_ptr<Application> ApplicationPtr;
typedef std::shared_ptr<ApplicationWindow> ApplicationWindowPtr;
typedef std::shared_ptr<ApplicationSubject> ApplicationSubjectPtr;

typedef std::vector<ApplicationPtr> ApplicationList;
typedef std::vector<ApplicationWindowPtr> WindowList;

enum class ApplicationEventType
{
  CREATE,
  DELETE,
  ACCESS,
  LEAVE
};

enum class AppType
{
  NORMAL,
  WEBAPP,
  MOCK,
  UNKNOWN
};

enum class WindowType
{
  NORMAL,
  DESKTOP,
  DOCK,
  DIALOG,
  TOOLBAR,
  MENU,
  UTILITY,
  SPLASHSCREEN,
  TAB,
  MOCK,
  UNKNOWN
};

class ApplicationWindow
{
public:
  virtual ~ApplicationWindow() = default;

  virtual WindowType type() const = 0;
  virtual Window window_id() const = 0;

  // It is possible for this to be null, especially in situations where
  // the application is starting up or shutting down.
  virtual ApplicationPtr application() const = 0;
  // Returns true if we made a best effort at focusing the window, or
  // false if this was not possible for some reason (like a missing window_id).
  virtual bool Focus() const = 0;
  // Closes the window, or the browser tab if a webapp.
  virtual void Quit() const = 0;

  virtual bool operator==(ApplicationWindow const& other) const
  {
    return (window_id() == other.window_id());
  }

  virtual bool operator!=(ApplicationWindow const& other) const
  {
    return !(operator==(other));
  }

  nux::ROProperty<int> monitor;

  nux::ROProperty<std::string> title;
  nux::ROProperty<std::string> icon;

  nux::ROProperty<bool> visible;
  nux::ROProperty<bool> active;
  nux::ROProperty<bool> urgent;
  nux::ROProperty<bool> maximized;

  sigc::signal<void> closed;
};


class Application
{
public:
  virtual ~Application() = default;

  virtual AppType type() const = 0;

  // A string representation of the object.
  virtual std::string repr() const = 0;

  virtual WindowList const& GetWindows() const = 0;
  virtual bool OwnsWindow(Window window_id) const = 0;

  virtual std::vector<std::string> GetSupportedMimeTypes() const = 0;

  virtual ApplicationWindowPtr GetFocusableWindow() const = 0;
  virtual void Focus(bool show_on_visible, int monitor) const = 0;
  // Calls quit on all the Windows for this application.
  virtual void Quit() const = 0;

  virtual bool CreateLocalDesktopFile() const = 0;

  virtual void LogEvent(ApplicationEventType, ApplicationSubjectPtr const&) const = 0;

  virtual std::string desktop_id() const = 0;

  virtual bool operator==(Application const& other) const
  {
    return (!desktop_id().empty() && (desktop_id() == other.desktop_id()));
  }

  virtual bool operator!=(Application const& other) const
  {
    return !(operator==(other));
  }

  nux::ROProperty<std::string> desktop_file;
  nux::ROProperty<std::string> title;
  nux::ROProperty<std::string> icon;
  nux::ROProperty<glib::Object<_GdkPixbuf>> icon_pixbuf;

  // Considering using a property for the "unity-seen" quark
  nux::RWProperty<bool> seen;
  nux::RWProperty<bool> sticky;

  nux::ROProperty<bool> visible;
  nux::ROProperty<bool> active;
  nux::ROProperty<bool> running;
  nux::ROProperty<bool> urgent;
  nux::ROProperty<bool> starting;

  sigc::signal<void> closed;

  sigc::signal<void, ApplicationWindowPtr const&> window_opened;
  sigc::signal<void, ApplicationWindowPtr const&> window_moved;
  sigc::signal<void, ApplicationWindowPtr const&> window_closed;
};


class ApplicationSubject
{
public:
  virtual ~ApplicationSubject() = default;

  nux::RWProperty<std::string> uri;
  nux::RWProperty<std::string> origin;
  nux::RWProperty<std::string> text;
  nux::RWProperty<std::string> storage;
  nux::RWProperty<std::string> current_uri;
  nux::RWProperty<std::string> current_origin;
  nux::RWProperty<std::string> mimetype;
  nux::RWProperty<std::string> interpretation;
  nux::RWProperty<std::string> manifestation;

  bool operator==(ApplicationSubject const& other) const
  {
    return uri() == other.uri() &&
           origin() == other.origin() &&
           text() == other.text() &&
           storage() == other.storage() &&
           current_uri() == other.current_uri() &&
           current_origin() == other.current_origin() &&
           mimetype() == other.mimetype() &&
           interpretation() == other.interpretation() &&
           manifestation() == other.manifestation();
  }

  bool operator!=(ApplicationSubject const& other) const
  {
    return !(operator==(other));
  }
};


class ApplicationManager
{
public:
  virtual ~ApplicationManager() = default;

  static ApplicationManager& Default();

  virtual ApplicationPtr GetUnityApplication() const = 0;
  virtual ApplicationPtr GetActiveApplication() const = 0;
  virtual ApplicationWindowPtr GetActiveWindow() const = 0;
  virtual ApplicationPtr GetApplicationForDesktopFile(std::string const& desktop_file) const = 0;
  virtual ApplicationList GetRunningApplications() const = 0;
  virtual WindowList GetWindowsForMonitor(int monitor = -1) const = 0;
  virtual ApplicationPtr GetApplicationForWindow(Window xid) const = 0;
  virtual ApplicationWindowPtr GetWindowForId(Window xid) const = 0;
  virtual void FocusWindowGroup(WindowList const&, bool show_on_visible, int monitor) const = 0;

  sigc::signal<void, ApplicationPtr const&> application_started;
  sigc::signal<void, ApplicationPtr const&> application_stopped;
  sigc::signal<void, ApplicationPtr const&> active_application_changed;
  sigc::signal<void, ApplicationWindowPtr const&> window_opened;
  sigc::signal<void, ApplicationWindowPtr const&> window_closed;
  sigc::signal<void, ApplicationWindowPtr const&> active_window_changed;
};


bool operator==(ApplicationPtr const&, ApplicationPtr const&);
bool operator!=(ApplicationPtr const&, ApplicationPtr const&);
bool operator==(ApplicationWindowPtr const&, ApplicationWindowPtr const&);
bool operator!=(ApplicationWindowPtr const&, ApplicationWindowPtr const&);

}

#endif // UNITYSHARED_APPLICATION_MANAGER_H
