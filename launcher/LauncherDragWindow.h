// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef LAUNCHERDRAGWINDOW_H
#define LAUNCHERDRAGWINDOW_H

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <UnityCore/GLibSource.h>

#include "LauncherIcon.h"

namespace unity
{
namespace launcher
{

class LauncherDragWindow : public nux::BaseWindow
{
  NUX_DECLARE_OBJECT_TYPE(LauncherDragWindow, nux::BaseWindow);
public:
  LauncherDragWindow(nux::ObjectPtr<nux::IOpenGLBaseTexture> icon);

  ~LauncherDragWindow();

  void DrawContent(nux::GraphicsEngine& gfxContext, bool forceDraw);

  void SetAnimationTarget(int x, int y);
  void StartAnimation();

  bool Animating();

  sigc::signal<void> anim_completed;
  sigc::connection on_anim_completed;

private:
  bool OnAnimationTimeout();

  nux::ObjectPtr<nux::IOpenGLBaseTexture> _icon;
  nux::Point2 _animation_target;
  glib::Source::UniquePtr animation_timer_;
};

} // namespace launcher
} // namespace unity

#endif // LAUNCHERDRAGWINDOW_H
