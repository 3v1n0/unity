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

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"
#include "NuxGraphics/GraphicsEngine.h"
#include "Nux/TextureArea.h"

#include "LauncherDragWindow.h"

NUX_IMPLEMENT_OBJECT_TYPE (LauncherDragWindow);

LauncherDragWindow::LauncherDragWindow (nux::IntrusiveSP<nux::IOpenGLBaseTexture> icon)
: nux::BaseWindow ("")
{
  _icon = icon;
  SetBaseSize (_icon->GetWidth(), _icon->GetHeight());
}

LauncherDragWindow::~LauncherDragWindow ()
{
}

void LauncherDragWindow::Draw (nux::GraphicsEngine& GfxContext, bool forceDraw)
{
}

void LauncherDragWindow::DrawContent (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry ();
  geo.SetX (0);
  geo.SetY (0);
  
  GfxContext.PushClippingRectangle (geo);
  
  nux::TexCoordXForm texxform;
  GfxContext.QRP_GLSL_1Tex (0,
                            0,
                            _icon->GetWidth(),
                            _icon->GetHeight(),
                            _icon,
                            texxform,
                            nux::Color::White);
  
  GfxContext.PopClippingRectangle ();
}
