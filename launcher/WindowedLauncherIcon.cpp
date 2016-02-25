// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2015 Canonical Ltd
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
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <Nux/Nux.h>

#include "WindowedLauncherIcon.h"
#include "MultiMonitor.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UScreen.h"

namespace unity
{
namespace launcher
{
namespace
{
const std::string ICON_DND_OVER_TIMEOUT = "windowed-icon-dnd-over";
const int COMPIZ_SCALE_DND_SPREAD = 1 << 7;
const int MAXIMUM_QUICKLIST_WIDTH = 300;
}

NUX_IMPLEMENT_OBJECT_TYPE(WindowedLauncherIcon);

WindowedLauncherIcon::WindowedLauncherIcon(AbstractLauncherIcon::IconType icon_type)
  : SimpleLauncherIcon(icon_type)
  , last_scroll_timestamp_(0)
  , progressive_scroll_(0)
{
  WindowManager& wm = WindowManager::Default();
  wm.window_minimized.connect(sigc::mem_fun(this, &WindowedLauncherIcon::OnWindowMinimized));
  wm.screen_viewport_switch_ended.connect(sigc::mem_fun(this, &WindowedLauncherIcon::EnsureWindowState));
  wm.terminate_expo.connect(sigc::mem_fun(this, &WindowedLauncherIcon::EnsureWindowState));
  UScreen::GetDefault()->changed.connect(sigc::hide(sigc::hide(sigc::mem_fun(this, &WindowedLauncherIcon::EnsureWindowsLocation))));

  windows_changed.connect([this] (int) {
    if (WindowManager::Default().IsScaleActiveForGroup() && IsActive())
      Spread(true, 0, false);
  });
}

bool WindowedLauncherIcon::IsActive() const
{
  return GetQuirk(Quirk::ACTIVE);
}

bool WindowedLauncherIcon::IsStarting() const
{
  return GetQuirk(Quirk::STARTING);
}

bool WindowedLauncherIcon::IsRunning() const
{
  return GetQuirk(Quirk::RUNNING);
}

bool WindowedLauncherIcon::IsUrgent() const
{
  return GetQuirk(Quirk::URGENT);
}

bool WindowedLauncherIcon::IsUserVisible() const
{
  return IsRunning();
}

void WindowedLauncherIcon::ActivateLauncherIcon(ActionArg arg)
{
  SimpleLauncherIcon::ActivateLauncherIcon(arg);
  WindowManager& wm = WindowManager::Default();

  // This is a little awkward as the target is only set from the switcher.
  if (arg.target)
  {
    // thumper: should we Raise too? should the WM raise?
    wm.Activate(arg.target);
    return;
  }

  bool scale_was_active = wm.IsScaleActive();
  bool active = IsActive();
  bool user_visible = IsRunning();
  /* We should check each child to see if there is
   * an unmapped (!= minimized) window around and
   * if so force "Focus" behaviour */

  if (arg.source != ActionArg::Source::SWITCHER)
  {
    user_visible = IsUserVisible();

    if (active)
    {
      bool any_visible = false;
      bool any_mapped = false;
      bool any_on_top = false;
      bool any_on_monitor = (arg.monitor < 0);
      int active_monitor = arg.monitor;

      for (auto const& window : GetManagedWindows())
      {
        Window xid = window->window_id();

        if (!any_visible && wm.IsWindowOnCurrentDesktop(xid))
        {
          any_visible = true;
        }

        if (!any_mapped && wm.IsWindowMapped(xid))
        {
          any_mapped = true;
        }

        if (!any_on_top && wm.IsWindowOnTop(xid))
        {
          any_on_top = true;
        }

        if (!any_on_monitor && window->monitor() == arg.monitor &&
            wm.IsWindowMapped(xid) && wm.IsWindowVisible(xid))
        {
          any_on_monitor = true;
        }

        if (window->active())
        {
          active_monitor = window->monitor();
        }
      }

      if (!any_visible || !any_mapped || !any_on_top)
        active = false;

      if (any_on_monitor && arg.monitor >= 0 && active_monitor != arg.monitor)
        active = false;
    }
  }

  /* Behaviour:
   * 1) Nothing running, or nothing visible -> launch application
   * 2) Running and active -> spread application
   * 3) Running and not active -> focus application
   * 4) Spread is active and different icon pressed -> change spread
   * 5) Spread is active -> Spread de-activated, and fall through
   */

  if (!IsRunning() || (IsRunning() && !user_visible)) // #1 above
  {
    if (GetQuirk(Quirk::STARTING, arg.monitor))
      return;

    wm.TerminateScale();
    SetQuirk(Quirk::STARTING, true, arg.monitor);
    OpenInstanceLauncherIcon(arg.timestamp);
  }
  else // container is running
  {
    if (active)
    {
      if (scale_was_active) // #5 above
      {
        wm.TerminateScale();

        if (minimize_window_on_click())
        {
          for (auto const& win : GetWindows(WindowFilter::ON_CURRENT_DESKTOP))
            wm.Minimize(win->window_id());
        }
        else
        {
          Focus(arg);
        }
      }
      else // #2 above
      {
        if (arg.source != ActionArg::Source::SWITCHER)
        {
          bool minimized = false;

          if (minimize_window_on_click())
          {
            WindowList const& windows = GetWindows(WindowFilter::ON_CURRENT_DESKTOP);

            if (windows.size() == 1)
            {
              wm.Minimize(windows[0]->window_id());
              minimized = true;
            }
          }

          if (!minimized)
          {
            Spread(true, 0, false);
          }
        }
      }
    }
    else
    {
      if (scale_was_active) // #4 above
      {
        if (GetWindows(WindowFilter::ON_CURRENT_DESKTOP).size() <= 1)
          wm.TerminateScale();

        Focus(arg);

        if (arg.source != ActionArg::Source::SWITCHER)
          Spread(true, 0, false);
      }
      else // #3 above
      {
        Focus(arg);
      }
    }
  }
}

WindowList WindowedLauncherIcon::GetWindows(WindowFilterMask filter, int monitor)
{
  if ((!filter && monitor < 0) || (filter == WindowFilter::ON_ALL_MONITORS))
    return GetManagedWindows();

  WindowManager& wm = WindowManager::Default();
  WindowList results;

  monitor = (filter & WindowFilter::ON_ALL_MONITORS) ? -1 : monitor;
  bool mapped = (filter & WindowFilter::MAPPED);
  bool user_visible = (filter & WindowFilter::USER_VISIBLE);
  bool current_desktop = (filter & WindowFilter::ON_CURRENT_DESKTOP);

  for (auto& window : GetManagedWindows())
  {
    if ((monitor >= 0 && window->monitor() == monitor) || monitor < 0)
    {
      if ((user_visible && window->visible()) || !user_visible)
      {
        Window xid = window->window_id();

        if ((mapped && wm.IsWindowMapped(xid)) || !mapped)
        {
          if ((current_desktop && wm.IsWindowOnCurrentDesktop(xid)) || !current_desktop)
          {
            results.push_back(window);
          }
        }
      }
    }
  }

  return results;
}

WindowList WindowedLauncherIcon::Windows()
{
  return GetWindows(WindowFilter::MAPPED|WindowFilter::ON_ALL_MONITORS);
}

WindowList WindowedLauncherIcon::WindowsOnViewport()
{
  WindowFilterMask filter = 0;
  filter |= WindowFilter::MAPPED;
  filter |= WindowFilter::USER_VISIBLE;
  filter |= WindowFilter::ON_CURRENT_DESKTOP;
  filter |= WindowFilter::ON_ALL_MONITORS;

  return GetWindows(filter);
}

WindowList WindowedLauncherIcon::WindowsForMonitor(int monitor)
{
  WindowFilterMask filter = 0;
  filter |= WindowFilter::MAPPED;
  filter |= WindowFilter::USER_VISIBLE;
  filter |= WindowFilter::ON_CURRENT_DESKTOP;

  return GetWindows(filter, monitor);
}

void WindowedLauncherIcon::OnWindowMinimized(Window xid)
{
  for (auto const& window : GetManagedWindows())
  {
    if (xid == window->window_id())
    {
      int monitor = GetCenterForMonitor(window->monitor()).first;

      if (monitor >= 0)
      {
        Present(0.5f, 600, monitor);
        FullyAnimateQuirkDelayed(300, Quirk::SHIMMER, monitor);
      }

      break;
    }
  }
}

void WindowedLauncherIcon::Focus(ActionArg arg)
{
  bool show_only_visible = (arg.source == ActionArg::Source::SWITCHER);
  ApplicationManager::Default().FocusWindowGroup(GetManagedWindows(), show_only_visible, arg.monitor);
}

bool WindowedLauncherIcon::Spread(bool current_desktop, int state, bool force)
{
  std::vector<Window> windows;
  for (auto& window : GetWindows(current_desktop ? WindowFilter::ON_CURRENT_DESKTOP : 0))
    windows.push_back(window->window_id());

  return WindowManager::Default().ScaleWindowGroup(windows, state, force);
}

void WindowedLauncherIcon::EnsureWindowState()
{
  std::vector<int> number_of_windows_on_monitor(monitors::MAX);

  for (auto const& window : WindowsOnViewport())
  {
    int monitor = window->monitor();

    // If monitor is -1 (or negative), show on all monitors.
    if (monitor < 0)
    {
      for (unsigned j; j < monitors::MAX; ++j)
        ++number_of_windows_on_monitor[j];
    }
    else
    {
      ++number_of_windows_on_monitor[monitor];
    }
  }

  for (unsigned i = 0; i < monitors::MAX; ++i)
    SetNumberOfWindowsVisibleOnMonitor(number_of_windows_on_monitor[i], i);
}

void WindowedLauncherIcon::EnsureWindowsLocation()
{
  EnsureWindowState();
  UpdateIconGeometries(GetCenters());
}

void WindowedLauncherIcon::UpdateIconGeometries(std::vector<nux::Point3> const& centers)
{
  nux::Geometry geo(0, 0, icon_size, icon_size);

  for (auto& window : GetManagedWindows())
  {
    Window xid = window->window_id();
    int monitor = GetCenterForMonitor(window->monitor()).first;

    if (monitor < 0)
    {
      WindowManager::Default().SetWindowIconGeometry(xid, nux::Geometry());
      continue;
    }

    geo.x = centers[monitor].x - icon_size / 2;
    geo.y = centers[monitor].y - icon_size / 2;
    WindowManager::Default().SetWindowIconGeometry(xid, geo);
  }
}

void WindowedLauncherIcon::OnCenterStabilized(std::vector<nux::Point3> const& centers)
{
  UpdateIconGeometries(centers);
}

void WindowedLauncherIcon::OnDndEnter()
{
  auto timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;

  _source_manager.AddTimeout(1000, [this, timestamp] {
    bool to_spread = GetWindows(WindowFilter::ON_CURRENT_DESKTOP).size() > 1;

    if (!to_spread)
      WindowManager::Default().TerminateScale();

    if (!IsRunning())
      return false;

    UBusManager::SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);
    Focus(ActionArg(ActionArg::Source::LAUNCHER, 1, timestamp));

    if (to_spread)
      Spread(true, COMPIZ_SCALE_DND_SPREAD, false);

    return false;
  }, ICON_DND_OVER_TIMEOUT);
}

void WindowedLauncherIcon::OnDndLeave()
{
  _source_manager.Remove(ICON_DND_OVER_TIMEOUT);
}

bool WindowedLauncherIcon::HandlesSpread()
{
  return true;
}

bool WindowedLauncherIcon::ShowInSwitcher(bool current)
{
  if (!removed() && IsRunning() && IsVisible())
  {
    // If current is true, we only want to show the current workspace.
    if (!current)
    {
      return true;
    }
    else
    {
      for (unsigned i = 0; i < monitors::MAX; ++i)
      {
        if (WindowVisibleOnMonitor(i))
          return true;
      }
    }
  }

  return false;
}

bool WindowedLauncherIcon::AllowDetailViewInSwitcher() const
{
  return true;
}

uint64_t WindowedLauncherIcon::SwitcherPriority()
{
  uint64_t result = 0;

  for (auto& window : GetManagedWindows())
  {
    Window xid = window->window_id();
    result = std::max(result, WindowManager::Default().GetWindowActiveNumber(xid));
  }

  return result;
}

void PerformScrollUp(WindowList const& windows, unsigned int progressive_scroll)
{
  if (progressive_scroll == windows.size() - 1)
  {
    //RestackAbove to preserve Global Stacking Order
    WindowManager::Default().RestackBelow(windows.at(0)->window_id(), windows.at(1)->window_id());
    WindowManager::Default().RestackBelow(windows.at(1)->window_id(), windows.at(0)->window_id());
    windows.back()->Focus();
    return;
  }

  WindowManager::Default().RestackBelow(windows.at(0)->window_id(), windows.at(progressive_scroll + 1)->window_id());
  windows.at(progressive_scroll + 1)->Focus();
}

void PerformScrollDown(WindowList const& windows, unsigned int progressive_scroll)
{
  if (!progressive_scroll)
  {
    WindowManager::Default().RestackBelow(windows.at(0)->window_id(), windows.back()->window_id());
    windows.at(1)->Focus();
    return;
  }

  WindowManager::Default().RestackBelow(windows.at(0)->window_id(), windows.at(progressive_scroll)->window_id());
  windows.at(progressive_scroll)->Focus();
}

void WindowedLauncherIcon::PerformScroll(ScrollDirection direction, Time timestamp)
{
  if (timestamp - last_scroll_timestamp_ < 150)
    return;
  else if (timestamp - last_scroll_timestamp_ > 1500)
    progressive_scroll_ = 0;

  last_scroll_timestamp_ = timestamp;

  auto const& windows = GetWindowsOnCurrentDesktopInStackingOrder();

  if (windows.empty())
    return;

  if (scroll_inactive_icons && !IsActive())
  {
    windows.at(0)->Focus();
    return;
  }

  if (!scroll_inactive_icons && !IsActive())
    return;

  if (windows.size() <= 1)
    return;

  if (direction == ScrollDirection::DOWN)
    ++progressive_scroll_;
  else
    //--progressive_scroll_; but roll to the top of windows
    progressive_scroll_ += windows.size() - 1;
  progressive_scroll_ %= windows.size();

  switch(direction)
  {
  case ScrollDirection::UP:
    PerformScrollUp(windows, progressive_scroll_);
    break;
  case ScrollDirection::DOWN:
    PerformScrollDown(windows, progressive_scroll_);
    break;
  }
}

WindowList WindowedLauncherIcon::GetWindowsOnCurrentDesktopInStackingOrder()
{
  auto windows = GetWindows(WindowFilter::ON_CURRENT_DESKTOP | WindowFilter::ON_ALL_MONITORS);
  auto sorted_windows = WindowManager::Default().GetWindowsInStackingOrder();

  // Order the windows
  std::sort(windows.begin(), windows.end(), [&sorted_windows] (ApplicationWindowPtr const& win1, ApplicationWindowPtr const& win2) {
    for (auto const& window : sorted_windows)
    {
      if (window == win1->window_id())
        return false;
      else if (window == win2->window_id())
        return true;
    }

    return true;
  });

  return windows;
}

void WindowedLauncherIcon::Quit() const
{
  for (auto& window : GetManagedWindows())
    window->Quit();
}

void WindowedLauncherIcon::AboutToRemove()
{
  Quit();
}

AbstractLauncherIcon::MenuItemsVector WindowedLauncherIcon::GetWindowsMenuItems()
{
  auto const& windows = Windows();
  MenuItemsVector menu_items;

  // We only add quicklist menu-items for windows if we have more than one window
  if (windows.size() < 2)
    return menu_items;

  // add menu items for all open windows
  for (auto const& w : windows)
  {
    auto const& title = w->title();

    if (title.empty())
      continue;

    glib::Object<DbusmenuMenuitem> menu_item(dbusmenu_menuitem_new());
    dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_LABEL, title.c_str());
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_ENABLED, true);
    dbusmenu_menuitem_property_set_bool(menu_item, DBUSMENU_MENUITEM_PROP_VISIBLE, true);
    dbusmenu_menuitem_property_set_bool(menu_item, QuicklistMenuItem::MARKUP_ACCEL_DISABLED_PROPERTY, true);
    dbusmenu_menuitem_property_set_int(menu_item, QuicklistMenuItem::MAXIMUM_LABEL_WIDTH_PROPERTY, MAXIMUM_QUICKLIST_WIDTH);

