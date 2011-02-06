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
#include "IconTexture.h"

#include <glib.h>
#include <pango/pangocairo.h>
#include <gtk/gtk.h>

#define DEFAULT_ICON "application-default-icon"

IconTexture::IconTexture (const char *icon_name, unsigned int size)
: TextureArea (NUX_TRACKER_LOCATION),
  _icon_name (NULL)
{
  if (_icon_name && strlen (_icon_name) > 3)
    _icon_name = g_strdup ("folder");
  else
    _icon_name = g_strdup (icon_name);

  _size = size;
  Refresh ();
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
  Refresh ();
}

void
IconTexture::SetByFilePath (const char *file_path, unsigned int size)
{
  g_free (_icon_name);
  _icon_name = g_strdup (file_path);
  _size = size;
  Refresh ();
}

void
IconTexture::Refresh ()
{
  char   *file_path = NULL;
  char   *stripped_icon_name = NULL;
  char   *ext_dot_pos = NULL;
  int     i;
  GError *error = NULL;
  GIcon  *icon;

  icon = g_icon_new_for_string (_icon_name, &error);

  if (G_IS_ICON (icon))
  {
    if (G_IS_THEMED_ICON (icon))
    {
      GtkIconInfo *info;

      info = gtk_icon_theme_lookup_by_gicon (gtk_icon_theme_get_default (),
                                             icon,
                                             _size,
                                             (GtkIconLookupFlags)0);
      if (info)
      {
        file_path = g_strdup (gtk_icon_info_get_filename (info));

        gtk_icon_info_free (info);
      }
      else
      {
        // Some desktop files put the extension in the icon name for themed icon.
        g_object_unref (icon);
        stripped_icon_name = (char *)g_malloc0 (strlen(_icon_name));
        ext_dot_pos = strchr (_icon_name, '.');
        
        if (ext_dot_pos)
        {
          i = 0;
          while ((_icon_name + i) != ext_dot_pos)
          {
            stripped_icon_name[i] = _icon_name[i];
            i++;
          }
          icon = g_icon_new_for_string (stripped_icon_name, &error);
          info = gtk_icon_theme_lookup_by_gicon (gtk_icon_theme_get_default (),
                                                 icon,
                                                 _size,
                                                 (GtkIconLookupFlags)0);
          g_free (stripped_icon_name);
        }
        if (info)
        {
          file_path = g_strdup (gtk_icon_info_get_filename (info));
          gtk_icon_info_free (info);
        }
        else
        {
          g_warning ("Cannot find themed icon %s", _icon_name);
          return;
        }
      }
    }
    else if (G_IS_FILE_ICON (icon))
    {
      file_path = g_file_get_path (g_file_icon_get_file (G_FILE_ICON (icon)));
    }
    else
    {
      g_warning ("Unsupported GIcon: %s", _icon_name);
      return;
    }

    g_object_unref (icon);
  }
  else if (g_file_test (_icon_name, G_FILE_TEST_EXISTS))
  {
    file_path = g_strdup (_icon_name);
  }
  else
  {
    if (error)
      g_error_free (error);
    error = NULL;

    GtkIconTheme *theme;
    GtkIconInfo *info;

    theme = gtk_icon_theme_get_default ();

    if (!_icon_name)
      _icon_name = g_strdup (DEFAULT_ICON);
    info = gtk_icon_theme_lookup_icon (theme,
                                       _icon_name,
                                       _size,
                                       (GtkIconLookupFlags) 0);
    if (!info || gtk_icon_info_get_filename (info) == NULL)
    {
      g_message ("Could not find icon %s: using default icon", _icon_name);
      info = gtk_icon_theme_lookup_icon (theme,
                                         DEFAULT_ICON,
                                         _size,
                                         (GtkIconLookupFlags) 0);
    }

    file_path = g_strdup (gtk_icon_info_get_filename (info));
  }

  _pixbuf = gdk_pixbuf_new_from_file_at_size (file_path, _size, _size, &error);
  
  if (error == NULL)
  {
    nux::BaseTexture *texture2D = nux::CreateTexture2DFromPixbuf (_pixbuf, true);
    nux::TexCoordXForm texxform;
    texxform.SetTexCoordType (nux::TexCoordXForm::OFFSET_SCALE_COORD);
    texxform.SetWrap (nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
    nux::TextureLayer* texture_layer = new nux::TextureLayer (texture2D->GetDeviceTexture(),
                                                              texxform,
                                                              nux::Color::White,
                                                              true,
                                                              rop);

    SetPaintLayer(texture_layer);
    texture2D->UnReference ();
    g_object_unref (_pixbuf);
  }
  else
  {
    g_warning ("Unable to load '%s' from icon theme: %s",
               _icon_name,
               error ? error->message : "unknown");
    g_error_free (error);
  }

  SetMinMaxSize (_size, _size);
  NeedRedraw ();

  g_free (file_path);
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
  g_variant_builder_add (builder, "{sv}", "have-pixbuf", g_variant_new_boolean (GDK_IS_PIXBUF (_pixbuf)));
  g_variant_builder_add (builder, "{sv}", "iconname", g_variant_new_string (_icon_name));
}
