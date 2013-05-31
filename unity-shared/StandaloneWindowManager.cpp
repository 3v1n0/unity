// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Tim Penhey <tim.penhey@canonical.com>
 *              Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <glib.h>
#include <sstream>
#include "StandaloneWindowManager.h"
#include "UScreen.h"

#include <NuxCore/Logger.h>
#include <UnityCore/Variant.h>

// Entirely stubs for now, unless we need this functionality at some point

namespace unity
{
DECLARE_LOGGER(logger, "unity.wm");

StandaloneWindow::StandaloneWindow(Window xid)
  : xid(xid)
  , name("StandaloneWindow " + std::to_string(xid))
  , geo(nux::Geometry(0, 0, 10, 10))
  , current_desktop(0)
  , monitor(0)
  , active_number(0)
  , active(false)
  , mapped(true)
  , visible(true)
  , maximized(false)
  , minimized(false)
  , decorated(true)
  , has_decorations(true)
  , on_top(false)
  , closable(true)
  , minimizable(true)
  , maximizable(true)
{
  geo.SetSetterFunction([this] (nux::Geometry &target, nux::Geometry const& new_value) {
    if (target.x != new_value.x || target.y != new_value.y)
      moved.emit();

    if (target.width != new_value.width || target.height != new_value.height)
      resized.emit();

    if (target != new_value)
    {
      target = new_value;
      return true;
    }

    return false;
  });
}

WindowManagerPtr create_window_manager()
{
  return WindowManagerPtr(new StandaloneWindowManager);
}

StandaloneWindowManager::StandaloneWindowManager()
  : expo_state_(false)
  , in_show_desktop_(false)
  , scale_active_(false)
  , scale_active_for_group_(false)
  , current_desktop_(0)
  , viewport_size_(2, 2)
{}

Window StandaloneWindowManager::GetActiveWindow() const
{
  for (auto const& window : standalone_windows_)
    if (window->active)
      return window->Xid();

  return 0;
}

StandaloneWindow::Ptr StandaloneWindowManager::GetWindowByXid(Window window_id) const
{
  auto begin = standalone_windows_.begin();
  auto end = standalone_windows_.end();
  auto it = std::find_if(begin, end, [window_id] (StandaloneWindow::Ptr window) {
    return window->Xid() == window_id;
  });

  if (it != end)
    return *it;
  else
    return StandaloneWindow::Ptr();
}

std::vector<Window> StandaloneWindowManager::GetWindowsInStackingOrder() const
{
  std::vector<Window> ret;
  for (auto const& window : standalone_windows_)
    ret.push_back(window->Xid());

  return ret;
}

bool StandaloneWindowManager::IsWindowMaximized(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->maximized;

  return false;
}

bool StandaloneWindowManager::IsWindowDecorated(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window && window->has_decorations)
    return window->decorated;

  return false;
}

bool StandaloneWindowManager::IsWindowOnCurrentDesktop(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return (window->current_desktop == current_desktop_);

  return true;
}

