/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include "PlacesSettings.h"
#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlacesSimpleTile.h"

#include <NuxImage/GdkGraphics.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

PlacesSimpleTile::PlacesSimpleTile (const char *icon_name,
                                    const char *label,
                                    int         icon_size,
                                    bool defer_icon_loading,
                                    const void *id)
: PlacesTile (NUX_TRACKER_LOCATION, id),
  _label (NULL),
  _icon (NULL),
  _uri (NULL)
{
  GtkSettings *settings = gtk_settings_get_default ();
  gchar *font = NULL;
  gchar *fontstring = NULL;
  
  nux::VLayout *layout = new nux::VLayout ("", NUX_TRACKER_LOCATION);

  _label = g_strdup (label);
  _icon = g_strdup (icon_name);

  _icontex = new IconTexture (_icon, icon_size, defer_icon_loading);
  _icontex->SetMinMaxSize (PlacesSettings::GetDefault ()->GetDefaultTileWidth (), icon_size);
  _icontex->SinkReference ();
  AddChild (_icontex);

  _cairotext = new nux::StaticCairoText (_label);
  _cairotext->SinkReference ();

  g_object_get (settings, "gtk-font-name", &font, NULL);
  fontstring = g_strdup_printf ("%s normal 10", font);
  
  _cairotext->SetFont (fontstring);
  _cairotext->SetTextEllipsize (nux::StaticCairoText::NUX_ELLIPSIZE_START);
  _cairotext->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_CENTRE);
  _cairotext->SetMaximumWidth (140);

  layout->AddLayout (new nux::SpaceLayout (0, 0, 12, 12));
  layout->AddView (_icontex, 0, nux::eCenter, nux::eFull);
  layout->AddLayout (new nux::SpaceLayout (0, 0, 12, 12));
  layout->AddView (_cairotext, 0, nux::eCenter, nux::eFull);

  SetMinMaxSize (160, 128);

  SetLayout (layout);

  SetDndEnabled (true, false);

  g_free (font);
  g_free (fontstring);
}


PlacesSimpleTile::~PlacesSimpleTile ()
{
  _icontex->UnReference ();
  _cairotext->UnReference ();

  g_free (_label);
  g_free (_icon);
  g_free (_uri);
}

void
PlacesSimpleTile::DndSourceDragBegin ()
{
  Reference ();
  ubus_server_send_message (ubus_server_get_default (),
                            UBUS_PLACE_VIEW_CLOSE_REQUEST,
                            NULL);
}

nux::NBitmapData * 
PlacesSimpleTile::DndSourceGetDragImage ()
{
  nux::NBitmapData *result = 0;
  GdkPixbuf *pbuf;
  GtkIconTheme *theme;
  GtkIconInfo *info;
  GError *error = NULL;
  GIcon *icon;
  
  const char *icon_name = _icon;
  int size = 64;
  
  if (!icon_name)
    icon_name = "application-default-icon";
   
  theme = gtk_icon_theme_get_default ();
  icon = g_icon_new_for_string (icon_name, NULL);

  if (G_IS_ICON (icon))
  {
    info = gtk_icon_theme_lookup_by_gicon (theme, icon, size, (GtkIconLookupFlags)0);
    g_object_unref (icon);
  }
  else
  {   
    info = gtk_icon_theme_lookup_icon (theme,
                                       icon_name,
                                       size,
                                       (GtkIconLookupFlags) 0);
  }

  if (!info)
  {
    info = gtk_icon_theme_lookup_icon (theme,
                                       "application-default-icon",
                                       size,
                                       (GtkIconLookupFlags) 0);
  }
        
  if (gtk_icon_info_get_filename (info) == NULL)
  {
    gtk_icon_info_free (info);
    info = gtk_icon_theme_lookup_icon (theme,
                                       "application-default-icon",
                                       size,
                                       (GtkIconLookupFlags) 0);
  }

  pbuf = gtk_icon_info_load_icon (info, &error);
  gtk_icon_info_free (info);

  if (GDK_IS_PIXBUF (pbuf))
  {
    nux::GdkGraphics graphics (pbuf);
    result = graphics.GetBitmap ();
    g_object_unref (pbuf);
  }
  
  return result;
}

std::list<const char *> 
PlacesSimpleTile::DndSourceGetDragTypes ()
{
  std::list<const char*> result;
  result.push_back ("text/uri-list");
  return result;
}

const char *
PlacesSimpleTile::DndSourceGetDataForType (const char *type, int *size, int *format)
{
  *format = 8;

  if (_uri)
  {
    *size = strlen (_uri);
    return _uri;
  }
  else
  {
    *size = 0;
    return 0;
  }
}

void
PlacesSimpleTile::DndSourceDragFinished (nux::DndAction result)
{
  UnReference ();
}

nux::Geometry
PlacesSimpleTile::GetHighlightGeometry ()
{
  nux::Geometry base = GetGeometry ();
  int width = 0, height = 0;

  _icontex->GetTextureSize (&width, &height);
  
  _highlight_geometry.x = (base.width - width) / 2;
  _highlight_geometry.y = 12;
  _highlight_geometry.width = width;
  _highlight_geometry.height = height;

  return _highlight_geometry;
}

const char *
PlacesSimpleTile::GetLabel ()
{
  return _label;
}

const char *
PlacesSimpleTile::GetIcon ()
{
  return _icon;
}

const char *
PlacesSimpleTile::GetURI ()
{
  return _uri;
}

void
PlacesSimpleTile::SetURI (const char *uri)
{
  if (_uri)
    g_free (_uri);
  
  _uri = NULL;

  if (uri)
    _uri = g_strdup (uri);
}

const gchar*
PlacesSimpleTile::GetName ()
{
	return "PlacesTile";
}

const gchar *
PlacesSimpleTile::GetChildsName ()
{
  return "PlacesTileContents";
}

void
PlacesSimpleTile::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}

void
PlacesSimpleTile::LoadIcon ()
{
  _icontex->LoadIcon ();

  QueueDraw ();
}
