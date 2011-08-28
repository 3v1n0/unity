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

#include "Nux/Nux.h"
#include "Nux/BaseWindow.h"
#include "NuxGraphics/GraphicsEngine.h"
#include "Nux/TextureArea.h"

#include "LauncherDragWindow.h"

NUX_IMPLEMENT_OBJECT_TYPE(LauncherDragWindow);

LauncherDragWindow::LauncherDragWindow(nux::ObjectPtr<nux::IOpenGLBaseTexture> icon)
  : nux::BaseWindow("")
{
  _icon = icon;
  _anim_handle = 0;
  SetBaseSize(_icon->GetWidth(), _icon->GetHeight());
}

LauncherDragWindow::~LauncherDragWindow()
{
  if (_anim_handle)
    g_source_remove(_anim_handle);

  if (on_anim_completed.connected())
    on_anim_completed.disconnect();
}

bool
LauncherDragWindow::Animating()
{
  return _anim_handle != 0;
}

void
LauncherDragWindow::SetAnimationTarget(int x, int y)
{
  _animation_target = nux::Point2(x, y);
}

void
LauncherDragWindow::StartAnimation()
{
  if (_anim_handle)
    return;

  _anim_handle = g_timeout_add(15, &LauncherDragWindow::OnAnimationTimeout, this);
}

gboolean
LauncherDragWindow::OnAnimationTimeout(gpointer data)
{
  LauncherDragWindow* self = (LauncherDragWindow*) data;
  nux::Geometry geo = self->GetGeometry();

  int half_size = geo.width / 2;

  int target_x = (int)(self->_animation_target.x) - half_size;
  int target_y = (int)(self->_animation_target.y) - half_size;

  int x_delta = (int)((float)(target_x - geo.x) * .3f);
  if (abs(x_delta) < 5)
    x_delta = (x_delta >= 0) ? MIN(5, target_x - geo.x) : MAX(-5, target_x - geo.x);

  int y_delta = (int)((float)(target_y - geo.y) * .3f);
  if (abs(y_delta) < 5)
    y_delta = (y_delta >= 0) ? MIN(5, target_y - geo.y) : MAX(-5, target_y - geo.y);

  self->SetBaseXY(geo.x + x_delta, geo.y + y_delta);

  geo = self->GetGeometry();

  if (geo.x == target_x && geo.y == target_y)
  {
    self->anim_completed.emit();
    self->_anim_handle = 0;
    return false;
  }

  return true;
}

void
LauncherDragWindow::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry();
  geo.SetX(0);
  geo.SetY(0);

  GfxContext.PushClippingRectangle(geo);

  nux::TexCoordXForm texxform;
  texxform.FlipVCoord(true);

  GfxContext.QRP_1Tex(0,
                      0,
                      _icon->GetWidth(),
                      _icon->GetHeight(),
                      _icon,
                      texxform,
                      nux::color::White);

  GfxContext.PopClippingRectangle();
}
