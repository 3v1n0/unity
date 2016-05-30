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

#include <unordered_map>
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

  bool GetVisible() const;
  bool GetActive() const;
  bool GetRunning() const;
  bool GetUrgent() const;
  bool GetStarting() const;

protected:
  ApplicationManager const& manager_;
  glib::Object<BamfView> bamf_view_;
  glib::SignalManager signals_;
};


class WindowBase: public ::unity::ApplicationWindow, public View
{
protected:
  WindowBase(ApplicationManager const& manager,
             glib::Object<BamfView> const& window);

public:
  bool Focus() const override;

  bool operator==(unity::ApplicationWindow const& other) const override
  {
    return static_cast<WindowBase const*>(this)->bamf_view_ == static_cast<WindowBase const&>(other).bamf_view_;
  }
  bool operator!=(unity::ApplicationWindow const& other) const override { return !(operator==(other)); }
};

// NOTE: Can't use Window as a type as there is a #define for Window to some integer value.
class AppWindow: public WindowBase
{
public:
  AppWindow(ApplicationManager const& manager,
            glib::Object<BamfWindow> const& window);
  AppWindow(ApplicationManager const& manager,
            glib::Object<BamfView> const& window);

  WindowType type() const override;
  Window window_id() const override;
  ApplicationPtr application() const override;
  void Quit() const override;

private:
  int GetMonitor() const;
  bool GetMaximized() const;

  glib::Object<BamfWindow> bamf_window_;
};

class Tab: public WindowBase
{
public:
  Tab(ApplicationManager const& manager,
      glib::Object<BamfTab> const& tab);
  Tab(ApplicationManager const& manager,
      glib::Object<BamfView> const& tab);

  WindowType type() const override;
  Window window_id() const override;
  ApplicationPtr application() const override;
  bool Focus() const override;
  void Quit() const override;

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

  virtual AppType type() const;

  virtual WindowList const& GetWindows() const;
  virtual bool OwnsWindow(Window window_id) const;

  virtual std::vector<std::string> GetSupportedMimeTypes() const;

  virtual ApplicationWindowPtr GetFocusableWindow() const;
  virtual void Focus(bool show_on_visible, int monitor) const;

  virtual void Quit() const;

  virtual bool CreateLocalDesktopFile() const;

  virtual std::string repr() const;

  bool operator==(unity::Application const& other) const override
  {
    return static_cast<Application const*>(this)->bamf_app_ == static_cast<Application const&>(other).bamf_app_;
  }
  bool operator!=(unity::Application const& other) const override { return !(operator==(other)); }

private: // Property getters and setters
  std::string GetDesktopFile() const;

  bool GetSeen() const;
  bool SetSeen(bool param);

  bool GetSticky() const;
  bool SetSticky(bool param);

  void UpdateWindows();

private:
  glib::Object<::BamfApplication> bamf_app_;
  WindowList windows_;
  glib::SignalManager signals_;
  std::string type_;
};

class Manager : public ::unity::ApplicationManager
{
public:
  Manager();
  ~Manager();

  ApplicationPtr GetUnityApplication() const override;
  ApplicationPtr GetActiveApplication() const override;
  ApplicationWindowPtr GetActiveWindow() const override;
  ApplicationPtr GetApplicationForDesktopFile(std::string const& desktop_file) const override;
  ApplicationList GetRunningApplications() const override;
  WindowList GetWindowsForMonitor(int monitor = -1) const override;
  ApplicationPtr GetApplicationForWindow(Window xid) const override;
  ApplicationWindowPtr GetWindowForId(Window xid) const override;

  ApplicationPtr EnsureApplication(BamfView*) const;
  ApplicationWindowPtr EnsureWindow(BamfView*) const;

  void FocusWindowGroup(WindowList const&, bool show_on_visible, int monitor) const;

private:
  void OnViewOpened(BamfMatcher* matcher, BamfView* view);
  void OnViewClosed(BamfMatcher* matcher, BamfView* view);

private:
  glib::Object<BamfMatcher> matcher_;
  glib::SignalManager signals_;
};

} // namespace bamf
} // namespace unity

#endif // UNITYSHARED_APPLICATION_MANAGER_H
