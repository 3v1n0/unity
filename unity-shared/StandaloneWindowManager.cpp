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

namespace
{
nux::logging::Logger logger("unity.wm");
}

WindowManagerPtr create_window_manager()
{
  return WindowManagerPtr(new StandaloneWindowManager);
}

StandaloneWindowManager::StandaloneWindowManager()
  : expo_state_(false)
  , in_show_desktop_(false)
{
}

Window StandaloneWindowManager::GetActiveWindow() const
{
  return 0;
}

bool StandaloneWindowManager::IsWindowMaximized(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::IsWindowDecorated(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::IsWindowOnCurrentDesktop(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::IsWindowObscured(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::IsWindowMapped(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::IsWindowVisible(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::IsWindowOnTop(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::IsWindowClosable(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::IsWindowMinimized(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::IsWindowMinimizable(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::IsWindowMaximizable(Window window_id) const
{
  return false;
}

bool StandaloneWindowManager::HasWindowDecorations(Window window_id) const
{
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

void StandaloneWindowManager::Restore(Window window_id)
{}

void StandaloneWindowManager::RestoreAt(Window window_id, int x, int y)
{}

void StandaloneWindowManager::Minimize(Window window_id)
{}

void StandaloneWindowManager::Close(Window window_id)
{}

void StandaloneWindowManager::Activate(Window window_id)
{}

void StandaloneWindowManager::Raise(Window window_id)
{}

void StandaloneWindowManager::Lower(Window window_id)
{}

void StandaloneWindowManager::TerminateScale()
{}

bool StandaloneWindowManager::IsScaleActive() const
{
  return false;
}

bool StandaloneWindowManager::IsScaleActiveForGroup() const
{
  return false;
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
{}

void StandaloneWindowManager::StartMove(Window window_id, int x, int y)
{}

int StandaloneWindowManager::GetWindowMonitor(Window window_id) const
{
  return -1;
}

nux::Geometry StandaloneWindowManager::GetWindowGeometry(Window window_id) const
{
  nux::Geometry geo(0, 0, 1, 1);
  return geo;
}

nux::Geometry StandaloneWindowManager::GetWindowSavedGeometry(Window window_id) const
{
  nux::Geometry geo(0, 0, 1, 1);
  return geo;
}

nux::Geometry StandaloneWindowManager::GetScreenGeometry() const
{
  nux::Geometry geo(0, 0, 1, 1);
  return geo;
}

nux::Geometry StandaloneWindowManager::GetWorkAreaGeometry(Window window_id) const
{
  nux::Geometry geo(0, 0, 1, 1);
  return geo;
}

nux::Size StandaloneWindowManager::GetWindowDecorationSize(Window window_id, WindowManager::Edge edge) const
{
  return nux::Size(0, 0);
}

unsigned long long StandaloneWindowManager::GetWindowActiveNumber(Window window_id) const
{
  return 0;
}

void StandaloneWindowManager::SetWindowIconGeometry(Window window, nux::Geometry const& geo)
{
}

void StandaloneWindowManager::CheckWindowIntersections(nux::Geometry const& region,
                                                       bool &active, bool &any)
{
}

int StandaloneWindowManager::WorkspaceCount() const
{
  return 4;
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
  return "";
}

void StandaloneWindowManager::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper wrapper(builder);
  wrapper.add(GetScreenGeometry())
         .add("workspace_count", WorkspaceCount())
         .add("active_window", GetActiveWindow())
         .add("screen_grabbed", IsScreenGrabbed())
         .add("scale_active", IsScaleActive())
         .add("scale_active_for_group", IsScaleActiveForGroup())
         .add("expo_active", IsExpoActive())
         .add("viewport_switch_running", IsViewPortSwitchStarted())
         .add("showdesktop_active", in_show_desktop_);
}

} // namespace unity
