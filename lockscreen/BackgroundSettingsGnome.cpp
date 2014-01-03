// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include "BackgroundSettingsGnome.h"

#include <libgnome-desktop/gnome-bg.h>

#include "unity-shared/GtkTexture.h"

namespace unity 
{
namespace lockscreen
{
namespace
{
const std::string SETTINGS_NAME = "org.gnome.desktop.background";
}

BackgroundSettingsGnome::BackgroundSettingsGnome()
  : gnome_bg_(gnome_bg_new())
  , settings_(g_settings_new(SETTINGS_NAME.c_str()))
{
  gnome_bg_load_from_preferences(gnome_bg_, settings_);

  // FIXME: does not work. Investigate!
  bg_changed_.Connect(gnome_bg_, "changed", [this] (GnomeBG*) {
    bg_changed.emit();
  });
}

BaseTexturePtr BackgroundSettingsGnome::GetBackgroundTexture(nux::Size const& size)
{
  cairo_surface_t* cairo_surface = gnome_bg_create_surface(gnome_bg_, gdk_get_default_root_window(),
                                                           size.width, size.height, FALSE);

  GdkPixbuf* gdk_pixbuf = gdk_pixbuf_get_from_surface(cairo_surface, 0, 0, 
                                                      size.width, size.height);

  return texture_ptr_from_gdk_graphics(nux::GdkGraphics(gdk_pixbuf));
}

} // lockscreen
} // unity