    Window xid = w->window_id();
    glib_signals_.Add<void, DbusmenuMenuitem*, unsigned>(menu_item, DBUSMENU_MENUITEM_SIGNAL_ITEM_ACTIVATED,
      [xid] (DbusmenuMenuitem*, unsigned) {
        WindowManager& wm = WindowManager::Default();
        wm.Activate(xid);
        wm.Raise(xid);
    });

    if (w->active())
    {
      dbusmenu_menuitem_property_set(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_TYPE, DBUSMENU_MENUITEM_TOGGLE_RADIO);
      dbusmenu_menuitem_property_set_int(menu_item, DBUSMENU_MENUITEM_PROP_TOGGLE_STATE, DBUSMENU_MENUITEM_TOGGLE_STATE_CHECKED);
    }

    menu_items.push_back(menu_item);
  }

  return menu_items;
}

std::string WindowedLauncherIcon::GetName() const
{
  return "WindowedLauncherIcon";
}

void WindowedLauncherIcon::AddProperties(debug::IntrospectionData& introspection)
{
  SimpleLauncherIcon::AddProperties(introspection);

  std::vector<Window> xids;
  for (auto const& window : GetManagedWindows())
    xids.push_back(window->window_id());

  introspection.add("xids", glib::Variant::FromVector(xids))
               .add("sticky", IsSticky());
}

} // namespace launcher
} // namespace unity
