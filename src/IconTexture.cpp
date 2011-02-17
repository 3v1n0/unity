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
#include "Variant.h"

#include <glib.h>
#include <pango/pangocairo.h>
#include <gtk/gtk.h>

#define DEFAULT_ICON "text-x-preview"

IconTexture::IconTexture (const char *icon_name, unsigned int size)
: TextureArea (NUX_TRACKER_LOCATION),
  _icon_name (NULL),
  _size (size)
{
  _icon_name = g_strdup (icon_name ? icon_name : DEFAULT_ICON);

  LoadIcon ();
}

IconTexture::~IconTexture ()
{
  g_free (_icon_name);
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

  SetMinMaxSize (_size, _size);
}

void
IconTexture::Refresh (GdkPixbuf *pixbuf)
{
  nux::BaseTexture *texture2D = nux::CreateTexture2DFromPixbuf (pixbuf, true);

  nux::TexCoordXForm texxform;
  texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_SCALE_COORD);
  texxform.SetWrap (nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);
  
  nux::IntrusiveSP<nux::IOpenGLBaseTexture> dev = texture2D->GetDeviceTexture();
  dev->SetFiltering (GL_LINEAR, GL_LINEAR);

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  nux::TextureLayer* texture_layer = new nux::TextureLayer (dev,
                                                            texxform,
                                                            nux::Color::White,
                                                            true,
                                                            rop);
  SetPaintLayer(texture_layer);
  texture2D->UnReference ();

  SetMinMaxSize (gdk_pixbuf_get_width (pixbuf) * (_size/(float)gdk_pixbuf_get_height (pixbuf)),
                 _size);
  QueueDraw ();
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
    SetByIconName (DEFAULT_ICON, _size);
  }
}

const gchar*
IconTexture::GetName ()
{
	return "IconTexture";
}


void
IconTexture::AddProperties (GVariantBuilder *builder)
{
  unity::variant::BuilderWrapper(builder)
    .add(GetGeometry())
    .add("iconname", _icon_name);
}
