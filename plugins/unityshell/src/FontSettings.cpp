/*
 * Copyright (C) 2011 Canonical Ltd
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

#include "FontSettings.h"

#include <cairo/cairo.h>
#include <gtk/gtk.h>


namespace unity
{

FontSettings::FontSettings()
{
  cairo_font_options_t* font_options = cairo_font_options_create();

  cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_DEFAULT);
  cairo_font_options_set_subpixel_order(font_options, CAIRO_SUBPIXEL_ORDER_DEFAULT);
  cairo_font_options_set_hint_style(font_options, CAIRO_HINT_STYLE_DEFAULT);
  cairo_font_options_set_hint_metrics(font_options, CAIRO_HINT_METRICS_ON);

  gdk_screen_set_font_options(gdk_screen_get_default(), font_options);
  cairo_font_options_destroy(font_options);
}

}
