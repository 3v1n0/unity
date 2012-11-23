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

#include "unity-shared/ApplicationManager.h"

namespace testmocks
{
class MockApplication : public unity::Application
{
public:
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

  virtual std::string icon() const { return icon_; }
  virtual std::string title() const { return title_; }
  virtual std::string desktop_file() const { return desktop_file_; }
  virtual std::string type() const { return "mock"; }

  virtual unity::WindowList GetWindows() const { return unity::WindowList(); }
  virtual bool OwnsWindow(Window window_id) const { return false; }

  virtual std::vector<std::string> GetSupportedMimeTypes() const { return {}; }
  virtual std::vector<unity::ApplicationMenu> GetRemoteMenus() const { return {}; }

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

  bool GetVisible() const { return visible_; }
  bool GetActive() const { return active_; }
  bool GetRunning() const { return running_; }
  bool GetUrgent() const { return urgent_; }
};

}

#endif
