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

#include "config.h"

#include <math.h>
#include <gtk/gtk.h>

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <NuxImage/CairoGraphics.h>

#include <UnityCore/GLibWrapper.h>
#include "PlacesStyle.h"

namespace
{
nux::logging::Logger logger("unity.places");
}

static PlacesStyle* _style = NULL;

PlacesStyle::PlacesStyle()
  : _util_cg(CAIRO_FORMAT_ARGB32, 1, 1),
    _text_color(1.0f, 1.0f, 1.0f, 1.0f),
    _text_width(0),
    _text_height(0),
    _n_cols(6),
    _dash_bottom_texture(NULL),
    _dash_right_texture(NULL),
    _dash_corner_texture(NULL),
    _dash_fullscreen_icon(NULL),
    _search_magnify_texture(NULL),
    _search_close_texture(NULL),
    _search_close_glow_texture(NULL),
    _search_spin_texture(NULL),
    _search_spin_glow_texture(NULL),
    _group_unexpand_texture(NULL),
    _group_expand_texture(NULL)
{
  g_signal_connect(gtk_settings_get_default(), "notify::gtk-font-name",
                   G_CALLBACK(PlacesStyle::OnFontChanged), this);
  g_signal_connect(gtk_settings_get_default(), "notify::gtk-xft-dpi",
                   G_CALLBACK(PlacesStyle::OnFontChanged), this);

  Refresh();
}

PlacesStyle::~PlacesStyle()
{
  if (_dash_bottom_texture)
    _dash_bottom_texture->UnReference();
  if (_dash_right_texture)
    _dash_right_texture->UnReference();
  if (_dash_corner_texture)
    _dash_corner_texture->UnReference();
  if (_dash_fullscreen_icon)
    _dash_fullscreen_icon->UnReference();
  if (_search_magnify_texture)
    _search_magnify_texture->UnReference();
  if (_search_close_texture)
    _search_close_texture->UnReference();
  if (_search_close_glow_texture)
    _search_close_glow_texture->UnReference();
  if (_search_spin_texture)
    _search_spin_texture->UnReference();
  if (_group_unexpand_texture)
    _group_unexpand_texture->UnReference();
  if (_group_expand_texture)
    _group_expand_texture->UnReference();

  if (_style == this)
    _style = NULL;
}

PlacesStyle* PlacesStyle::GetDefault()
{
  if (G_UNLIKELY(!_style))
    _style = new PlacesStyle();

  return _style;
}

int PlacesStyle::GetDefaultNColumns()
{
  return _n_cols;
}

void PlacesStyle::SetDefaultNColumns(int n_cols)
{
  if (_n_cols == n_cols)
    return;

  _n_cols = n_cols;

  columns_changed.emit();
}

int PlacesStyle::GetTileIconSize()
{
  return 48;
}

int PlacesStyle::GetTileWidth()
{
  return _text_width;
}

int PlacesStyle::GetTileHeight()
{
  return GetTileIconSize() + (_text_height * 4);
}

int PlacesStyle::GetHomeTileIconSize()
{
  return 104;
}

int PlacesStyle::GetHomeTileWidth()
{
  return _text_width * 1.2;
}

int PlacesStyle::GetHomeTileHeight()
{
  return GetHomeTileIconSize() + (_text_height * 4);
}


nux::BaseTexture* PlacesStyle::GetDashBottomTile()
{
  if (!_dash_bottom_texture)
    _dash_bottom_texture = TextureFromFilename(PKGDATADIR"/dash_bottom_border_tile.png");
  return _dash_bottom_texture;
}

nux::BaseTexture* PlacesStyle::GetDashRightTile()
{
  if (!_dash_right_texture)
    _dash_right_texture =  TextureFromFilename(PKGDATADIR"/dash_right_border_tile.png");
  return _dash_right_texture;
}

nux::BaseTexture* PlacesStyle::GetDashCorner()
{
  if (!_dash_corner_texture)
    _dash_corner_texture =  TextureFromFilename(PKGDATADIR"/dash_bottom_right_corner.png");
  return _dash_corner_texture;
}

nux::BaseTexture* PlacesStyle::GetDashFullscreenIcon()
{
  if (!_dash_fullscreen_icon)
    _dash_fullscreen_icon = TextureFromFilename(PKGDATADIR"/dash_fullscreen_icon.png");
  return _dash_fullscreen_icon;
}

