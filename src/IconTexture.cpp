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
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */

#include "config.h"

#include "Nux/Nux.h"
#include "NuxGraphics/GLThread.h"
#include "IconLoader.h"
#include "IconTexture.h"
#include "TextureCache.h"

#include <glib.h>
#include <pango/pangocairo.h>
#include <gtk/gtk.h>

#define DEFAULT_ICON "text-x-preview"

IconTexture::IconTexture (const char *icon_name, unsigned int size, bool defer_icon_loading)
: TextureArea (NUX_TRACKER_LOCATION),
  _icon_name (NULL),
  _size (size),
  _texture_cached (NULL),
  _texture_width (0),
  _texture_height (0),
  _loading (false)
{
  _icon_name = g_strdup (icon_name ? icon_name : DEFAULT_ICON);

  if (!g_strcmp0 (_icon_name, "") == 0 && !defer_icon_loading)
    LoadIcon ();

  _can_pass_focus_to_composite_layout = false;
}

IconTexture::~IconTexture ()
{
  g_free (_icon_name);

  if (_texture_cached)
    _texture_cached->UnReference ();
}

void
IconTexture::SetByIconName (const char *icon_name, unsigned int size)
{
  g_free (_icon_name);
  _icon_name = g_strdup (icon_name);
  _size = size;
  LoadIcon ();
}

void
IconTexture::SetByFilePath (const char *file_path, unsigned int size)
{
  g_free (_icon_name);
  _icon_name = g_strdup (file_path);
  _size = size;

  LoadIcon ();
}

void
IconTexture::LoadIcon ()
{
  GIcon  *icon;

  if (_loading)
    return;
  _loading = true;

  icon = g_icon_new_for_string (_icon_name, NULL);

  if (G_IS_ICON (icon))
  {
    IconLoader::GetDefault ()->LoadFromGIconString (_icon_name,
                                                    _size,
                                                    sigc::mem_fun (this, &IconTexture::IconLoaded));
    g_object_unref (icon);
  }
  else
  {
    IconLoader::GetDefault ()->LoadFromIconName (_icon_name,
                                                 _size,
                                                 sigc::mem_fun (this, &IconTexture::IconLoaded));
  }
}

void
IconTexture::CreateTextureCallback (const char *texid, int width, int height, nux::BaseTexture **texture)
{
  nux::BaseTexture *texture2D = nux::CreateTexture2DFromPixbuf (_pixbuf_cached, true);
  *texture = texture2D;
}

void
IconTexture::Refresh (GdkPixbuf *pixbuf)
{
  TextureCache *cache = TextureCache::GetDefault ();
  char *id = NULL;

  _pixbuf_cached = pixbuf;

  // Cache the pixbuf dimensions so we scale correctly
  _texture_width = gdk_pixbuf_get_width (pixbuf);
  _texture_height = gdk_pixbuf_get_height (pixbuf);

  // Try and get a texture from the texture cache
  id = g_strdup_printf ("IconTexture.%s", _icon_name);
  if (_texture_cached)
    _texture_cached->UnReference ();

  _texture_cached = cache->FindTexture (id,
                                        _texture_width,
                                        _texture_height,
                                        sigc::mem_fun (this, &IconTexture::CreateTextureCallback));
  _texture_cached->Reference ();

  QueueDraw ();

  g_free (id);
}

void
IconTexture::IconLoaded (const char *icon_name, guint size, GdkPixbuf *pixbuf)
{
  if (GDK_IS_PIXBUF (pixbuf))
  {
    Refresh (pixbuf);
  }
  else
  {
    _loading = false;
    SetByIconName (DEFAULT_ICON, _size);
  }
}

void
IconTexture::Draw (nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry ();

  GfxContext.PushClippingRectangle (geo);

  if (_texture_cached)
  {
    nux::TexCoordXForm texxform;
    texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_COORD);
    texxform.SetWrap (nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

    GfxContext.QRP_1Tex (geo.x + ((geo.width - _texture_width)/2),
                         geo.y + ((geo.height - _texture_height)/2),
                         _texture_width,
                         _texture_height,
                         _texture_cached->GetDeviceTexture (),
                         texxform,
                         nux::Color::White);
  }

  GfxContext.PopClippingRectangle ();
}

void
IconTexture::GetTextureSize (int *width, int *height)
{
  if (width)
    *width = _texture_width;
  if (height)
    *height = _texture_height;
}

bool
IconTexture::CanFocus ()
{
  return false;
}

const gchar*
IconTexture::GetName ()
{
	return "IconTexture";
}


void
IconTexture::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
  g_variant_builder_add (builder, "{sv}", "iconname", g_variant_new_string (_icon_name));
}
