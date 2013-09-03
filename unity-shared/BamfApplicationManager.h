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

#include "unity-shared/DesktopApplicationManager.h"


namespace unity
{
namespace bamf
{
class View
{
public:
  View(ApplicationManager const& manager,
       glib::Object<BamfView> const& view);

  std::string GetTitle() const;
  std::string GetIcon() const;
  std::string type() const;

  bool GetVisible() const;
  bool GetActive() const;
  bool GetRunning() const;
  bool GetUrgent() const;

protected:
  ApplicationManager const& manager_;
  glib::Object<BamfView> bamf_view_;
};


class WindowBase: public ::unity::ApplicationWindow, public View
{
protected:
  WindowBase(ApplicationManager const& manager,
             glib::Object<BamfView> const& window);

public:
  virtual std::string type() const; // 'window' or 'tab'

  virtual bool Focus() const;

private:
  glib::SignalManager signals_;
};

// NOTE: Can't use Window as a type as there is a #define for Window to some integer value.
class AppWindow: public WindowBase
{
public:
  AppWindow(ApplicationManager const& manager,
            glib::Object<BamfWindow> const& window);
  AppWindow(ApplicationManager const& manager,
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
  Tab(ApplicationManager const& manager,
      glib::Object<BamfTab> const& tab);
  Tab(ApplicationManager const& manager,
      glib::Object<BamfView> const& tab);

  virtual Window window_id() const;
  virtual int monitor() const;
  virtual ApplicationPtr application() const;
  virtual bool Focus() const;
  virtual void Quit() const;

private:
  glib::Object<BamfTab> bamf_tab_;
};


class Application : public ::unity::desktop::Application, public View
{
public:
  Application(ApplicationManager const& manager,
              glib::Object<BamfApplication> const& app);
  Application(ApplicationManager const& manager,
              glib::Object<BamfView> const& app);

  virtual std::string type() const;

  virtual WindowList GetWindows() const;
  virtual bool OwnsWindow(Window window_id) const;

  virtual std::vector<std::string> GetSupportedMimeTypes() const;

  virtual ApplicationWindowPtr GetFocusableWindow() const;
  virtual void Focus(bool show_on_visible, int monitor) const;

  virtual void Quit() const;

  virtual bool CreateLocalDesktopFile() const;

  virtual std::string repr() const;

private: // Property getters and setters
  void HookUpEvents();

  std::string GetDesktopFile() const;

  bool GetSeen() const;
  bool SetSeen(bool const& param);

  bool GetSticky() const;
  bool SetSticky(bool const& param);

private:
  glib::Object<::BamfApplication> bamf_app_;
  glib::SignalManager signals_;
  std::string type_;
};

class Manager : public ::unity::ApplicationManager
{
public:
  Manager();
  ~Manager();

  ApplicationWindowPtr GetActiveWindow() override;

  ApplicationPtr GetApplicationForDesktopFile(std::string const& desktop_file) override;

  ApplicationList GetRunningApplications() override;

  ApplicationPtr GetApplicationForWindow(Window xid) override;

private:
  void OnViewOpened(BamfMatcher* matcher, BamfView* view);

private:
  glib::Object<BamfMatcher> matcher_;
  glib::SignalManager signals_;
};

} // namespace bamf
} // namespace unity

#endif // UNITYSHARED_APPLICATION_MANAGER_H
