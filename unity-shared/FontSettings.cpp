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

#include <UnityCore/GLibWrapper.h>
#include <sigc++/sigc++.h>

namespace unity
{

FontSettings::FontSettings()
{
  GtkSettings* settings = gtk_settings_get_default();

  sig_man_.Add<void, GtkSettings*, GParamSpec*>(settings, "notify::gtk-xft-hintstyle", sigc::mem_fun(this, &FontSettings::Refresh));
  sig_man_.Add<void, GtkSettings*, GParamSpec*>(settings, "notify::gtk-xft-rgba", sigc::mem_fun(this, &FontSettings::Refresh));
  sig_man_.Add<void, GtkSettings*, GParamSpec*>(settings, "notify::gtk-xft-antialias", sigc::mem_fun(this, &FontSettings::Refresh));

  Refresh();
}

void FontSettings::Refresh(GtkSettings* unused0, GParamSpec* unused1)
{
  GtkSettings* settings = gtk_settings_get_default();
  cairo_font_options_t* font_options = cairo_font_options_create();

  {
    cairo_subpixel_order_t order = CAIRO_SUBPIXEL_ORDER_DEFAULT;
    glib::String value;
    g_object_get(settings, "gtk-xft-rgba", &value, NULL);
    gint antialias;
    g_object_get(settings, "gtk-xft-antialias", &antialias, NULL);
    
    if (value.Str() == "rgb")
      order = CAIRO_SUBPIXEL_ORDER_RGB;
    else if (value.Str() == "bgr")
      order = CAIRO_SUBPIXEL_ORDER_BGR;
    else if (value.Str() == "vrgb")
      order = CAIRO_SUBPIXEL_ORDER_VRGB;
    else if (value.Str() == "vbgr")
      order = CAIRO_SUBPIXEL_ORDER_VBGR;

    cairo_font_options_set_subpixel_order(font_options, order);
    cairo_font_options_set_antialias(font_options,
                                     value.Str() == "none" ? (antialias ? CAIRO_ANTIALIAS_GRAY : CAIRO_ANTIALIAS_NONE) 
                                                           : CAIRO_ANTIALIAS_SUBPIXEL);

  }

  {
    cairo_hint_style_t style = CAIRO_HINT_STYLE_DEFAULT;
    glib::String value;
    g_object_get(settings, "gtk-xft-hintstyle", &value, NULL);
    
    if (value.Str() == "hintnone")
      style = CAIRO_HINT_STYLE_NONE;
    else if (value.Str() == "hintslight")
      style = CAIRO_HINT_STYLE_SLIGHT;
    else if (value.Str() == "hintmedium")
      style = CAIRO_HINT_STYLE_MEDIUM;
    else if (value.Str() == "hintfull")
      style = CAIRO_HINT_STYLE_FULL;

    cairo_font_options_set_hint_style(font_options, style);
  }
  
  // FIXME: Where do we read this value from?
  cairo_font_options_set_hint_metrics(font_options, CAIRO_HINT_METRICS_ON);

  gdk_screen_set_font_options(gdk_screen_get_default(), font_options);
  cairo_font_options_destroy(font_options);
}

}
