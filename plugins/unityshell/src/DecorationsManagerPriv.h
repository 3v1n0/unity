// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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

#ifndef UNITY_DECORATION_MANAGER_PRIV
#define UNITY_DECORATION_MANAGER_PRIV

#include "DecorationsManager.h"
#include "unityshell.h"
#include <X11/Xlib.h>

class CompRegion;

namespace unity
{
namespace decoration
{

struct Window::Impl
{
  Impl(decoration::Window*, UnityWindow*);
  ~Impl();

  void Update();
  bool FullyDecorated() const;

private:
  void UnsetExtents();
  void SetupExtents();
  void UnsetFrame();
  void UpdateFrame();
  void SyncXShapeWithFrameRegion();
  bool ShouldBeDecorated() const;

  friend class Window;
  friend struct Manager::Impl;

  decoration::Window *parent_;
  ::UnityWindow* uwin_;
  ::Window frame_;
  bool undecorated_;

  nux::Geometry frame_geo_;
  CompRegion frame_region_;
};

struct Manager::Impl
{
  Impl(UnityScreen*);
  ~Impl();

private:
  friend class Manager;
  friend struct Window::Impl;

  bool HandleEventBefore(XEvent*);
  bool HandleEventAfter(XEvent*);

  Window::Ptr GetWindowByXid(::Window);
  Window::Ptr GetWindowByFrame(::Window);

  bool UpdateWindow(::Window);

  ::UnityScreen* uscreen_;
  ::CompScreen* cscreen_;
  ::Window active_window_;

  std::map<UnityWindow*, decoration::Window::Ptr> windows_;
};

} // decoration namespace
} // unity namespace

#endif
