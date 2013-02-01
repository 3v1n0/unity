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
 *              Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#ifndef UNITYSHARED_STANDALONE_WINDOW_MANAGER_H
#define UNITYSHARED_STANDALONE_WINDOW_MANAGER_H

#include "unity-shared/WindowManager.h"
#include <list>
#include <NuxCore/Property.h>

namespace unity
{

struct StandaloneWindow : sigc::trackable
{
  typedef std::shared_ptr<StandaloneWindow> Ptr;
  StandaloneWindow(Window xid);

private:
  Window xid;

public:
  Window Xid() const { return xid; }

  std::string name;
  nux::Property<nux::Geometry> geo;
  nux::Size deco_sizes[4];
  unsigned current_desktop;
  unsigned monitor;
  nux::Property<bool> active;
  nux::Property<bool> mapped;
  nux::Property<bool> visible;
  nux::Property<bool> maximized;
  nux::Property<bool> minimized;
  nux::Property<bool> decorated;
  nux::Property<bool> has_decorations;
  nux::Property<bool> on_top;
  nux::Property<bool> closable;
  nux::Property<bool> minimizable;
  nux::Property<bool> maximizable;

  sigc::signal<void> resized;
  sigc::signal<void> moved;
};

class StandaloneWindowManager : public WindowManager
{
public:
  StandaloneWindowManager();

  virtual Window GetActiveWindow() const;

  virtual bool IsWindowMaximized(Window window_id) const;
  virtual bool IsWindowDecorated(Window window_id) const;
  virtual bool IsWindowOnCurrentDesktop(Window window_id) const;
  virtual bool IsWindowObscured(Window window_id) const;
  virtual bool IsWindowMapped(Window window_id) const;
  virtual bool IsWindowVisible(Window window_id) const;
  virtual bool IsWindowOnTop(Window window_id) const;
  virtual bool IsWindowClosable(Window window_id) const;
  virtual bool IsWindowMinimized(Window window_id) const;
  virtual bool IsWindowMinimizable(Window window_id) const;
  virtual bool IsWindowMaximizable(Window window_id) const;
  virtual bool HasWindowDecorations(Window) const;

  virtual void ShowDesktop();
  virtual bool InShowDesktop() const;

  virtual void Maximize(Window window_id);
  virtual void Restore(Window window_id);
  virtual void RestoreAt(Window window_id, int x, int y);
  virtual void Minimize(Window window_id);
  virtual void UnMinimize(Window window_id);
  virtual void Close(Window window_id);

  virtual void Activate(Window window_id);
  virtual void Raise(Window window_id);
  virtual void Lower(Window window_id);
  void RestackBelow(Window window_id, Window sibiling_id) override;

  virtual void Decorate(Window window_id) const;
  virtual void Undecorate(Window window_id) const;

  virtual void TerminateScale();
  virtual bool IsScaleActive() const;
  virtual bool IsScaleActiveForGroup() const;

  virtual void InitiateExpo();
  virtual void TerminateExpo();
  virtual bool IsExpoActive() const;

  virtual bool IsWallActive() const;

  virtual void FocusWindowGroup(std::vector<Window> const& windows,
                                FocusVisibility, int monitor = -1, bool only_top_win = true);
  virtual bool ScaleWindowGroup(std::vector<Window> const& windows,
                                int state, bool force);

  virtual bool IsScreenGrabbed() const;
  virtual bool IsViewPortSwitchStarted() const;

  virtual void MoveResizeWindow(Window window_id, nux::Geometry geometry);
  virtual void StartMove(Window window_id, int x, int y);

  virtual int GetWindowMonitor(Window window_id) const;
  virtual nux::Geometry GetWindowGeometry(Window window_id) const;
  virtual nux::Geometry GetWindowSavedGeometry(Window window_id) const;
  virtual nux::Size GetWindowDecorationSize(Window window_id, Edge) const;
  virtual nux::Geometry GetScreenGeometry() const;
  virtual nux::Geometry GetWorkAreaGeometry(Window window_id) const;

  virtual unsigned long long GetWindowActiveNumber(Window window_id) const;

  virtual void SetWindowIconGeometry(Window window, nux::Geometry const& geo);

  virtual void CheckWindowIntersections (nux::Geometry const& region, bool &active, bool &any);

  virtual int WorkspaceCount() const;

  nux::Point GetCurrentViewport() const override;
  int GetViewportHSize() const override;
  int GetViewportVSize() const override;

  virtual bool SaveInputFocus();
  virtual bool RestoreInputFocus();

  virtual std::string GetWindowName(Window window_id) const;

  // Mock functions
  void AddStandaloneWindow(StandaloneWindow::Ptr const& window);
  std::list<StandaloneWindow::Ptr> GetStandaloneWindows() const;

  void SetScaleActive(bool scale_active);
  void SetScaleActiveForGroup(bool scale_active_for_group);
  void SetCurrentDesktop(unsigned desktop_id);

  void SetViewportSize(unsigned horizontal, unsigned vertical);
  void SetCurrentViewport(nux::Point const& vp);
  void SetWorkareaGeometry(nux::Geometry const& geo);

protected:
  virtual void AddProperties(GVariantBuilder* builder);

private:
  StandaloneWindow::Ptr GetWindowByXid(Window window_id) const;

  bool expo_state_;
  bool in_show_desktop_;
  bool scale_active_;
  bool scale_active_for_group_;
  unsigned current_desktop_;
  nux::Size viewport_size_;
  nux::Point current_vp_;
  nux::Geometry workarea_geo_;
  std::list<StandaloneWindow::Ptr> standalone_windows_;
};

}

#endif // UNITYSHARED_WINDOW_MANAGER_H
