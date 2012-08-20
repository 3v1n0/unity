/*
 * Copyright (C) 2010 Canonical Ltd
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

#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <Nux/Nux.h>
#include <gdk/gdkx.h>
#include <core/core.h>

#include "unity-shared/Introspectable.h"

class WindowManager : public unity::debug::Introspectable
{
  // This is a glue interface that breaks the dependancy of Unity with Compiz
  // Basically it has a default implementation that does nothing useful, but
  // the idea is that unity.cpp uses SetDefault() early enough in it's
  // initialization so the things that require it get a usable implementation

public:
  WindowManager() :
    m_MoveResizeAtom(XInternAtom(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                                 "_NET_WM_MOVERESIZE", FALSE))
  {
  };

  enum class FocusVisibility
  {
    OnlyVisible,
    ForceUnminimizeInvisible,
    ForceUnminimizeOnCurrentDesktop
  };

  static WindowManager* Default();
  static void           SetDefault(WindowManager* manager);

  virtual guint32 GetActiveWindow() const = 0;

  virtual bool IsWindowMaximized(guint32 xid) const = 0;
  virtual bool IsWindowDecorated(guint32 xid) = 0;
  virtual bool IsWindowOnCurrentDesktop(guint32 xid) const = 0;
  virtual bool IsWindowObscured(guint32 xid) const = 0;
  virtual bool IsWindowMapped(guint32 xid) const = 0;
  virtual bool IsWindowVisible(guint32 xid) const = 0;
  virtual bool IsWindowOnTop(guint32 xid) const = 0;
  virtual bool IsWindowClosable(guint32 xid) const = 0;
  virtual bool IsWindowMinimizable(guint32 xid) const = 0;
  virtual bool IsWindowMaximizable(guint32 xid) const = 0;

  virtual void ShowDesktop() = 0;

  virtual void Restore(guint32 xid) = 0;
  virtual void RestoreAt(guint32 xid, int x, int y) = 0;
  virtual void Minimize(guint32 xid) = 0;
  virtual void Close(guint32 xid) = 0;

  virtual void Activate(guint32 xid) = 0;
  virtual void Raise(guint32 xid) = 0;
  virtual void Lower(guint32 xid) = 0;

  virtual void TerminateScale() = 0;
  virtual bool IsScaleActive() const = 0;
  virtual bool IsScaleActiveForGroup() const = 0;

  virtual void InitiateExpo() = 0;
  virtual bool IsExpoActive() const = 0;

  virtual bool IsWallActive() const = 0;

  virtual void FocusWindowGroup(std::vector<Window> windows, FocusVisibility, int monitor = -1, bool only_top_win = true) = 0;
  virtual bool ScaleWindowGroup(std::vector<Window> windows, int state, bool force) = 0;

  virtual void Decorate(guint32 xid) {};
  virtual void Undecorate(guint32 xid) {};

  virtual bool IsScreenGrabbed() const = 0;
  virtual bool IsViewPortSwitchStarted() const = 0;

  virtual void MoveResizeWindow(guint32 xid, nux::Geometry geometry) = 0;
  void StartMove(guint32 xid, int, int);

  virtual int GetWindowMonitor(guint32 xid) const = 0;
  virtual nux::Geometry GetWindowGeometry(guint32 xid) const = 0;
  virtual nux::Geometry GetWindowSavedGeometry(guint32 xid) const = 0;
  virtual nux::Geometry GetScreenGeometry() const = 0;
  virtual nux::Geometry GetWorkAreaGeometry(guint32 xid = 0) const = 0;

  virtual unsigned long long GetWindowActiveNumber(guint32 xid) const = 0;

  virtual void SetWindowIconGeometry(Window window, nux::Geometry const& geo) = 0;

  virtual void CheckWindowIntersections (nux::Geometry const& region, bool &active, bool &any) = 0;

  virtual int WorkspaceCount() const = 0;

  virtual bool saveInputFocus() = 0;
  virtual bool restoreInputFocus() = 0;

  // Signals
  sigc::signal<void, guint32> window_mapped;
  sigc::signal<void, guint32> window_unmapped;
  sigc::signal<void, guint32> window_maximized;
  sigc::signal<void, guint32> window_restored;
  sigc::signal<void, guint32> window_minimized;
  sigc::signal<void, guint32> window_unminimized;
  sigc::signal<void, guint32> window_shaded;
  sigc::signal<void, guint32> window_unshaded;
  sigc::signal<void, guint32> window_shown;
  sigc::signal<void, guint32> window_hidden;
  sigc::signal<void, guint32> window_resized;
  sigc::signal<void, guint32> window_moved;
  sigc::signal<void, guint32> window_focus_changed;
  sigc::signal<void, guint32> window_decorated;
  sigc::signal<void, guint32> window_undecorated;

  sigc::signal<void> initiate_spread;
  sigc::signal<void> terminate_spread;

  sigc::signal<void> initiate_expo;
  sigc::signal<void> terminate_expo;

  sigc::signal<void> compiz_screen_grabbed;
  sigc::signal<void> compiz_screen_ungrabbed;
  sigc::signal<void> compiz_screen_viewport_switch_started;
  sigc::signal<void> compiz_screen_viewport_switch_ended;

  sigc::signal<void, const char*, const char*, CompOption::Vector&> compiz_event;

protected:
  std::string GetName() const;
  virtual void AddProperties(GVariantBuilder* builder) = 0;

private:
  Atom m_MoveResizeAtom;
};

#endif // WINDOW_MANAGER_H
