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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef UNITYSHARED_WINDOW_MANAGER_H
#define UNITYSHARED_WINDOW_MANAGER_H

#include <memory>
#include <vector>
#include <sigc++/sigc++.h>

// To bring in nux::Geometry we first need the Rect header, then Utils.
#include <NuxCore/Rect.h>
#include <NuxCore/Property.h>
#include <Nux/Utils.h>

#ifdef USE_X11
#include <X11/Xlib.h>
#else
typedef unsigned long Window;
typedef unsigned long Atom;
#endif

#include "unity-shared/Introspectable.h"

namespace unity
{
class WindowManager;
typedef std::shared_ptr<WindowManager> WindowManagerPtr;

// This function is used by the static Default method on the WindowManager
// class, and uses link time to make sure there is a function available.
WindowManagerPtr create_window_manager();

class WindowManager : public debug::Introspectable
{
  // This is a glue interface that breaks the dependancy of Unity with Compiz
  // Basically it has a default implementation that does nothing useful, but
  // the idea is that unity.cpp uses SetDefault() early enough in it's
  // initialization so the things that require it get a usable implementation

public:
  virtual ~WindowManager() {}

  enum class FocusVisibility
  {
    OnlyVisible,
    ForceUnminimizeInvisible,
    ForceUnminimizeOnCurrentDesktop
  };

  enum class Edge : unsigned
  {
    LEFT,
    RIGHT,
    TOP,
    BOTTOM
  };

  static WindowManager& Default();

  virtual Window GetActiveWindow() const = 0;
  virtual std::vector<Window> GetWindowsInStackingOrder() const = 0;

  virtual bool IsTopWindowFullscreenOnMonitorWithMouse() const = 0;

  virtual bool IsWindowMaximized(Window window_id) const = 0;
  virtual bool IsWindowVerticallyMaximized(Window window_id) const = 0;
  virtual bool IsWindowHorizontallyMaximized(Window window_id) const = 0;
  virtual bool IsWindowDecorated(Window window_id) const = 0;
  virtual bool IsWindowOnCurrentDesktop(Window window_id) const = 0;
  virtual bool IsWindowObscured(Window window_id) const = 0;
  virtual bool IsWindowMapped(Window window_id) const = 0;
  virtual bool IsWindowVisible(Window window_id) const = 0;
  virtual bool IsWindowOnTop(Window window_id) const = 0;
  virtual bool IsWindowClosable(Window window_id) const = 0;
  virtual bool IsWindowMinimized(Window window_id) const = 0;
  virtual bool IsWindowMinimizable(Window window_id) const = 0;
  virtual bool IsWindowMaximizable(Window window_id) const = 0;
  virtual bool HasWindowDecorations(Window window_id) const = 0;

  virtual void ShowDesktop() = 0;
  virtual bool InShowDesktop() const = 0;

  virtual void ShowActionMenu(Time, Window, unsigned button, nux::Point const&) = 0;
  virtual void Maximize(Window window_id) = 0;
  virtual void Restore(Window window_id) = 0;
  virtual void RestoreAt(Window window_id, int x, int y) = 0;
  virtual void Minimize(Window window_id) = 0;
  virtual void UnMinimize(Window window_id) = 0;
  virtual void Close(Window window_id) = 0;

  virtual void Activate(Window window_id) = 0;
  virtual void Raise(Window window_id) = 0;
  virtual void Lower(Window window_id) = 0;
  virtual void RestackBelow(Window window_id, Window sibiling_id) = 0;

  virtual void TerminateScale() = 0;
  virtual bool IsScaleActive() const = 0;
  virtual bool IsScaleActiveForGroup() const = 0;

  virtual void InitiateExpo() = 0;
  virtual void TerminateExpo() = 0;
  virtual bool IsExpoActive() const = 0;

  virtual bool IsWallActive() const = 0;

  virtual bool IsAnyWindowMoving() const = 0;

  virtual void FocusWindowGroup(std::vector<Window> const& windows,
                                FocusVisibility, int monitor = -1,
                                bool only_top_win = true) = 0;
  virtual bool ScaleWindowGroup(std::vector<Window> const& windows,
                                int state, bool force) = 0;

  virtual bool IsScreenGrabbed() const = 0;
  virtual bool IsViewPortSwitchStarted() const = 0;

  virtual void MoveResizeWindow(Window window_id, nux::Geometry geometry) = 0;
  virtual void StartMove(Window window_id, int x, int y) = 0;
  virtual void UnGrabMousePointer(Time, int button, int x, int y) = 0;

  virtual int GetWindowMonitor(Window window_id) const = 0;
  virtual nux::Geometry GetWindowGeometry(Window window_id) const = 0;
  virtual nux::Geometry GetWindowSavedGeometry(Window window_id) const = 0;
  virtual nux::Size GetWindowDecorationSize(Window window_id, Edge) const = 0;
  virtual nux::Geometry GetScreenGeometry() const = 0;
  virtual nux::Geometry GetWorkAreaGeometry(Window window_id = 0) const = 0;

  virtual uint64_t GetWindowActiveNumber(Window window_id) const = 0;

  virtual void SetWindowIconGeometry(Window window, nux::Geometry const& geo) = 0;

  virtual void CheckWindowIntersections (nux::Geometry const& region, bool &active, bool &any) = 0;

  virtual int WorkspaceCount() const = 0;

  virtual nux::Point GetCurrentViewport() const = 0;
  virtual void SetViewportSize(int horizontal, int vertical) = 0;
  virtual int GetViewportHSize() const = 0;
  virtual int GetViewportVSize() const = 0;

  virtual bool SaveInputFocus() = 0;
  virtual bool RestoreInputFocus() = 0;

  virtual std::string GetWindowName(Window window_id) const = 0;

  virtual std::string GetStringProperty(Window, Atom) const = 0;
  virtual std::vector<long> GetCardinalProperty(Window, Atom) const = 0;

  // Nux Modifiers, Nux Keycode (= X11 KeySym)
  nux::Property<std::pair<unsigned, unsigned>> close_window_key;
  nux::Property<nux::Color> average_color;

  // Signals
  sigc::signal<void, Window> window_mapped;
  sigc::signal<void, Window> window_unmapped;
  sigc::signal<void, Window> window_maximized;
  sigc::signal<void, Window> window_restored;
  sigc::signal<void, Window> window_minimized;
  sigc::signal<void, Window> window_unminimized;
  sigc::signal<void, Window> window_shaded;
  sigc::signal<void, Window> window_unshaded;
  sigc::signal<void, Window> window_shown;
  sigc::signal<void, Window> window_hidden;
  sigc::signal<void, Window> window_resized;
  sigc::signal<void, Window> window_moved;
  sigc::signal<void, Window> window_focus_changed;

  sigc::signal<void> initiate_spread;
  sigc::signal<void> terminate_spread;

  sigc::signal<void> initiate_expo;
  sigc::signal<void> terminate_expo;

  sigc::signal<void> screen_grabbed;
  sigc::signal<void> screen_ungrabbed;
  sigc::signal<void> screen_viewport_switch_started;
  sigc::signal<void> screen_viewport_switch_ended;
  sigc::signal<void, int, int> viewport_layout_changed;

protected:
  std::string GetName() const;
  virtual void AddProperties(debug::IntrospectionData& introspection) = 0;

};

}

#endif // UNITYSHARED_WINDOW_MANAGER_H
