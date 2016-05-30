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

namespace unity
{

FontSettings::FontSettings()
  : hintstyle_("gtk-xft-hintstyle")
  , rgba_("gtk-xft-rgba")
  , antialias_("gtk-xft-antialias")
{
  auto refresh_cb = sigc::hide(sigc::mem_fun(this, &FontSettings::Refresh));
  hintstyle_.changed.connect(refresh_cb);
  rgba_.changed.connect(refresh_cb);
  antialias_.changed.connect(refresh_cb);

  Refresh();
}

void FontSettings::Refresh()
{
  cairo_font_options_t* font_options = cairo_font_options_create();

  auto const& rgba = rgba_();
  cairo_subpixel_order_t order = CAIRO_SUBPIXEL_ORDER_DEFAULT;

  if (rgba == "rgb")
    order = CAIRO_SUBPIXEL_ORDER_RGB;
  else if (rgba == "bgr")
    order = CAIRO_SUBPIXEL_ORDER_BGR;
  else if (rgba == "vrgb")
    order = CAIRO_SUBPIXEL_ORDER_VRGB;
  else if (rgba == "vbgr")
    order = CAIRO_SUBPIXEL_ORDER_VBGR;

  cairo_font_options_set_subpixel_order(font_options, order);
  cairo_font_options_set_antialias(font_options,
                                   rgba == "none" ? (antialias_() ? CAIRO_ANTIALIAS_GRAY : CAIRO_ANTIALIAS_NONE)
                                                    : CAIRO_ANTIALIAS_SUBPIXEL);

  auto const& hintstyle = hintstyle_();
  cairo_hint_style_t style = CAIRO_HINT_STYLE_DEFAULT;

  if (hintstyle == "hintnone")
    style = CAIRO_HINT_STYLE_NONE;
  else if (hintstyle == "hintslight")
    style = CAIRO_HINT_STYLE_SLIGHT;
  else if (hintstyle == "hintmedium")
    style = CAIRO_HINT_STYLE_MEDIUM;
  else if (hintstyle == "hintfull")
    style = CAIRO_HINT_STYLE_FULL;

  cairo_font_options_set_hint_style(font_options, style);

  // FIXME: Where do we read this value from?
  cairo_font_options_set_hint_metrics(font_options, CAIRO_HINT_METRICS_ON);

  gdk_screen_set_font_options(gdk_screen_get_default(), font_options);
  cairo_font_options_destroy(font_options);
}

}