nux::BaseTexture* PlacesStyle::GetSearchMagnifyIcon()
{
  if (!_search_magnify_texture)
    _search_magnify_texture = TextureFromFilename(PKGDATADIR"/search_magnify.png");
  return _search_magnify_texture;
}

nux::BaseTexture* PlacesStyle::GetSearchCloseIcon()
{
  if (!_search_close_texture)
    _search_close_texture = TextureFromFilename(PKGDATADIR"/search_close.png");
  return _search_close_texture;
}

nux::BaseTexture* PlacesStyle::GetSearchCloseGlowIcon()
{
  if (!_search_close_glow_texture)
    _search_close_glow_texture = TextureFromFilename(PKGDATADIR"/search_close_glow.png");
  return _search_close_glow_texture;
}

nux::BaseTexture* PlacesStyle::GetSearchSpinIcon()
{
  if (!_search_spin_texture)
    _search_spin_texture = TextureFromFilename(PKGDATADIR"/search_spin.png");
  return _search_spin_texture;
}

nux::BaseTexture* PlacesStyle::GetSearchSpinGlowIcon()
{
  if (!_search_spin_glow_texture)
    _search_spin_glow_texture = TextureFromFilename(PKGDATADIR"/search_spin_glow.png");
  return _search_spin_glow_texture;
}

nux::BaseTexture* PlacesStyle::GetGroupUnexpandIcon()
{
  if (!_group_unexpand_texture)
    _group_unexpand_texture = TextureFromFilename(PKGDATADIR"/dash_group_unexpand.png");
  return _group_unexpand_texture;
}

nux::BaseTexture* PlacesStyle::GetGroupExpandIcon()
{
  if (!_group_expand_texture)
    _group_expand_texture = TextureFromFilename(PKGDATADIR"/dash_group_expand.png");
  return _group_expand_texture;
}

nux::BaseTexture* PlacesStyle::TextureFromFilename(const char* filename)
{
  glib::Object<GdkPixbuf> pixbuf;
  glib::Error error;
  nux::BaseTexture* texture = NULL;

  pixbuf = gdk_pixbuf_new_from_file(filename, &error);
  if (error)
  {
    LOG_WARN(logger) << "Unable to texture " << filename << ": " << error;
  }
  else
  {
    texture = nux::CreateTexture2DFromPixbuf(pixbuf, true);
    texture->Reference();  // stop it getting unreffed by IconTexture
  }

  return texture;
}

void PlacesStyle::Refresh()
{
#define _TEXT_ "Chromium Web Browser"
  PangoLayout*          layout = NULL;
  PangoFontDescription* desc = NULL;
  GtkSettings*          settings = gtk_settings_get_default();
  cairo_t*              cr;
  char*                 font_description = NULL;
  PangoContext*         cxt;
  PangoRectangle        log_rect;
  GdkScreen*            screen = gdk_screen_get_default();
  int                   dpi = 0;

  cr = _util_cg.GetContext();

  g_object_get(settings,
               "gtk-font-name", &font_description,
               "gtk-xft-dpi", &dpi,
               NULL);
  desc = pango_font_description_from_string(font_description);
  pango_font_description_set_weight(desc, PANGO_WEIGHT_NORMAL);

  layout = pango_cairo_create_layout(cr);
  pango_layout_set_font_description(layout, desc);
  pango_layout_set_text(layout, _TEXT_, -1);

  cxt = pango_layout_get_context(layout);
  pango_cairo_context_set_font_options(cxt, gdk_screen_get_font_options(screen));
  pango_cairo_context_set_resolution(cxt, (float)dpi / (float)PANGO_SCALE);
  pango_layout_context_changed(layout);

  pango_layout_get_extents(layout, NULL, &log_rect);
  _text_width = log_rect.width / PANGO_SCALE;
  _text_height = log_rect.height / PANGO_SCALE;

  changed.emit();

  pango_font_description_free(desc);
  g_free(font_description);
  cairo_destroy(cr);
}

nux::Color& PlacesStyle::GetTextColor()
{
  return _text_color;
}

void PlacesStyle::OnFontChanged(GObject* object, GParamSpec* pspec, PlacesStyle* self)
{
  self->Refresh();
}
