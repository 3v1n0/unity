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
 */

#ifndef UNITYSHARED_STANDALONE_WINDOW_MANAGER_H
#define UNITYSHARED_STANDALONE_WINDOW_MANAGER_H

#include "unity-shared/WindowManager.h"

namespace unity
{

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
  virtual bool IsWindowMinimizable(Window window_id) const;
  virtual bool IsWindowMaximizable(Window window_id) const;
  virtual bool HasWindowDecorations(Window) const;

  virtual void ShowDesktop();
  virtual bool InShowDesktop() const;

  virtual void Restore(Window window_id);
  virtual void RestoreAt(Window window_id, int x, int y);
  virtual void Minimize(Window window_id);
  virtual void Close(Window window_id);

  virtual void Activate(Window window_id);
  virtual void Raise(Window window_id);
  virtual void Lower(Window window_id);

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

  virtual bool SaveInputFocus();
  virtual bool RestoreInputFocus();

  virtual std::string GetWindowName(Window window_id) const;

protected:
  virtual void AddProperties(GVariantBuilder* builder);

private:
  bool expo_state_;
  bool in_show_desktop_;
};

}

#endif // UNITYSHARED_WINDOW_MANAGER_H
