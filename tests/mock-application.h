/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Tim Penhey  <tim.penhey@canonical.com>
 */
#ifndef TESTS_MOCK_APPLICATION_H
#define TESTS_MOCK_APPLICATION_H

#include <map>

#include "unity-shared/ApplicationManager.h"
#include "unity-shared/WindowManager.h"

namespace testmocks
{
struct MockApplicationWindow : unity::ApplicationWindow
{
  MockApplicationWindow(Window xid)
    : xid_(xid)
    , monitor_(0)
    , title_("MockApplicationWindow "+std::to_string(xid_))
    , type_("window")
    , visible_(true)
    , active_(false)
    , urgent_(false)
  {
    visible.SetGetterFunction([this] { return visible_; });
    active.SetGetterFunction([this] { return active_; });
    urgent.SetGetterFunction([this] { return urgent_; });
  }

  Window xid_;
  int monitor_;
  std::string title_;
  std::string icon_;
  std::string type_;

  bool visible_;
  bool active_;
  bool urgent_;

  virtual std::string title() const { return title_; }
  virtual std::string icon() const { return icon_; }
  virtual std::string type() const { return type_; }

  virtual Window window_id() const { return xid_; }
  virtual int monitor() const { return monitor_; }

  virtual unity::ApplicationPtr application() const { return unity::ApplicationPtr(); }
  virtual bool Focus() const
  {
    auto& wm = unity::WindowManager::Default();
    wm.Raise(xid_);
    wm.Activate(xid_);
    return true;
  }
  virtual void Quit() const {}
};

struct MockApplication : unity::Application
{
  MockApplication(std::string const& desktop_file,
                  std::string const& icon = "",
                  std::string const& title = "")
    : desktop_file_(desktop_file)
    , icon_(icon)
    , title_(title)
    , seen_(false)
    , sticky_(false)
    , visible_(false)
    , active_(false)
    , running_(false)
    , urgent_(false)
    , type_("mock")
    {
      seen.SetGetterFunction(sigc::mem_fun(this, &MockApplication::GetSeen));
      seen.SetSetterFunction(sigc::mem_fun(this, &MockApplication::SetSeen));
      sticky.SetGetterFunction(sigc::mem_fun(this, &MockApplication::GetSticky));
      sticky.SetSetterFunction(sigc::mem_fun(this, &MockApplication::SetSticky));
      visible.SetGetterFunction(sigc::mem_fun(this, &MockApplication::GetVisible));
      active.SetGetterFunction(sigc::mem_fun(this, &MockApplication::GetActive));
      running.SetGetterFunction(sigc::mem_fun(this, &MockApplication::GetRunning));
      urgent.SetGetterFunction(sigc::mem_fun(this, &MockApplication::GetUrgent));
    }

  std::string desktop_file_;
  std::string icon_;
  std::string title_;
  bool seen_;
  bool sticky_;
  bool visible_;
  bool active_;
  bool running_;
  bool urgent_;
  unity::WindowList windows_;
  std::string type_;


  virtual std::string icon() const { return icon_; }
  virtual std::string title() const { return title_; }
  virtual std::string desktop_file() const { return desktop_file_; }
  virtual std::string type() const { return type_; }
  virtual std::string repr() const { return "MockApplication"; }

  virtual unity::WindowList GetWindows() const { return windows_; }
  virtual bool OwnsWindow(Window window_id) const {
    auto end = std::end(windows_);
    return std::find_if(std::begin(windows_), end, [window_id] (unity::ApplicationWindowPtr window) {
      return window->window_id() == window_id;
    }) != end;
  }

  virtual std::vector<std::string> GetSupportedMimeTypes() const { return {}; }

  virtual unity::ApplicationWindowPtr GetFocusableWindow() const { return unity::ApplicationWindowPtr(); }
  virtual void Focus(bool show_on_visible, int monitor) const  {}
  virtual void Quit() const {}
  void SetRunState(bool state) {
    running_ = state;
    running.changed.emit(state);
    }

  bool GetSeen() const { return seen_; }
  bool SetSeen(bool const& param) {
    if (param != seen_) {
      seen_ = param;
      return true;
    }
    return false;
  }

  bool GetSticky() const { return sticky_; }
  bool SetSticky(bool const& param) {
    if (param != sticky_) {
      sticky_ = param;
      return true;
    }
    return false;
  }

  void SetActiveState(bool state)
  {
    if (active_ == state)
      return;
    active_ = state;
    active.changed.emit(state);
  }

  bool GetVisible() const { return visible_; }
  bool GetActive() const { return active_; }
  bool GetRunning() const { return running_; }
  bool GetUrgent() const { return urgent_; }
};

class MockApplicationManager : public unity::ApplicationManager
{
public:
  static void StartApp(std::string const& desktop_file)
  {
      // We know the application manager is a mock one so we can cast it.
      auto self = dynamic_cast<MockApplicationManager&>(unity::ApplicationManager::Default());
      auto app = self.GetApplicationForDesktopFile(desktop_file);
      self.application_started.emit(app);
  }

  virtual unity::ApplicationWindowPtr GetActiveWindow()
  {
      return unity::ApplicationWindowPtr();
  }

  unity::ApplicationPtr GetApplicationForDesktopFile(std::string const& desktop_file)
  {
    AppMap::iterator iter = app_map_.find(desktop_file);
    if (iter == app_map_.end())
    {
      unity::ApplicationPtr app(new MockApplication(desktop_file));
      app_map_.insert(AppMap::value_type(desktop_file, app));
      return app;
    }
    else
    {
      return iter->second;
    }
  }

  unity::ApplicationList GetRunningApplications()
  {
      return unity::ApplicationList();
  }

  unity::ApplicationPtr GetApplicationForWindow(Window xid)
  {
    return unity::ApplicationPtr();
  }

private:
  typedef std::map<std::string, unity::ApplicationPtr> AppMap;
  AppMap app_map_;
};

}

#endif
