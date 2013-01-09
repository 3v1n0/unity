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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *
 */
#ifndef _UNITY_STUB_SWITCHER_CONTROLLER_H
#define _UNITY_STUB_SWITCHER_CONTROLLER_H

#include <memory>

#include <gmock/gmock.h>

#include <SwitcherController.h>
#include <SwitcherView.h>
#include <AbstractLauncherIcon.h>

namespace unity
{
namespace switcher
{
class StubSwitcherController : public ShellController
{
  public:
    typedef std::shared_ptr <StubSwitcherController> Ptr;

    StubSwitcherController()
      : ShellController()
      , window_constructed_(false)
      , view_constructed_(false)
      , view_shown_(false)
      , detail_timeout_reached_(false)
      , mock_view_ (nullptr)
    {
      Reset ();
    };

    StubSwitcherController(unsigned int load_timeout)
      : ShellController(load_timeout)
      , window_constructed_(false)
      , view_constructed_(false)
      , view_shown_(false)
      , detail_timeout_reached_(false)
      , mock_view_ (nullptr)
    {
      Reset ();
    };

    void SetView (SwitcherView *view)
    {
      mock_view_ = view;
    }

    SwitcherView * GetView ()
    {
      return mock_view_;
    }

    virtual void ConstructWindow()
    {
      window_constructed_ = true;
    }

    virtual void ConstructView()
    {
      view_constructed_ = true;
    }

    virtual void ShowView()
    {
      view_shown_ = true;
    }

    virtual bool OnDetailTimer()
    {
      detail_timeout_reached_ = true;
      clock_gettime(CLOCK_MONOTONIC, &detail_timespec_);
      return false;
    }

    void Reset()
    {
      is_visible_ = false;
      prev_count_ = 0;
      next_count_ = 0;
      index_selected_ = -1;
    }

    void Next()
    {
      ++next_count_;
    }

    void Prev()
    {
      ++prev_count_;
    }

    void Select(int index)
    {
      index_selected_ = index;
    }

    bool Visible()
    {
      return is_visible_;
    }

    void Hide(bool accept_state = true)
    {
      is_visible_ = false;
    }

    unsigned int GetConstructTimeout() const
    {
      return construct_timeout_;
    }

    void FakeSelectionChange()
    {
      unity::launcher::AbstractLauncherIcon::Ptr icon;
      OnModelSelectionChanged(icon);
    }

    bool window_constructed_;
    bool view_constructed_;
    bool view_shown_;
    bool detail_timeout_reached_;
    struct timespec detail_timespec_;

    bool is_visible_;
    int prev_count_;
    int next_count_;
    int index_selected_;

  private:
    SwitcherView *mock_view_;
};
}
}

#endif
