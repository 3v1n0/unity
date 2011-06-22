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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include "PlacesSettings.h"
#include "PlacesSettings.h"
#include "ubus-server.h"
#include "UBusMessages.h"

#include "PlacesHorizontalTile.h"

#include <Nux/HLayout.h>
#include <NuxImage/GdkGraphics.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

PlacesHorizontalTile::PlacesHorizontalTile (const char *icon_name,
                                            const char *label,
                                            const char *comment,
                                            int         icon_size,
                                            bool defer_icon_loading,
                                            const void *id)
: PlacesTile (NUX_TRACKER_LOCATION, id),
  _label (NULL),
  _icon (NULL),
  _uri (NULL)
{
  _label = g_strdup (label);
  _icon = g_strdup (icon_name);
  _comment = g_strdup_printf ("<small>%s</small>", comment);

  int w = (PlacesSettings::GetDefault ()->GetDefaultTileWidth () * 2) - icon_size - 24;//padding
  int lines = 0;

  nux::HLayout *layout = new nux::HLayout ("", NUX_TRACKER_LOCATION);
  layout->AddLayout (new nux::SpaceLayout (6, 6, 0, 0));

  _icontex = new IconTexture (_icon, icon_size, defer_icon_loading);
  _icontex->SetMinMaxSize (icon_size, icon_size);
  AddChild (_icontex);
  layout->AddView (_icontex, 0, nux::eLeft, nux::eFix);

  layout->AddLayout (new nux::SpaceLayout (6, 6, 0, 0));
  
  nux::VLayout *vlayout = new nux::VLayout ("", NUX_TRACKER_LOCATION);
  layout->AddView (vlayout, 1, nux::eLeft, nux::eFull);

  vlayout->AddLayout (new nux::SpaceLayout (0, 0, 6, 6));

  _cairotext = new nux::StaticCairoText (_label);
  _cairotext->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_LEFT);
  _cairotext->SetMaximumWidth (w);
  _cairotext->SetLines (-2);
  vlayout->AddView (_cairotext, 0, nux::eLeft, nux::eFull);
  lines = _cairotext->GetLineCount ();

  _cairotext = new nux::StaticCairoText (_comment);
  _cairotext->SetTextEllipsize (nux::StaticCairoText::NUX_ELLIPSIZE_END);
  _cairotext->SetTextAlignment (nux::StaticCairoText::NUX_ALIGN_LEFT);
  _cairotext->SetLines (-1 * (4 - lines));
  _cairotext->SetMaximumWidth (w);
  _cairotext->SetTextColor (nux::Color (1.0f, 1.0f, 1.0f, 0.8f));
  vlayout->AddView (_cairotext, 1, nux::eLeft, nux::eFull);

  SetLayout (layout);
  
  SetDndEnabled (true, false);
}


PlacesHorizontalTile::~PlacesHorizontalTile ()
{
  g_free (_comment);
  g_free (_label);
  g_free (_icon);
  g_free (_uri);
}

void
PlacesHorizontalTile::DndSourceDragBegin ()
{
  Reference ();
  ubus_server_send_message (ubus_server_get_default (),
                            UBUS_PLACE_VIEW_CLOSE_REQUEST,
                            NULL);
}

nux::NBitmapData * 
PlacesHorizontalTile::DndSourceGetDragImage ()
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
PlacesHorizontalTile::DndSourceGetDragTypes ()
{
  std::list<const char*> result;
  result.push_back ("text/uri-list");
  return result;
}

const char *
PlacesHorizontalTile::DndSourceGetDataForType (const char *type, int *size, int *format)
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
PlacesHorizontalTile::DndSourceDragFinished (nux::DndAction result)
{
  UnReference ();
}

nux::Geometry
PlacesHorizontalTile::GetHighlightGeometry ()
{
  nux::Geometry base = GetGeometry ();
  int width = 0, height = 0;
  _icontex->GetTextureSize (&width, &height);

  _highlight_geometry.x = 6;
  _highlight_geometry.y = 6;
  _highlight_geometry.width = _icontex->GetMaximumWidth ();
  _highlight_geometry.height = base.height - 12;

  return _highlight_geometry;
}

void
PlacesHorizontalTile::SetURI (const char *uri)
{
  if (_uri)
    g_free (_uri);
  
  _uri = NULL;

  if (uri)
    _uri = g_strdup (uri);
}

const gchar*
PlacesHorizontalTile::GetName ()
{
	return "PlacesHorizontalTile";
}

const gchar *
PlacesHorizontalTile::GetChildsName ()
{
  return "PlacesHorizontalTileContents";
}

void
PlacesHorizontalTile::AddProperties (GVariantBuilder *builder)
{
  nux::Geometry geo = GetGeometry ();

  g_variant_builder_add (builder, "{sv}", "x", g_variant_new_int32 (geo.x));
  g_variant_builder_add (builder, "{sv}", "y", g_variant_new_int32 (geo.y));
  g_variant_builder_add (builder, "{sv}", "width", g_variant_new_int32 (geo.width));
  g_variant_builder_add (builder, "{sv}", "height", g_variant_new_int32 (geo.height));
}
