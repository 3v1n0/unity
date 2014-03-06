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

#include <Nux/Nux.h>
#include "BackgroundSettings.h"

#include <libgnome-desktop/gnome-bg.h>

#include "LockScreenSettings.h"
#include "unity-shared/CairoTexture.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UScreen.h"

namespace unity
{
namespace lockscreen
{
namespace
{
const std::string SETTINGS_NAME = "org.gnome.desktop.background";
const std::string GREETER_SETTINGS = "com.canonical.unity-greeter";
const std::string LOGO_KEY = "logo";

constexpr int GetGridOffset(int size) { return (size % Settings::GRID_SIZE) / 2; }
}

BackgroundSettings::BackgroundSettings()
  : gnome_bg_(gnome_bg_new())
{
  glib::Object<GSettings> settings(g_settings_new(SETTINGS_NAME.c_str()));
  gnome_bg_load_from_preferences(gnome_bg_, settings);
}

BackgroundSettings::~BackgroundSettings()
{}

BaseTexturePtr BackgroundSettings::GetBackgroundTexture(int monitor, bool draw_grid, bool draw_logo)
{
  nux::Geometry geo = UScreen::GetDefault()->GetMonitorGeometry(monitor);

  nux::CairoGraphics cairo_graphics(CAIRO_FORMAT_ARGB32, geo.width, geo.height);
  cairo_t* c = cairo_graphics.GetInternalContext();

  cairo_surface_t* bg_surface = gnome_bg_create_surface(gnome_bg_, gdk_get_default_root_window(), geo.width, geo.height, FALSE);
  cairo_set_source_surface(c, bg_surface, 0, 0);
  cairo_paint(c);
  cairo_surface_destroy(bg_surface);

  if (draw_logo)
  {
    int grid_x_offset = GetGridOffset(geo.width);
    int grid_y_offset = GetGridOffset(geo.height) + panel::Style::Instance().PanelHeight(monitor);

    cairo_save(c);

    glib::Object<GSettings> greeter_settings(g_settings_new(GREETER_SETTINGS.c_str()));
    glib::String logo(g_settings_get_string(greeter_settings, LOGO_KEY.c_str()));
    cairo_surface_t* logo_surface = cairo_image_surface_create_from_png(logo);

    int height = cairo_image_surface_get_height(logo_surface);
    int x = grid_x_offset;
    int y = grid_y_offset + Settings::GRID_SIZE * (geo.height / Settings::GRID_SIZE - 1) - height;
    cairo_translate (c, x, y);

    cairo_set_source_surface(c, logo_surface, 0, 0);
    cairo_paint_with_alpha(c, 0.5);
    cairo_surface_destroy(logo_surface);
    cairo_restore(c);
  }

  if (draw_grid)
  {
    int width = geo.width;
    int height = geo.height;
    int grid_x_offset = GetGridOffset(width);
    int grid_y_offset = GetGridOffset(height) + panel::Style::Instance().PanelHeight(monitor);

    // overlay grid
    cairo_surface_t* overlay_surface = cairo_surface_create_similar(cairo_graphics.GetSurface(),
                                                                    CAIRO_CONTENT_COLOR_ALPHA,
                                                                    Settings::GRID_SIZE,
                                                                    Settings::GRID_SIZE);

    cairo_t* oc = cairo_create(overlay_surface);
    cairo_rectangle(oc, 0, 0, 1, 1);
    cairo_rectangle(oc, Settings::GRID_SIZE - 1, 0, 1, 1);
    cairo_rectangle(oc, 0, Settings::GRID_SIZE - 1, 1, 1);
    cairo_rectangle(oc, Settings::GRID_SIZE - 1, Settings::GRID_SIZE - 1, 1, 1);
    cairo_set_source_rgba(oc, 1.0, 1.0, 1.0, 0.25);
    cairo_fill(oc);

    cairo_pattern_t* overlay = cairo_pattern_create_for_surface(overlay_surface);

    cairo_matrix_t matrix;
    cairo_matrix_init_identity(&matrix);
    cairo_matrix_translate(&matrix, -grid_x_offset, -grid_y_offset);

    cairo_pattern_set_matrix(overlay, &matrix);
    cairo_pattern_set_extend(overlay, CAIRO_EXTEND_REPEAT);

    cairo_save(c);
    cairo_set_source(c, overlay);
    cairo_rectangle(c, 0, 0, width, height);
    cairo_fill(c);
    cairo_restore(c);

    cairo_pattern_destroy(overlay);
    cairo_surface_destroy(overlay_surface);
    cairo_destroy(oc);
  }

  return texture_ptr_from_cairo_graphics(cairo_graphics);
}

} // lockscreen
} // unity
