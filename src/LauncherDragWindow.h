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

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"
#include "NuxGraphics/GraphicsEngine.h"

#include "LauncherIcon.h"

class LauncherDragWindow : public nux::BaseWindow
{
  NUX_DECLARE_OBJECT_TYPE (LauncherDragWindow, nux::BaseWindow);
public:
  LauncherDragWindow (nux::IntrusiveSP<nux::IOpenGLBaseTexture> icon, int size);

  ~LauncherDragWindow ();

  void Draw (nux::GraphicsEngine& gfxContext,
    bool             forceDraw);

  void DrawContent (nux::GraphicsEngine& gfxContext,
    bool             forceDraw);

private:
  
  nux::IntrusiveSP<nux::IOpenGLBaseTexture> _icon;
  int _size;
  

};

#endif // LAUNCHERDRAGWINDOW_H

