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
 */

#include "Nux/Nux.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"
#include "Nux/Button.h"

#include "NuxGraphics/GLThread.h"
#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "WindowButtons.h"

#include <glib.h>

class WindowButton : public nux::Button
{
public:
  WindowButton ()
  : nux::Button ("X", NUX_TRACKER_LOCATION)
  {

  }

  ~WindowButton ()
  {

  }
};

WindowButtons::WindowButtons ()
{
  WindowButton *but;

  but = new WindowButton ();
  AddView (but, 0, nux::eCenter, nux::eFull);

  but = new WindowButton ();
  AddView (but, 0, nux::eCenter, nux::eFull);

  but = new WindowButton ();
  AddView (but, 0, nux::eCenter, nux::eFull);
  
  SetContentDistribution (nux::eStackLeft);
}


WindowButtons::~WindowButtons ()
{
}

const gchar *
WindowButtons::GetName ()
{
  return "window-buttons";
}

const gchar *
WindowButtons::GetChildsName ()
{
  return "";
}

void
WindowButtons::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  /* Now some props from ourselves */
  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}
