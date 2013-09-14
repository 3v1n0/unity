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
 *              Sam Spilsbury <sam.spilsbury@canonical.com>
 *              Didier Roche <didier.roche@canonical.com>
 *              Marco Trevisan (Treviño) <mail@3v1n0.net>
 */

#include <Nux/Nux.h>

#include "PanelTitlebarGrabAreaView.h"

#include <UnityCore/Variant.h>
#include <X11/cursorfont.h>

namespace unity
{
namespace
{

const int MOUSE_DOWN_TIMEOUT = 150;
const int MOUSE_MOVEMENT_TOLERANCE = 4;

}

PanelTitlebarGrabArea::PanelTitlebarGrabArea()
  : InputArea(NUX_TRACKER_LOCATION)
  , grab_cursor_(None)
  , grab_started_(false)
  , mouse_down_button_(0)
{
  EnableDoubleClick(true);

  mouse_down.connect(sigc::mem_fun(this, &PanelTitlebarGrabArea::OnMouseDown));
  mouse_up.connect(sigc::mem_fun(this, &PanelTitlebarGrabArea::OnMouseUp));
  mouse_drag.connect(sigc::mem_fun(this, &PanelTitlebarGrabArea::OnGrabMove));

  mouse_double_click.connect([&] (int x, int y, unsigned long button_flags, unsigned long)
  {
    if (nux::GetEventButton(button_flags) == 1)
      restore_request.emit(x, y);
  });
}

PanelTitlebarGrabArea::~PanelTitlebarGrabArea()
{
  if (grab_cursor_)
    XFreeCursor(nux::GetGraphicsDisplay()->GetX11Display(), grab_cursor_);
}

void PanelTitlebarGrabArea::SetGrabbed(bool enabled)
{
  auto display = nux::GetGraphicsDisplay()->GetX11Display();
  auto panel_window = static_cast<nux::BaseWindow*>(GetTopLevelViewWindow());

  if (!panel_window || !display)
    return;

  if (enabled && !grab_cursor_)
  {
    grab_cursor_ = XCreateFontCursor(display, XC_fleur);
    XDefineCursor(display, panel_window->GetInputWindowId(), grab_cursor_);
  }
  else if (!enabled && grab_cursor_)
  {
    XUndefineCursor(display, panel_window->GetInputWindowId());
    XFreeCursor(display, grab_cursor_);
    grab_cursor_ = None;
  }
}

bool PanelTitlebarGrabArea::IsGrabbed()
{
  return (grab_cursor_ != None);
}

void PanelTitlebarGrabArea::OnMouseDown(int x, int y, unsigned long button_flags, unsigned long)
{
  mouse_down_button_ = nux::GetEventButton(button_flags);

  if (mouse_down_button_ == 2)
  {
    lower_request.emit(x, y);
  }
  else if (mouse_down_button_ == 1)
  {
    mouse_down_point_.x = x;
    mouse_down_point_.y = y;

    mouse_down_timer_.reset(new glib::Timeout(MOUSE_DOWN_TIMEOUT));
    mouse_down_timer_->Run([&] () {
      if (!grab_started_)
      {
        nux::Point const& mouse = nux::GetGraphicsDisplay()->GetMouseScreenCoord();
        nux::Geometry const& geo = GetAbsoluteGeometry();
        grab_started.emit(mouse.x - geo.x, mouse.y - geo.y);
        grab_started_ = true;
      }

      mouse_down_timer_.reset();
      return false;
    });
  }
}

void PanelTitlebarGrabArea::OnMouseUp(int x, int y, unsigned long button_flags, unsigned long)
{
  int button = nux::GetEventButton(button_flags);

  if (button == 1)
  {
    if (mouse_down_timer_)
    {
      mouse_down_timer_.reset();
      activate_request.emit(x, y);
    }

    if (grab_started_)
    {
      grab_end.emit(x, y);
      grab_started_ = false;
    }
  }

  mouse_down_button_ = 0;
  mouse_down_point_.x = 0;
  mouse_down_point_.y = 0;
}

void PanelTitlebarGrabArea::OnGrabMove(int x, int y, int, int, unsigned long button_flags, unsigned long)
{
  if (mouse_down_button_ != 1)
    return;

  if (mouse_down_timer_)
  {
    if (abs(mouse_down_point_.x - x) <= MOUSE_MOVEMENT_TOLERANCE &&
        abs(mouse_down_point_.y - y) <= MOUSE_MOVEMENT_TOLERANCE)
    {
      return;
    }

    mouse_down_timer_.reset();
  }

  if (!grab_started_)
  {
    grab_started.emit(x, y);
    grab_started_ = true;
  }
  else
  {
    grab_move.emit(x, y);
  }
}

std::string
PanelTitlebarGrabArea::GetName() const
{
  return "GrabArea";
}

void
PanelTitlebarGrabArea::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
  .add(GetAbsoluteGeometry())
  .add("grabbed", IsGrabbed());
}

}