bool StandaloneWindowManager::IsWindowObscured(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::IsWindowMapped(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->mapped;

  return true;
}

bool StandaloneWindowManager::IsWindowVisible(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->visible;

  return true;
}

bool StandaloneWindowManager::IsWindowOnTop(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->on_top;

  return false;
}

bool StandaloneWindowManager::IsWindowClosable(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->closable;

  return false;
}

bool StandaloneWindowManager::IsWindowMinimized(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->minimized;

  return false;
}

bool StandaloneWindowManager::IsWindowMinimizable(Window window_id) const
{
  auto window = GetWindowByXid(window_id);

  if (window)
    return window->minimizable;

  return false;
}

bool StandaloneWindowManager::IsWindowMaximizable(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->maximizable;

  return false;
}

bool StandaloneWindowManager::HasWindowDecorations(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->has_decorations;

  return false;
}

void StandaloneWindowManager::ShowDesktop()
{
  in_show_desktop_ = !in_show_desktop_;
}

bool StandaloneWindowManager::InShowDesktop() const
{
  return in_show_desktop_;
}

void StandaloneWindowManager::Decorate(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    window->decorated = window->has_decorations();
}

void StandaloneWindowManager::Undecorate(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    window->decorated = false;
}

void StandaloneWindowManager::Maximize(Window window_id)
{
  auto window = GetWindowByXid(window_id);
  if (window)
  {
    window->maximized = true;
    Undecorate(window_id);
  }
}

void StandaloneWindowManager::Restore(Window window_id)
{
  auto window = GetWindowByXid(window_id);
  if (window)
  {
    window->maximized = false;
    Decorate(window_id);
  }
}

void StandaloneWindowManager::RestoreAt(Window window_id, int x, int y)
{
  nux::Geometry new_geo = GetWindowGeometry(window_id);
  new_geo.SetPosition(x, y);
  Restore(window_id);
  MoveResizeWindow(window_id, new_geo);
}

void StandaloneWindowManager::UnMinimize(Window window_id)
{
  auto window = GetWindowByXid(window_id);
  if (window)
  {
    window->minimized = false;

    if (window->maximized)
      Undecorate(window_id);
  }
}

void StandaloneWindowManager::Minimize(Window window_id)
{
  auto window = GetWindowByXid(window_id);
  if (window)
  {
    window->minimized = true;

    if (window->maximized)
      Decorate(window_id);
  }
}

void StandaloneWindowManager::Close(Window window_id)
{
  standalone_windows_.remove_if([window_id] (StandaloneWindow::Ptr window) {
    return window->Xid() == window_id;
  });
}

void StandaloneWindowManager::Activate(Window window_id)
{
  auto window = GetWindowByXid(window_id);
  if (window)
    // This will automatically set the others active windows as unactive
    window->active = true;
}

void StandaloneWindowManager::Lower(Window window_id)
{
  auto begin = standalone_windows_.begin();
  auto end = standalone_windows_.end();
  auto window = std::find_if(begin, end, [window_id] (StandaloneWindow::Ptr window) {
    return window->Xid() == window_id;
  });

  if (window != end)
    standalone_windows_.splice(begin, standalone_windows_, window);
}

void StandaloneWindowManager::Raise(Window window_id)
{
  auto end = standalone_windows_.end();
  auto window = std::find_if(standalone_windows_.begin(), end, [window_id] (StandaloneWindow::Ptr window) {
    return window->Xid() == window_id;
  });

  if (window != end)
    standalone_windows_.splice(end, standalone_windows_, window);
}

void StandaloneWindowManager::RestackBelow(Window window_id, Window sibiling_id)
{
  auto end = standalone_windows_.end();
  auto begin = standalone_windows_.begin();

  auto window = std::find_if(begin, end, [window_id] (StandaloneWindow::Ptr window) {
    return window->Xid() == window_id;
  });
  auto sibiling = std::find_if(begin, end, [sibiling_id] (StandaloneWindow::Ptr window) {
    return window->Xid() == sibiling_id;
  });

  if (window != end && sibiling != end)
    standalone_windows_.splice(sibiling, standalone_windows_, window);
}

void StandaloneWindowManager::TerminateScale()
{}

void StandaloneWindowManager::SetScaleActive(bool scale_active)
{
  scale_active_ = scale_active;
}

bool StandaloneWindowManager::IsScaleActive() const
{
  return scale_active_;
}

void StandaloneWindowManager::SetScaleActiveForGroup(bool scale_active_for_group)
{
  scale_active_for_group_ = scale_active_for_group;
}

bool StandaloneWindowManager::IsScaleActiveForGroup() const
{
  return scale_active_for_group_;
}

void StandaloneWindowManager::InitiateExpo()
{
  expo_state_ = !expo_state_;
}

void StandaloneWindowManager::TerminateExpo()
{
  expo_state_ = false;
}

bool StandaloneWindowManager::IsExpoActive() const
{
  return expo_state_;
}

bool StandaloneWindowManager::IsWallActive() const
{
  return false;
}

void StandaloneWindowManager::FocusWindowGroup(std::vector<Window> const& windows,
                                               FocusVisibility,
                                               int monitor,
                                               bool only_top_win)
{}

bool StandaloneWindowManager::ScaleWindowGroup(std::vector<Window> const& windows,
                                               int state, bool force)
{
  return false;
}

bool StandaloneWindowManager::IsScreenGrabbed() const
{
  return false;
}

bool StandaloneWindowManager::IsViewPortSwitchStarted() const
{
  return false;
}

void StandaloneWindowManager::MoveResizeWindow(Window window_id, nux::Geometry geometry)
{
  auto window = GetWindowByXid(window_id);
  if (window)
    window->geo = geometry;
}

void StandaloneWindowManager::StartMove(Window window_id, int x, int y)
{
  // This is called when we ask the WM to start the movement of a window,
  // but it does not actually move it.
}

int StandaloneWindowManager::GetWindowMonitor(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->monitor;

  return -1;
}

nux::Geometry StandaloneWindowManager::GetWindowGeometry(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->geo;

  return nux::Geometry(0, 0, 1, 1);
}

nux::Geometry StandaloneWindowManager::GetWindowSavedGeometry(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->geo;

  return nux::Geometry();
}

nux::Geometry StandaloneWindowManager::GetScreenGeometry() const
{
  return nux::Geometry(0, 0, 1024, 768);
}

nux::Geometry StandaloneWindowManager::GetWorkAreaGeometry(Window window_id) const
{
  return workarea_geo_;
}

void StandaloneWindowManager::SetWorkareaGeometry(nux::Geometry const& geo)
{
  workarea_geo_ = geo;
}

nux::Size StandaloneWindowManager::GetWindowDecorationSize(Window window_id, WindowManager::Edge edge) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->deco_sizes[unsigned(edge)];

  return nux::Size();
}

