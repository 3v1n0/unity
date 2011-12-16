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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Sam Spilsbury <sam.spilsbury@canonical.com>
 *              Didier Roche <didier.roche@canonical.com>
 *              Marco Trevisan (Treviño) <mail@3v1n0.net>
 */

#include <Nux/Nux.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/Button.h>

#include <NuxGraphics/GLThread.h>
#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

#include "PanelTitlebarGrabAreaView.h"
#include <UnityCore/Variant.h>

#include <glib.h>
#include <X11/cursorfont.h>

#define DELTA_MOUSE_DOUBLE_CLICK 500000000

PanelTitlebarGrabArea::PanelTitlebarGrabArea()
  : InputArea(NUX_TRACKER_LOCATION)
  , _grab_cursor(None)
{
  // FIXME: the two following functions should be used instead of the insane trick with fixed value. But nux is broken
  // right now and we need jay to focus on other things
  /*InputArea::EnableDoubleClick (true);
  InputArea::OnMouseDoubleClick.connect (sigc::mem_fun (this, &PanelTitlebarGrabArea::RecvMouseDoubleClick));*/
  InputArea::mouse_up.connect(sigc::mem_fun(this, &PanelTitlebarGrabArea::RecvMouseUp));
  _last_click_time.tv_sec = 0;
  _last_click_time.tv_nsec = 0;
}


PanelTitlebarGrabArea::~PanelTitlebarGrabArea()
{
  if (_grab_cursor)
    XFreeCursor(nux::GetGraphicsDisplay()->GetX11Display(), _grab_cursor);
}

void PanelTitlebarGrabArea::SetGrabbed(bool enabled)
{
  auto display = nux::GetGraphicsDisplay()->GetX11Display();
  auto panel_window = static_cast<nux::BaseWindow*>(GetTopLevelViewWindow());

  if (!panel_window || !display)
    return;

  if (enabled && !_grab_cursor)
  {
    _grab_cursor = XCreateFontCursor(display, XC_fleur);
    XDefineCursor(display, panel_window->GetInputWindowId(), _grab_cursor);
  }
  else if (!enabled && _grab_cursor)
  {
    XUndefineCursor(display, panel_window->GetInputWindowId());
    XFreeCursor(display, _grab_cursor);
    _grab_cursor = None;
  }
}

bool PanelTitlebarGrabArea::IsGrabbed()
{
  return _grab_cursor != None;
}

void PanelTitlebarGrabArea::RecvMouseDoubleClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  mouse_double_click.emit(x, y, button_flags, key_flags);
}

// TODO: can be safely removed once OnMouseDoubleClick is fixed in nux
void PanelTitlebarGrabArea::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  struct timespec event_time, delta;
  clock_gettime(CLOCK_MONOTONIC, &event_time);
  delta = time_diff(_last_click_time, event_time);

  _last_click_time.tv_sec = event_time.tv_sec;
  _last_click_time.tv_nsec = event_time.tv_nsec;

  if ((delta.tv_sec == 0) && (delta.tv_nsec < DELTA_MOUSE_DOUBLE_CLICK))
    RecvMouseDoubleClick(x, y, button_flags, key_flags);
}

struct timespec PanelTitlebarGrabArea::time_diff(struct timespec start, struct timespec end)
{
  struct timespec temp;
  if ((end.tv_nsec - start.tv_nsec) < 0)
  {
    temp.tv_sec = end.tv_sec - start.tv_sec - 1;
    temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
  }
  else
  {
    temp.tv_sec = end.tv_sec - start.tv_sec;
    temp.tv_nsec = end.tv_nsec - start.tv_nsec;
  }
  return temp;
}

std::string
PanelTitlebarGrabArea::GetName() const
{
  return "panel-titlebar-grab-area";
}

void
PanelTitlebarGrabArea::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder).add(GetGeometry());
}
