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

#ifndef UNITYSHARED_BAMF_APPLICATION_MANAGER_H
#define UNITYSHARED_BAMF_APPLICATION_MANAGER_H

#include <libbamf/libbamf.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/GLibSignal.h>

#include "unity-shared/ApplicationManager.h"


namespace unity
{
namespace bamf
{
class Manager;
class View
{
public:
  View(Manager const& manager,
       glib::Object<BamfView> const& view);

  std::string title() const;
  std::string icon() const;
  std::string type() const;

  bool GetVisible() const;
  bool GetActive() const;
  bool GetRunning() const;
  bool GetUrgent() const;

protected:
  Manager const& manager_;
  glib::Object<BamfView> bamf_view_;
};


class WindowBase: public ::unity::ApplicationWindow, public View
{
protected:
  WindowBase(Manager const& manager,
             glib::Object<BamfView> const& window);

public:
  virtual std::string title() const;
  virtual std::string icon() const;
  virtual std::string type() const; // 'window' or 'tab'

  virtual bool Focus() const;

private: // Property getters and setters
  void HookUpEvents();

private:
  glib::SignalManager signals_;
};

// NOTE: Can't use Window as a type as there is a #define for Window to some integer value.
class AppWindow: public WindowBase
{
public:
  AppWindow(Manager const& manager,
            glib::Object<BamfView> const& window);

  virtual Window window_id() const;
  virtual int monitor() const;
  virtual ApplicationPtr application() const;
  virtual void Quit() const;

private:
  glib::Object<BamfWindow> bamf_window_;
};

class Tab: public WindowBase
{
public:
  Tab(Manager const& manager,
      glib::Object<BamfView> const& tab);

  virtual Window window_id() const;
  virtual int monitor() const;
  virtual ApplicationPtr application() const;
  virtual bool Focus() const;
  virtual void Quit() const;

private:
  glib::Object<BamfTab> bamf_tab_;
};


class Application : public ::unity::Application, public View
{
public:
  Application(Manager const& manager,
              glib::Object<BamfView> const& app);
  Application(Manager const& manager,
              glib::Object<BamfApplication> const& app);
  ~Application();

  virtual std::string title() const;
  virtual std::string icon() const;
  virtual std::string desktop_file() const;
  virtual std::string type() const;

  virtual WindowList GetWindows() const;
  virtual bool OwnsWindow(Window window_id) const;

  virtual std::vector<std::string> GetSupportedMimeTypes() const;
  virtual std::vector<ApplicationMenu> GetRemoteMenus() const;

  virtual ApplicationWindowPtr GetFocusableWindow() const;
  virtual void Focus(bool show_on_visible, int monitor) const;

  virtual void Quit() const;

private: // Property getters and setters
  void HookUpEvents();

  bool GetSeen() const;
  bool SetSeen(bool const& param);

  bool GetSticky() const;
  bool SetSticky(bool const& param);

private:
  glib::Object< ::BamfApplication> bamf_app_;
  glib::SignalManager signals_;
};

class Manager : public ::unity::ApplicationManager
{
public:
  Manager();
  ~Manager();

  virtual ApplicationWindowPtr GetActiveWindow() const;

  virtual ApplicationPtr GetApplicationForDesktopFile(std::string const& desktop_file) const;

  virtual ApplicationList GetRunningApplications() const;


  virtual ApplicationPtr GetApplicationForWindow(glib::Object<BamfWindow> const& window) const;

private:
  void OnViewOpened(BamfMatcher* matcher, BamfView* view);

private:
  glib::Object<BamfMatcher> matcher_;
  glib::SignalManager signals_;
};

} // namespace bamf
} // namespace unity

#endif // UNITYSHARED_APPLICATION_MANAGER_H