uint64_t StandaloneWindowManager::GetWindowActiveNumber(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->active_number;

  return 0;
}

void StandaloneWindowManager::SetWindowIconGeometry(Window window, nux::Geometry const& geo)
{
}

void StandaloneWindowManager::CheckWindowIntersections(nux::Geometry const& region,
                                                       bool &active, bool &any)
{
}

void StandaloneWindowManager::SetViewportSize(int horizontal, int vertical)
{
  if (horizontal < 1 || vertical < 1)
    return;

  nux::Size new_size(horizontal, vertical);

  if (viewport_size_ == new_size)
    return;

  viewport_size_ = new_size;
  viewport_layout_changed.emit(new_size.width, new_size.height);
}

int StandaloneWindowManager::WorkspaceCount() const
{
  return viewport_size_.width * viewport_size_.height;
}

nux::Point StandaloneWindowManager::GetCurrentViewport() const
{
  return current_vp_;
}

 int StandaloneWindowManager::GetViewportHSize() const
{
  return viewport_size_.width;
}

int StandaloneWindowManager::GetViewportVSize() const
{
  return viewport_size_.height;
}

bool StandaloneWindowManager::SaveInputFocus()
{
  return false;
}

bool StandaloneWindowManager::RestoreInputFocus()
{
  return false;
}

std::string StandaloneWindowManager::GetWindowName(Window window_id) const
{
  auto window = GetWindowByXid(window_id);
  if (window)
    return window->name;

  return "";
}

// Mock functions

void StandaloneWindowManager::SetCurrentDesktop(unsigned desktop_id)
{
  current_desktop_ = desktop_id;
}

void StandaloneWindowManager::AddStandaloneWindow(StandaloneWindow::Ptr const& window)
{
  if (!window)
    return;

  auto xid = window->Xid();
  Close(xid);
  standalone_windows_.push_back(window);

  window->mapped.changed.connect([this, xid] (bool v) {v ? window_mapped(xid) : window_unmapped(xid);});
  window->visible.changed.connect([this, xid] (bool v) {v ? window_shown(xid) : window_hidden(xid);});
  window->maximized.changed.connect([this, xid] (bool v) {v ? window_maximized(xid) : window_restored(xid);});
  window->minimized.changed.connect([this, xid] (bool v) {v ? window_minimized(xid) : window_unminimized(xid);});
  window->decorated.changed.connect([this, xid] (bool v) {v ? window_decorated(xid) : window_undecorated(xid);});
  window->has_decorations.changed.connect([this, xid] (bool v) {v ? window_decorated(xid) : window_undecorated(xid);});
  window->resized.connect([this, xid] { window_resized(xid); });
  window->moved.connect([this, xid] { window_moved(xid); });

  window->active.changed.connect([this, xid] (bool active) {
    if (!active)
      return;

    // Ensuring that this is the only active window we have on screen
    for (auto const& window : standalone_windows_)
      if (window->Xid() != xid && window->active)
        window->active = false;

    window_focus_changed(xid);
  });

  window->active = true;
}

std::list<StandaloneWindow::Ptr> StandaloneWindowManager::GetStandaloneWindows() const
{
  return standalone_windows_;
}

void StandaloneWindowManager::SetCurrentViewport(nux::Point const& vp)
{
  current_vp_ = vp;
}

void StandaloneWindowManager::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper wrapper(builder);
  wrapper.add(GetScreenGeometry())
         .add("workspace_count", WorkspaceCount())
         .add("active_window", (uint64_t)GetActiveWindow())
         .add("screen_grabbed", IsScreenGrabbed())
         .add("scale_active", IsScaleActive())
         .add("scale_active_for_group", IsScaleActiveForGroup())
         .add("expo_active", IsExpoActive())
         .add("viewport_switch_running", IsViewPortSwitchStarted())
         .add("showdesktop_active", in_show_desktop_);
}

} // namespace unity
