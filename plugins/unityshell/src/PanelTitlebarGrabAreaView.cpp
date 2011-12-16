// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2011 Canonical Ltd
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

PanelTitlebarGrabArea::PanelTitlebarGrabArea()
  : InputArea(NUX_TRACKER_LOCATION)
  , _grab_cursor(None)
{
  EnableDoubleClick(true);
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
  return (_grab_cursor != None);
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
