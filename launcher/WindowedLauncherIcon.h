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

#ifndef WINDOWED_LAUNCHER_ICON_H
#define WINDOWED_LAUNCHER_ICON_H

#include <UnityCore/GLibSignal.h>

#include "SimpleLauncherIcon.h"

namespace unity
{
namespace launcher
{

class WindowedLauncherIcon : public SimpleLauncherIcon
{
  NUX_DECLARE_OBJECT_TYPE(WindowedLauncherIcon, SimpleLauncherIcon);
public:
  WindowedLauncherIcon(AbstractLauncherIcon::IconType);

  WindowList Windows() override;
  WindowList WindowsOnViewport() override;
  WindowList WindowsForMonitor(int monitor) override;

  virtual bool IsActive() const;
  virtual bool IsStarting() const;
  virtual bool IsRunning() const;
  virtual bool IsUrgent() const;
  virtual bool IsUserVisible() const;

  virtual void Quit() const;

protected:
  virtual WindowList GetManagedWindows() const = 0;
  void EnsureWindowState();
  void EnsureWindowsLocation();

  virtual void UpdateIconGeometries(std::vector<nux::Point3> const& centers);

  std::string GetName() const override;
  void AddProperties(debug::IntrospectionData&) override;

  bool HandlesSpread() override;
  bool ShowInSwitcher(bool current) override;
  bool AllowDetailViewInSwitcher() const override;
  uint64_t SwitcherPriority() override;
  void AboutToRemove() override;

  void ActivateLauncherIcon(ActionArg arg) override;
  void PerformScroll(ScrollDirection direction, Time timestamp) override;
  virtual void Focus(ActionArg arg);
  virtual bool Spread(bool current_desktop, int state, bool force);

  typedef unsigned long int WindowFilterMask;
  enum WindowFilter
  {
    MAPPED = (1 << 0),
    USER_VISIBLE = (1 << 1),
    ON_CURRENT_DESKTOP = (1 << 2),
    ON_ALL_MONITORS = (1 << 3),
  };

  WindowList GetWindows(WindowFilterMask filter = 0, int monitor = -1);
  WindowList GetWindowsOnCurrentDesktopInStackingOrder();

  MenuItemsVector GetWindowsMenuItems();

private:
  void OnCenterStabilized(std::vector<nux::Point3> const& centers) override;
  void OnWindowMinimized(Window);
  void OnDndEnter();
  void OnDndLeave();

  Time last_scroll_timestamp_;
  unsigned int progressive_scroll_;

protected:
  glib::SignalManager glib_signals_;
};

} // namespace launcher
} // namespace unity

#endif // WINDOWED_LAUNCHER_ICON_H
