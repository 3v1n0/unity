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
*              Marco Trevisan <marco.trevisan@canonical.com>
*/

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/TextureArea.h>

#include "LauncherDragWindow.h"
#include "unity-shared/WindowManager.h"

namespace unity
{
namespace launcher
{

NUX_IMPLEMENT_OBJECT_TYPE(LauncherDragWindow);

LauncherDragWindow::LauncherDragWindow(nux::ObjectPtr<nux::IOpenGLBaseTexture> icon)
  : nux::BaseWindow("")
  , animation_speed_(0.3f)
  , _cancelled(false)
  , _icon(icon)
{
  SetBaseSize(_icon->GetWidth(), _icon->GetHeight());

  key_down.connect([this] (unsigned long, unsigned long keysym, unsigned long, const char*, unsigned short) {
    if (keysym == NUX_VK_ESCAPE)
      CancelDrag();
  });

  auto wm = WindowManager::Default();
  wm->window_mapped.connect(sigc::hide(sigc::mem_fun(this, &LauncherDragWindow::CancelDrag)));
  wm->window_unmapped.connect(sigc::hide(sigc::mem_fun(this, &LauncherDragWindow::CancelDrag)));
}

LauncherDragWindow::~LauncherDragWindow()
{
  if (on_anim_completed.connected())
    on_anim_completed.disconnect();

  UnGrabKeyboard();
}

bool LauncherDragWindow::Animating() const
{
  return bool(animation_timer_);
}

bool LauncherDragWindow::Cancelled() const
{
  return _cancelled;
}

void LauncherDragWindow::CancelDrag()
{
  _cancelled = true;
  drag_cancel_request.emit();
}

void LauncherDragWindow::SetAnimationTarget(int x, int y)
{
  _animation_target = nux::Point2(x, y);
}

void LauncherDragWindow::StartQuickAnimation()
{
  animation_speed_ = 0.3f;
  StartAnimation();
}

void LauncherDragWindow::StartSlowAnimation()
{
  animation_speed_ = 0.05f;
  StartAnimation();
}

void LauncherDragWindow::StartAnimation()
{
  if (animation_timer_)
    return;


  animation_timer_.reset(new glib::Timeout(15));
  animation_timer_->Run(sigc::mem_fun(this, &LauncherDragWindow::OnAnimationTimeout));
}

bool LauncherDragWindow::OnAnimationTimeout()
{ 
  nux::Geometry const& geo = GetGeometry();

  int half_size = geo.width / 2;

  int target_x = static_cast<int>(_animation_target.x) - half_size;
  int target_y = static_cast<int>(_animation_target.y) - half_size;

  int x_delta = static_cast<int>(static_cast<float>(target_x - geo.x) * animation_speed_);
  if (std::abs(x_delta) < 5)
    x_delta = (x_delta >= 0) ? std::min(5, target_x - geo.x) : std::max(-5, target_x - geo.x);

  int y_delta = static_cast<int>(static_cast<float>(target_y - geo.y) * animation_speed_);
  if (std::abs(y_delta) < 5)
    y_delta = (y_delta >= 0) ? std::min(5, target_y - geo.y) : std::max(-5, target_y - geo.y);

  SetBaseXY(geo.x + x_delta, geo.y + y_delta);

  nux::Geometry const& new_geo = GetGeometry();

  if (new_geo.x == target_x && new_geo.y == target_y)
  {
    animation_timer_.reset();
    anim_completed.emit();

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

bool LauncherDragWindow::InspectKeyEvent(unsigned int event_type, unsigned int keysym, const char* character)
{
  return (event_type == nux::NUX_KEYDOWN);
}

bool LauncherDragWindow::AcceptKeyNavFocus()
{
  return true;
}

} // namespace launcher
} // namespace unity
