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
 * Authored by: Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#include "Nux/Nux.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include "Nux/Button.h"

#include "NuxGraphics/GLThread.h"
#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "PanelTitlebarGrabAreaView.h"

#include <glib.h>

enum
{
  BUTTON_CLOSE=0,
  BUTTON_MINIMISE,
  BUTTON_UNMAXIMISE
};


PanelTitlebarGrabArea::PanelTitlebarGrabArea ()
: InputArea (NUX_TRACKER_LOCATION)
{
  InputArea::OnMouseDown.connect (sigc::mem_fun (this, &PanelTitlebarGrabArea::RecvMouseDown));
}


PanelTitlebarGrabArea::~PanelTitlebarGrabArea ()
{
}

const gchar *
PanelTitlebarGrabArea::GetName ()
{
  return "panel-titlebar-grab-area";
}

const gchar *
PanelTitlebarGrabArea::GetChildsName ()
{
  return "";
}

void
PanelTitlebarGrabArea::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  /* Now some props from ourselves */
  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}

void PanelTitlebarGrabArea::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  mouse_down.emit (x, y);
}


